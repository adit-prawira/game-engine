#include "live_window.hpp"

namespace live {

LiveWindow::LiveWindow(int w, int h, std::string name): width{w}, height{h}, windowName{name} {
    initWindow();
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

}
