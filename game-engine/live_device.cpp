#include "live_device.hpp"

// std
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <set>

namespace live {
// Utilities
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData){
        std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger){
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if(func ==nullptr) return VK_ERROR_EXTENSION_NOT_PRESENT;
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }

    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator){
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if(func == nullptr) return;
        func(instance, debugMessenger, pAllocator);
    }

    // Publices
    LiveDevice::LiveDevice(LiveWindow &window): window{window}{
        this->createInstance();
        this->setupDebugMessenger();
        this->createSurface();
        this->pickPhysicalDevice();
        this->createLogicalDevice();
        this->createCommandPool();
    }

    LiveDevice::~LiveDevice(){
        vkDestroyCommandPool(this->device_, this->commandPool, nullptr);
        vkDestroyDevice(this->device_, nullptr);
        if(this->enableValidationLayers){
            DestroyDebugUtilsMessengerEXT(this->instance, this->debugMessenger, nullptr);
        }
        vkDestroySurfaceKHR(this->instance, this->surface_, nullptr);
        vkDestroyInstance(this->instance, nullptr);
    }

    uint32_t LiveDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties){
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(this->physicalDevice, &memoryProperties);
        for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++){
            bool matchedBit = typeFilter & (1 << i);
            bool matchedMemoryTypeProperties = (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;
            bool isFound = matchedBit && matchedMemoryTypeProperties;
            if(isFound) return i;
        }
        throw std::runtime_error("Failed to find suitable memory type");
    }

    VkFormat LiveDevice::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features){
        for(VkFormat candidate:candidates){
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(this->physicalDevice, candidate, &properties);
            bool isImageTillingLinear = tiling==VK_IMAGE_TILING_LINEAR;
            bool isLinearTillingFeatures = (properties.linearTilingFeatures & features) == features;
            bool isImageTillingOptimal = tiling==VK_IMAGE_TILING_OPTIMAL;
            bool isOptimalTillingFeatures = (properties.optimalTilingFeatures & features) == features;
            
            bool isValidFormat = (isImageTillingLinear && isLinearTillingFeatures) || (isImageTillingOptimal && isOptimalTillingFeatures);\
            if(isValidFormat) return candidate;
        }
        throw std::runtime_error("Failed to find supported format!");
    }

    // Privates
    void LiveDevice::createInstance(){
        if(this->enableValidationLayers && !this->checkValidationLayerSupport()) throw std::runtime_error("Validation layers requested, but not available");
        VkApplicationInfo applicationInfo = this->buildApplicationInfo();
        VkInstanceCreateInfo instanceCreateInfo = this->buildInstanceCreateInfo(&applicationInfo);
        if(vkCreateInstance(&instanceCreateInfo, nullptr, &this->instance)) throw std::runtime_error("Failed to create instance");
        this->validateGLfwRequiredInstanceExtensions();
    }

    void LiveDevice::setupDebugMessenger(){
        if(!this->enableValidationLayers) return;
        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        this->populateDebugMessengerCreateInfo(createInfo);
        bool isSuccess = CreateDebugUtilsMessengerEXT(this->instance, &createInfo, nullptr, &this->debugMessenger) == VK_SUCCESS;
        if(!isSuccess) throw std::runtime_error("Failed to setup debugger messenger!");
    }

    void LiveDevice::createSurface(){
        window.createWindowSurface(this->instance, &this->surface_);
    }

    void LiveDevice::pickPhysicalDevice(){
        uint32_t deviceCount =0;
        vkEnumeratePhysicalDevices(this->instance, &deviceCount, nullptr);
        if(deviceCount ==0) throw std::runtime_error("Failed to find GPU with Vulkan support!");
        std::cout << "Total GPU devices available -> " << deviceCount << std::endl;
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(this->instance, &deviceCount, devices.data());
        
        for(const VkPhysicalDevice &device:devices){
            if(this->isDeviceSuitable(device)){
                this->physicalDevice=device;
                break;
            }
        }
        if(this->physicalDevice==VK_NULL_HANDLE) throw std::runtime_error("Failed to find suitable GPU");
        vkGetPhysicalDeviceProperties(this->physicalDevice, &this->properties);
        std::cout << "Physical Device -> "<< this->properties.deviceName << std::endl;
    }

    void LiveDevice::createLogicalDevice(){
        QueueFamilyIndices indices = this->findQueueFamilies(this->physicalDevice);
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueFamilyIndices = {indices.graphicsFamily, indices.presentFamily};
        float queuePriority = 1.0f;
        for(uint32_t queueFamily:uniqueFamilyIndices){
            queueCreateInfos.push_back(this->buildQueueCreateInfo(queueFamily, &queuePriority));
        }
        VkPhysicalDeviceFeatures deviceFeatures = this->buildDeviceFeatures();
        VkDeviceCreateInfo createInfo = this->buildBaseDeviceCreateInfo(static_cast<uint32_t>(queueCreateInfos.size()), queueCreateInfos.data(), &deviceFeatures, static_cast<uint32_t>(this->deviceExtensions.size()), this->deviceExtensions.data());
        
        if(this->enableValidationLayers){
            createInfo.enabledLayerCount = static_cast<uint32_t>(this->validationLayers.size());
            createInfo.ppEnabledLayerNames = this->validationLayers.data();
        }else {
            createInfo.enabledLayerCount = 0;
        }
        
        bool isCreateDeviceSuccess = vkCreateDevice(this->physicalDevice,&createInfo, nullptr, &this->device_) == VK_SUCCESS;
        if(!isCreateDeviceSuccess) throw std::runtime_error("Failed to create logical device!");
        
        vkGetDeviceQueue(this->device_, indices.graphicsFamily, 0, &this->graphicsQueue_);
        vkGetDeviceQueue(this->device_, indices.presentFamily, 0, &this->presentQueue_);
    }

    void LiveDevice::createCommandPool(){
        QueueFamilyIndices queueFamilyIndices = this->findPhysicalQueueFamilies();
        VkCommandPoolCreateInfo poolInfo = this->buildCommandPoolCreateInfo(queueFamilyIndices.graphicsFamily);
        bool isCreateCommandPoolSuccess = vkCreateCommandPool(this->device_, &poolInfo, nullptr, &this->commandPool);
        if(!isCreateCommandPoolSuccess) throw std::runtime_error("Failed to create command pool!");
    }

    VkCommandPoolCreateInfo LiveDevice::buildCommandPoolCreateInfo(uint32_t queueFamilyIndex){
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndex;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        return poolInfo;
    }

    VkDeviceQueueCreateInfo LiveDevice::buildQueueCreateInfo(uint32_t queueFamilyIndex, float *queuePriority){
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = queuePriority;
        return queueCreateInfo;
    }

    VkApplicationInfo LiveDevice::buildApplicationInfo(){
        VkApplicationInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        info.pApplicationName = "GameEngine App";
        info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        info.pEngineName = "No Engine";
        info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        info.apiVersion = VK_API_VERSION_1_0;
        return info;
    }

    VkPhysicalDeviceFeatures LiveDevice::buildDeviceFeatures(){
        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        return deviceFeatures;
    }

    VkDeviceCreateInfo LiveDevice::buildBaseDeviceCreateInfo(uint32_t queueCreateInfoCount, const VkDeviceQueueCreateInfo *pQueueCreateInfos, const VkPhysicalDeviceFeatures *enabledFeatures, uint32_t enabledExtensionCount, const char *const *ppEnabledExtensionNames){
        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        
        createInfo.queueCreateInfoCount = queueCreateInfoCount;
        createInfo.pQueueCreateInfos = pQueueCreateInfos;
        
        createInfo.pEnabledFeatures = enabledFeatures;
        createInfo.enabledExtensionCount = enabledExtensionCount;
        createInfo.ppEnabledExtensionNames = ppEnabledExtensionNames;
        return createInfo;
    }

    VkInstanceCreateInfo LiveDevice::buildInstanceCreateInfo(const VkApplicationInfo* appInfo){
        auto extension = this->getRequiredExtensions();
        VkInstanceCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        info.pApplicationInfo = appInfo;
        info.enabledExtensionCount = static_cast<uint32_t>(extension.size());
        info.ppEnabledExtensionNames = extension.data();
        
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        if(this->enableValidationLayers){
            info.enabledLayerCount = static_cast<uint32_t>(this->validationLayers.size());
            info.ppEnabledLayerNames = this->validationLayers.data();
            this->populateDebugMessengerCreateInfo(debugCreateInfo);
            info.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
        }else {
            info.enabledLayerCount =0;
            info.pNext = nullptr;
        }
        return info;
    }

    bool LiveDevice::isDeviceSuitable(VkPhysicalDevice device){
        QueueFamilyIndices indices = this->findQueueFamilies(device);
        bool isExtensionSupported = this->checkDeviceExtensionSupport(device);
        bool isSwapChainAdequate = false;
        if(isExtensionSupported){
            SwapChainSupportDetails swapChainSupport = this->querySwapChainSupport(device);
            isSwapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
        return indices.isComplete() && isExtensionSupported && isSwapChainAdequate && supportedFeatures.samplerAnisotropy;
    }

    QueueFamilyIndices LiveDevice::findQueueFamilies(VkPhysicalDevice device){
        QueueFamilyIndices indices;
        uint32_t queuFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queuFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queuFamilyCount);
        
        int i = 0;
        
        for(const VkQueueFamilyProperties queueFamily:queueFamilies){
            if(queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT){
                indices.graphicsFamily = i;
                indices.graphicFamilyHasValue = true;
            }
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, this->surface_, &presentSupport);
            if(queueFamily.queueCount > 0 && presentSupport){
                indices.presentFamily = i;
                indices.presentFamilyHasValue = true;
            }
            
            if(indices.isComplete()) break;
            i++;
        }
        
        return indices;
    }

    bool LiveDevice::checkDeviceExtensionSupport(VkPhysicalDevice device){
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
        std::set<std::string> requiredExtension(this->deviceExtensions.begin(), this->deviceExtensions.end());
        for(const VkExtensionProperties &extension:availableExtensions){
            requiredExtension.insert(extension.extensionName);
        }
        return requiredExtension.empty();
    }

    SwapChainSupportDetails LiveDevice::querySwapChainSupport(VkPhysicalDevice device){
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, this->surface_, &details.capabilities);
        
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->surface_, &formatCount, nullptr);
        
        if(formatCount != 0 ){
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->surface_, &formatCount, details.formats.data());
        }
        
        uint32_t presentModeCount =0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->surface_, &presentModeCount, nullptr);
        
        if(presentModeCount != 0){
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->surface_, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    void LiveDevice::validateGLfwRequiredInstanceExtensions(){
        uint32_t count =0;
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
        std::vector<VkExtensionProperties> extensions(count);
        vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());
        std::unordered_set<std::string> availableExtensions;
        
        std::cout << "Available extensions:" << std::endl;
        for(const auto &extension: extensions){
            std::cout << "\t->" << extension.extensionName << std::endl;
            availableExtensions.insert(extension.extensionName);
        }
        
        std::vector<const char *> requiredExtensions = this->getRequiredExtensions();
        
        std::cout << "Required extensions:" << std::endl;
        for(const auto &requiredExtension:requiredExtensions){
            std::cout << "\t->"<< requiredExtension << std::endl;
            bool isRequiredExtensionFound = availableExtensions.find(requiredExtension) != availableExtensions.end();
            if(!isRequiredExtensionFound) throw std::runtime_error("Missing required GLFW extension");
        }
    }

    void LiveDevice::populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT 
            |VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT 
            |VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT 
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;  // Optional
    }

    std::vector<const char *> LiveDevice::getRequiredExtensions(){
        uint32_t glfwExtensionsCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
        // specify extensions into the vector
        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionsCount);
        if(this->enableValidationLayers) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        return extensions;
    }

    bool LiveDevice::checkValidationLayerSupport(){
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        for(const char* validationLayer:this->validationLayers){
            bool isLayerFound = false;
            for(const VkLayerProperties &availableLayer:availableLayers){
                if(strcmp(validationLayer, availableLayer.layerName)==0){
                    isLayerFound = true;
                    break;
                }
            }
            if(!isLayerFound) return false;
        }
        return true;
    }
}
