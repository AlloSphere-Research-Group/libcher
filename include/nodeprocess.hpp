#ifndef NODEPROCESS_HPP
#define NODEPROCESS_HPP

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <memory>
#include <cstdlib>
#include <mutex>

namespace zmq {
    class context_t;
    class socket_t;
}

namespace datk {

typedef struct {
    std::string hostname;
    int port;
    int role;
    int rank;
    int group;
    zmq::socket_t *socket{ nullptr };
} NodeDetails;

typedef enum {
    DESKTOP = 0x00,
    SIMULATOR = 0x10,
    INTERFACE = 0x20,
    RENDERER = 0x30,
    USER = 0xC0
} Role;

/**
 * @brief The NodeProcess class
 */
class NodeProcess
{
public:
    NodeProcess();

//    /**
//     * @brief Synchronize the entire application at this point
//     *
//     * @param [in] timeout Continue without syncing if timeout in ms is reached
//     *
//     * @return true is sync succeeded
//     *
//     * sync() will block the entire application
//     *
//     * How to do this on Windows!!!
//     */
//    bool sync(int timeout = -1) {}

    /**
     * @brief Synchronze current thread/context
     * @param [in] timeout Continue without syncing if timeout in ms is reached
     * @return true is sync succeeded
     *
     * Using syncThread will only block the thread on which it is called
     */
    bool syncThread(int root = 0, int group = -1, int timeout = -1);

    bool syncData(std::string &value, int root = 0, int group = -1, int timeout = -1);
    bool syncData(int &value, int root = 0, int group = -1, int timeout = -1);
    bool syncData(void *data, int size, int root = 0, int group = -1, int timeout = -1);


    void verbose(bool v) {mVerbose = v;}
    bool verbose() {return mVerbose;}

    int rank() { return mRank; }
private:
    int mRank {0};
    Role mRole {DESKTOP};
    int mGroup {0};
    int mPort{ 0 };
    bool mVerbose {true};
    std::vector<NodeDetails> mNodes;
    std::mutex mNodesLock;
    zmq::context_t *mContext{ nullptr };
    zmq::socket_t *mSocket{ nullptr };
};

} // namespace datk


#endif // NODEPROCESS_HPP
