
#include <iostream>
#include <chrono>
#include <thread>

#include "datk.hpp"

int main(char *argv[], int argc) {
	datk::NodeProcess process;

    process.log("Node " + std::to_string(process.rank()) + " before sync");

	process.syncThread();

    process.log("Node " + std::to_string(process.rank()) + " after sync");

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    process.log("Done.");

	return 0;
}