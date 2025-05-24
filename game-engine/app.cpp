#include "app.hpp"

namespace engine {
    void App::run(){
        while (!engineWindow.shouldClose()){
            glfwPollEvents();
        }
    }
}
