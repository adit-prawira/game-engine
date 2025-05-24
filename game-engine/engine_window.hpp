#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

namespace engine {
    class EngineWindow {
        public:
            EngineWindow(int w, int h, std::string name);
            ~EngineWindow();
            
            //    Destructor
            EngineWindow(const EngineWindow &) = delete;
            EngineWindow &operator = (const EngineWindow &) = delete;
            
            bool shouldClose(){
                return glfwWindowShouldClose(window);
            }
            VkExtent2D getExtent() {
                return {
                    static_cast<uint32_t> (this->width),
                    static_cast<uint32_t> (this->height)
                };
            }
            void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
        private:
            GLFWwindow *window;
            void initWindow();
            
            const int width;
            const int height;
            std::string windowName;
    };
}
