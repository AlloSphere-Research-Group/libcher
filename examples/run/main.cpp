
#include <iostream>

#include "datk.hpp"
#include "json.hpp"

int main(char *argv[], int argc) {
    datk::NodeController controller;
    controller.verbose(true);
    controller.run();

    controller.log("Press enter to quit.");
    getchar();

	return 0;
}