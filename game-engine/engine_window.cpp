#include "engine_window.hpp"
#include <stdexcept>

namespace engine {

    EngineWindow::EngineWindow(int w, int h, std::string name): width{w}, height{h}, windowName{name} {
        this->initWindow();
    };

    // Destroy window  upon class initialization (Constructor)
    EngineWindow::~EngineWindow(){
        glfwDestroyWindow(window);
        glfwTerminate();
    };

    void EngineWindow::initWindow(){
        
        // initialize library
        glfwInit();
        
        // do not use OpenGL context since we are using Vulkan
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        
        // disable window from being resize after creation
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        
        window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    };

    void EngineWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface){
        bool isSuccess = glfwCreateWindowSurface(instance, this->window, nullptr, surface) == VK_SUCCESS;
        if(!isSuccess) throw std::runtime_error("Unable to create window surface");
    }

}
