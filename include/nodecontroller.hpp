#ifndef NODECONTROLLER_HPP
#define NODECONTROLLER_HPP

#include <cinttypes>
#include <memory>
#include <fstream>
#include <thread>
#include <mutex>

// To avoid having to link to zmq when using static library
namespace zmq {
    class context_t;
    class socket_t;
}

#include <zmq.hpp>
#include "json.hpp"

#include "nodeio.hpp"

namespace datk
{

class RegisteredNode {
public:
    std::string address;
    uint16_t port;
    int rank;
    int group;
};

class NodeController: public NodeIO
{
public:
    NodeController(uint16_t startPort = 11300);

    bool run();
    bool stop();

private:
    void listen(); // Asynchronous function to precess incoming requests

    bool runAsycLocal(std::string command);

    bool processCommand(nlohmann::json &args);

    bool mListening;
    std::unique_ptr<std::thread> mListenerThread;

    std::unique_ptr<zmq::context_t> mContext;
    std::unique_ptr<zmq::socket_t> mSocket;
    uint16_t mStartPort;

    std::mutex mWorldLock;
    std::vector<RegisteredNode> mWorld;
};

}

#endif // NODECONTROLLER_HPP