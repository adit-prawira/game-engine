#include "app.hpp"

// std
#include <iostream>
#include <cstdlib>
#include <stdexcept>

int main(){
    live::App app{};
    try {
        app.run();
    }catch(const std::exception &e){
        // only console log error if any
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
