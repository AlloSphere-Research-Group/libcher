#include <string>
#include <iostream>
#include <memory>
#include <cstdlib>
#include <cassert>

#include <zmq.hpp>
#include <libssh/libssh.h>
#include "json.hpp"

#include "nodeprocess.hpp"

using namespace datk;
using namespace std;
using nlohmann::json;

#define AL_VERSION 1


NodeProcess::NodeProcess() {
    const char *controller_port = getenv("DATK_CONTROLLER_PORT");
    const char *rank = getenv("DATK_RANK");

    if (controller_port && rank) {
        mRank = std::atoi(rank);
        mContext = std::make_shared<zmq::context_t>();
        mSocket = std::make_shared<zmq::socket_t>(*mContext, ZMQ_REQ);

        float timeoutSecs = 5.0;
        mSocket->setsockopt(ZMQ_RCVTIMEO, (int)(timeoutSecs * 1000));
        mSocket->setsockopt(ZMQ_SNDTIMEO, (int)(timeoutSecs * 1000));

        // Report to controller node
        string address = "tcp://127.0.0.1:" + string(controller_port);

        try {
            mSocket->connect(address);
        }
        catch (zmq::error_t &) {
            logError("Error connecting to " + address);
            throw std::exception("Error binding");
        }
        if (verbose()) { log("Connected to controller: " +  address); }
        try {
#ifdef DATK_WINDOWS
            mPid = (int64_t) GetCurrentProcessId();
#else DATK_UNIX
            mPid = (int64_t) getpid();
#endif
            zmq::message_t pidMessage(&mPid, sizeof(int64_t));
            mSocket->send(pidMessage);
            log("Sent pid: " + std::to_string(mPid));
        }
        catch (zmq::error_t &) {
            logError("Failed to send sync acknowledgement.");
        }

        zmq::message_t regAck;
        if (!mSocket->recv(&regAck)) {
            logError("Controller reply timeout.");
            throw std::exception("Controller reply timeout");
            //TODO: should we retry a couple of times?
        }

        // TODO verify response. This could be exploited
        if (verbose()) { log("Received ack from controller." + std::string((const char *)regAck.data())); }

        // Make socket server for this node and register it with controller
        mServerSocket = std::make_shared<zmq::socket_t>(*mContext, ZMQ_REQ);
        uint16_t port = 15000;
        bool connected = false;
        do {
            address = "tcp://*:" + std::to_string(port);
            try {
                mServerSocket->bind(address);
                connected = true;
            }
            catch (zmq::error_t &) {
                port++;
            }
        } while (!connected && port < UINT16_MAX);

        if (!connected) {
            throw std::exception("Could not create node server.");
        }

        if (verbose()) { log("Created server at:" + address); }

        // Register port
        json command;
        command["datk_command"] =
            {
                {"command" , "registerServer"},
                {"port", port}
        };
        command["id"] = { {"rank", mRank}, {"pid", mPid} , {"count", mMessageCount} };

        //TODO: json library provides cbor, msgpack and ubjson. Which one is preferred?
        //auto commandBinary = json::to_cbor(command);
        std::string commandBinary = command.dump(); // TODO: Currently sending json text. Should we send serialized binary?
        commandBinary.push_back(0); //TODO is adding a null needed? zmq needs it, but does json provide it?

        if (verbose()) { log("Sending command size " + std::to_string(commandBinary.size())); }

        try {
            sendToController(commandBinary, true);
        }
        catch (std::exception &) {
            logError("Registration message to controller failed.");
            throw std::exception("Registration message to controller failed.");
        }
        zmq::message_t ack;
        mSocket->recv(&ack);
        uint64_t msgCountReport = *((uint64_t *)ack.data());
        
        if (msgCountReport != mMessageCount) {
            logError("Message count mismatch.");
        }
        mMessageCount++;

        if (rank) {
            const char *group = getenv("AL_GROUP");
            const char *port = getenv("AL_PORT");
            const char *role = getenv("AL_ROLE");
            const char *world = getenv("AL_WORLD");
            const char *version = getenv("AL_VERSION");
            
            if (role) {
                mRole = (Role)std::atoi(role);
                if (verbose()) { log("Role " + std::to_string(mRole)); }
            }

            if (version) {
                int versionNumber = std::stoi(version);
                if (versionNumber != AL_VERSION) {
                    std::cerr << "Version number mismatch " << versionNumber << " != " << AL_VERSION << std::endl;
                }
            }
        }
    } else {
        if (verbose()) { log("Not running distributed");}
    }
}

bool NodeProcess::syncThread(int root, int group, int timeout)
{
    if (verbose()) {
        log("syncThread " + std::to_string(root) + " " + std::to_string(group));
    }

    unique_lock<mutex> lk(mNodesLock);
    assert(root >= 0);
    if (rank() == -1) { // Not running distributed
        return true;
    }
    if (root == rank()) { // If this node is the sync master for this call
        for (auto node : mNodes) {
            if (node.port != mPort) {
                std::cout << "Syncing " << node.hostname << ":" << node.port << " rank "<< node.rank << std::endl;
                if (group < 0 || node.group == group) { // If all nodes or group matches

                    zmq::message_t syncRequest(5);
                    memcpy(syncRequest.data(), "sync\0", 5);
                    node.socket->send(syncRequest);

                    zmq::message_t reply;
                    node.socket->recv(&reply);
                    int number;
                    number = *((int *)reply.data());
                    std::cout << "Received Confirmation " << number << std::endl;

                }
            }
            
        }
    }
    else {
        if (group < 0 || mGroup == group) {
            zmq::message_t request;

            if (verbose()) { log("Waiting for sync");};
            //  Wait for next request from client
            mSocket->recv(&request);
            //std::cout << "Received Hello " << rank() << " " << (char *) request.data() << std::endl;

            zmq::message_t reply(sizeof(int));

            memcpy(reply.data(), &mRank, sizeof(int));
            mSocket->send(reply);

            if (verbose()) { log("Sent Reply"); };
        }

    }
    return false;
}

bool NodeProcess::syncData(std::string &value, int root, int group, int timeout)
{
    int size = (int)value.size();

    bool sizeSync = syncData(size, root, group, timeout);
    if (mRank == root) { // This is the sender
        for (auto node: mNodes) {
            if (group < 0 || node.group == group) {

            }
        }
    } else { // Receivers
    }
    return true;
}

bool NodeProcess::syncData(int &value, int root, int group, int timeout)
{

    //return syncData(value, , root, group, timeout);
    return false;
}

bool NodeProcess::syncData(void *data, int size, int root, int group, int timeout)
{

    return false;
}

bool datk::NodeProcess::sendToController(std::string& data, bool exceptionOnFail)
{
    bool success = false;
    zmq::message_t message((void *)data.data(), data.size()); // Zero copy
    try {
        mSocket->send(message);
        success = true;
    }
    catch (zmq::error_t &) {
        if (exceptionOnFail) {
            std::exception("Message to controller failed.");
        }
    }
    return success;
}
