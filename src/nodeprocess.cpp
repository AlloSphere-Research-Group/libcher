#include <string>
//#include <unique_lock>

#include <zmq.hpp>

#include "nodeprocess.hpp"

using namespace datk;
using namespace std;

#define AL_VERSION 1

NodeProcess::NodeProcess() {
    const char *rank = getenv("AL_RANK");
    const char *group = getenv("AL_GROUP");
    const char *port = getenv("AL_PORT");
    const char *role = getenv("AL_ROLE");
    const char *world = getenv("AL_WORLD");
    const char *version = getenv("AL_VERSION");

    if (rank) {
        mRank = std::atoi(rank);

        mContext = new zmq::context_t(1);
        mSocket = new zmq::socket_t(*mContext, ZMQ_PAIR);
        mPort = 5555 + mRank;

        if (port) {
            mPort = atoi(port);
        }
        string address = "tcp://*:" + to_string(mPort);

        if (verbose()) { std::cout << "Binding to " << address << std::endl; }
        mSocket->bind(address);

        if (world) {
            std::stringstream ss(world);
            std::string item;
            std::vector<std::string> tokens;
            while (getline(ss, item, ',')) {
                tokens.push_back(item);
            }
            if (verbose() && this->rank() == 0) {
                std::cout << "--- World ---" << std::endl;
            }
            if (tokens.size() > 0 && tokens.size()% 5 == 0) {
                auto it = tokens.begin();
                while (it != tokens.end()) {
                    std::string hostname = *it++;
                    int port = std::stoi(*it++);
                    int role = std::stoi(*it++);
                    int rank = std::stoi(*it++);
                    int group = std::stoi(*it++);

                    string address = "tcp://" + hostname + ":" + to_string(port);
                    //string address = "tcp://*:*";

                    if (verbose() && mRank == 0) { 
                        std::cout << hostname << ":" << port 
                            << " rank " << rank << " group " << group
                            << " role " << role <<  " " << address << std::endl; 

                        std::cout << " --- --- ---" << std::endl;
                    }

                    mNodes.push_back({hostname, port, role, rank, group,
                        new zmq::socket_t(*mContext, ZMQ_PAIR)});
                    try {
                        mNodes.back().socket->connect(address);
                    }
                    catch ( ...) {
                        std::cerr << "Could not connect socket" << std::endl;
                    }

                }
            }
        }

        if (role) {
            mRole = (Role) std::atoi(role);
            if (verbose()) { std::cout << "Role " << mRole << std::endl; }
        }

        if (version) {
            int versionNumber = std::stoi(version);
            if (versionNumber != AL_VERSION) {
                std::cerr << "Version number mismatch " << versionNumber << " != " << AL_VERSION << std::endl;
            }
        }
    } else {
        if (verbose()) { std::cout << "Not running distributed" << std::endl;}
    }

}

bool NodeProcess::syncThread(int root, int group, int timeout)
{
    std::cout << "syncThread " << root << " " << group << std::endl;
    unique_lock<mutex> lk(mNodesLock);
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

            std::cout << "Waiting for sync " << std::endl;
            //  Wait for next request from client
            mSocket->recv(&request);
            //std::cout << "Received Hello " << rank() << " " << (char *) request.data() << std::endl;

            zmq::message_t reply(sizeof(int));

            memcpy(reply.data(), &mRank, sizeof(int));
            mSocket->send(reply);

            std::cout << "Sent Reply " << std::endl;
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
