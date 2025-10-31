#include "cli/CommandHandler.h"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        bik::CommandHandler handler;
        return handler.execute(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
