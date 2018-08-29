#include "datk.hpp"
#include <iostream>

int main(char *argv[], int argc) {
	datk::NodeProcess process;

    std::cout << "Hello I am " << process.rank() << std::endl;

	process.syncThread();
	return 0;
}