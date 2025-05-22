#include "live_window.hpp"
#include <stdexcept>

namespace live {

LiveWindow::LiveWindow(int w, int h, std::string name): width{w}, height{h}, windowName{name} {
    this->initWindow();
};

// Destroy window  upon class initialization (Constructor)
LiveWindow::~LiveWindow(){
    glfwDestroyWindow(window);
    glfwTerminate();
};

void LiveWindow::initWindow(){
    
    // initialize library
    glfwInit();
    
    // do not use OpenGL context since we are using Vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    
    // disable window from being resize after creation
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
};

void LiveWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface){
    bool isSuccess = glfwCreateWindowSurface(instance, this->window, nullptr, surface) == VK_SUCCESS;
    if(!isSuccess) throw std::runtime_error("Unable to create window surface");
}

}
