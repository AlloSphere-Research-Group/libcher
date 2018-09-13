#ifndef NODEIO_HPP
#define NODEIO_HPP

#include <iostream>
#include <string>

namespace datk
{

class NodeIO {
public:

    void log(std::string message) {
        std::cout << prefix() << message << std::endl;
    }

    void logError(std::string message) {
        std::cerr << "!" << prefix() << message << std::endl;
    }

    bool verbose() { return mVerbose; }
    void verbose(bool verbose) { mVerbose = verbose; }

protected:
    int mRank{ -1 };

private:
    bool mVerbose{ false };

    std::string prefix() {
        if (mRank < 0) {
            return "[C] ";
        }
        else {
            return "[" + std::to_string(mRank) + "] ";
        }
        
    }
};

}

#endif //NODEIO_HPP
