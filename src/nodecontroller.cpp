#include <string>
#include <iostream>
#include <cstdlib>
//#include <unique_lock>

#include <zmq.hpp>
#include <libssh/libssh.h>

#include "json.hpp"

#include "nodecontroller.hpp"

using namespace datk;
using nlohmann::json;


#ifdef DATK_WINDOWS
// Windows doesn't provide setenv
int setenv(const char *name, const char *value, int overwrite = 0)
{
    int errcode = 0;
    if (!overwrite) {
        size_t envsize = 0;
        errcode = getenv_s(&envsize, NULL, 0, name);
        if (errcode || envsize) return errcode;
    }
    return _putenv_s(name, value);
}
#endif

NodeController::NodeController(uint16_t startPort)
    : mStartPort(startPort)
{
    std::ifstream jsonFile("datk.json");
    jsonFile.open("datk.json", std::ifstream::in);
    nlohmann::json config;
    if (jsonFile.good()) {
        jsonFile >> config;
    }
    if (config.find("nodes") == config.end()) {
        config["nodes"];
    }
}

bool NodeController::run() {

    // Configure context and socket connection to root node
    mContext = std::make_unique<zmq::context_t>(1);
    mSocket = std::make_unique<zmq::socket_t>(*mContext, ZMQ_REP);

    float timeoutSecs = 5.0;
    mSocket->setsockopt(ZMQ_RCVTIMEO, (int) (timeoutSecs* 1000));
    mSocket->setsockopt(ZMQ_SNDTIMEO, (int)(timeoutSecs * 1000));

    // Create local socket
    bool connected = false;
    uint16_t port = mStartPort;
    std::string address;
    do {
        address = "tcp://127.0.0.1:" + std::to_string(port);
        try {
            mSocket->bind(address);
            connected = true;
        }
        catch (zmq::error_t &) {
            port++;
        }
    } while (!connected && port < UINT16_MAX);

    if (!connected) {
        throw std::exception();
    }

    if (verbose()) { log("Controller Bound to " + address); }

    // Launch root node
    std::string rootNodeAddress = "localhost";
    std::string rootNodeCommand = "simpleTest";
    if (rootNodeAddress == "localhost") {
        setenv("DATK_CONTROLLER_PORT", std::to_string(port).c_str());
        setenv("DATK_RANK", std::to_string(0).c_str());
        if (!runAsycLocal(rootNodeCommand)) {
            logError("Error executing process in local root node.");
            throw std::exception("Error executing process in local root node.");
        }
    }
    else { // Remote root node
   /*     ssh_session my_ssh_session = ssh_new();
        if (my_ssh_session == NULL)
            exit(-1);
        ...
            ssh_free(my_ssh_session);
            */
    }

    zmq::message_t registration;
    if (!mSocket->recv(&registration)) {
        logError("Registration from root node timed out.");
        throw std::exception("Registration from root node timed out.");
    }
    int64_t *rootPid = (int64_t *) registration.data();
    log("Received Confirmation root pid: " + std::to_string(*rootPid));

    // Run the rest of the nodes
    setenv("DATK_CONTROLLER_PORT", std::to_string(port).c_str());
    setenv("DATK_RANK", std::to_string(1).c_str(), 1);
    if (!runAsycLocal(rootNodeCommand)) {
        throw std::exception("Error executing process in local node.");
    }

    zmq::message_t regAcknowledge("sync10\0",7);
    if (!mSocket->send(regAcknowledge)) {
        logError("Registration acknowledge timed out.");
        throw std::exception("Registration acknowledge timed out.");
    }

    mListening = true;
    mListenerThread = std::make_unique<std::thread>(
        std::bind(&NodeController::listen, this));

    return false;
}

bool datk::NodeController::stop()
{
    mListening = false;
    if (mListenerThread) {
        mListenerThread->join();
    }
    return true;
}

void datk::NodeController::listen()
{
    if (verbose()) { log("Controller listening thread active." ); }
    while (mListening) {
        zmq::message_t reply;
        if (!mSocket->recv(&reply)) {
            if (verbose()) {
                log("No message received. Sleeping..."); 
                std::unique_lock<std::mutex> lk(mWorldLock);
                for (auto node : mWorld) {
                    log("node: " + node.address + ":" + std::to_string(node.port));
                }
            }
        } else {
            if (verbose()) { log("Received controller request. Size " + std::to_string(reply.size())); }

            std::string msg_str(static_cast<char*>(reply.data()), reply.size());
            uint64_t msgCount{ 0 };
            try {
                // Is this parsing secure?
                //auto msg = json::from_cbor(msg_str); // TODO use binary to move things around?
                auto msg = json::parse(msg_str);
                log("Received command " + msg["datk_command"].dump() + " from " + msg["id"].dump());
                if (!processCommand(msg)) {
                    logError("Error processing command.");
                }

                msgCount = msg["id"]["count"];
            }
            catch ( ... ) {
                logError("Parsing command failed: " + msg_str);
            }
            zmq::message_t ack(&msgCount, sizeof(uint64_t));
            mSocket->send(ack);
            //mSocket->send()
        }
    }

    if (verbose()) { log("Controller listening thread done."); }
}

bool datk::NodeController::runAsycLocal(std::string command)
{
    //FIXME: Quick and dirty async running. Should be replaced
#ifdef DATK_WINDOWS
    command = "start " + command;
#else
    command += " &";
#endif
    return system((command).c_str()) == 0; // Reports if system call was sucessful (meaningless most of the time as it is here...)
}

bool datk::NodeController::processCommand(nlohmann::json & args)
{
    if (args.find("datk_command") == args.end()) {
        return false;
    }
    std::string command = args["datk_command"]["command"];
    if (command == "registerServer") {
        std::unique_lock<std::mutex> lk(mWorldLock);
        mWorld.push_back(RegisteredNode{"localhost", args["datk_command"]["port"], args["id"]["rank"], 1 });
    }
    return true;
}
