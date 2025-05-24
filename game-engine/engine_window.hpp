#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

namespace engine {
    class EngineWindow {

        private:
            GLFWwindow *window;
            void initWindow();
            
            const int width;
            const int height;
            std::string windowName;
            
        public:
            EngineWindow(int w, int h, std::string name);
            ~EngineWindow();
            
            //    Destructor
            EngineWindow(const EngineWindow &) = delete;
            EngineWindow &operator = (const EngineWindow &) = delete;
            
            bool shouldClose(){
                return glfwWindowShouldClose(window);
            }
            
            void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
    };
}
