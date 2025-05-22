#pragma once
#include "live_window.hpp"

//std
#include <vector>
#include <string>

namespace live {

struct SwapChainSupportDetails{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices{
    uint32_t graphicsFamily;
    uint32_t presentFamily;
    bool graphicFamilyHasValue = false;
    bool presentFamilyHasValue = false;
    bool isComplete(){
        return graphicFamilyHasValue && presentFamilyHasValue;
    }
};

class LiveDevice {
    public:
        #ifdef NDEBUG
        const bool enableValidationLayers = false;
        #else
        const bool enableValidationLayers = true;
        #endif
        
        LiveDevice(LiveWindow &window);
        ~LiveDevice();

        // not copyable
        LiveDevice(const LiveDevice &) = delete;
        LiveDevice &operator = (const LiveDevice &) = delete;
        
        // not movable
        LiveDevice(LiveDevice &&) = delete;
        LiveDevice &operator = (LiveDevice &&) = delete;


        VkCommandPool getCommandPool(){
            return commandPool;
        }

        VkDevice device(){
            return this->device_;
        }
        VkSurfaceKHR surface(){
            return this->surface_;
        }
        VkQueue graphicsQueue(){
            return this->graphicsQueue_;
        }
        VkQueue presentQueue(){
            return this->presentQueue_;
        }
        
        SwapChainSupportDetails getSwapChainSupportDetails(){
            return this->querySwapChainSupport(physicalDevice);
        }

        QueueFamilyIndices findPhysicalQueueFamilies(){
            return this->findQueueFamilies(physicalDevice);
        }

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    
        VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

        // Buffer utility functions
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);
        void copyBuffer(VkBuffer sourceBuffer, VkBuffer destinationBuffer, VkDeviceSize size);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);
        void createImageWithInfo(const VkImageCreateInfo &imageInfo, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);
        VkPhysicalDeviceProperties properties;

    private:
        void createInstance();
        void setupDebugMessenger();
        void createSurface();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createCommandPool();
        
        VkApplicationInfo buildApplicationInfo();
        VkInstanceCreateInfo buildInstanceCreateInfo(const VkApplicationInfo* appInfo);
        VkDeviceQueueCreateInfo buildQueueCreateInfo(uint32_t queueFamilyIndex, float* queuePriority);
        VkPhysicalDeviceFeatures buildDeviceFeatures();
        VkDeviceCreateInfo buildBaseDeviceCreateInfo(uint32_t queueCreateInfoCount, const VkDeviceQueueCreateInfo* pQueueCreateInfos, const VkPhysicalDeviceFeatures* enabledFeatures, uint32_t enabledExtensionCount, const char* const* ppEnabledExtensionNames);
        VkCommandPoolCreateInfo buildCommandPoolCreateInfo(uint32_t queueFamilyIndex);
    
        // utility functions
        bool isDeviceSuitable(VkPhysicalDevice device);
        std::vector<const char *> getRequiredExtensions();
        bool checkValidationLayerSupport();
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
        void validateGLfwRequiredInstanceExtensions();
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        LiveWindow &window;
        VkCommandPool commandPool;

        VkDevice device_;
        VkSurfaceKHR surface_;
        VkQueue graphicsQueue_;
        VkQueue presentQueue_;

        const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_VALIDATION"};
        const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};
}
