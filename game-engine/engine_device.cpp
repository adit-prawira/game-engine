#include "engine_device.hpp"

// std
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <set>

namespace engine {
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

    // Publics
    EngineDevice::EngineDevice(EngineWindow &window): window{window}{
        std::cout << "EngineDevice: Initialising engine device" << std::endl;
        this->createInstance();
        this->setupDebugMessenger();
        this->createSurface();
        this->pickPhysicalDevice();
        this->createLogicalDevice();
        this->createCommandPool();
        std::cout << "EngineDevice: Successfully initialise engine device" << std::endl;
    }

    EngineDevice::~EngineDevice(){
        vkDestroyCommandPool(this->device_, this->commandPool, nullptr);
        vkDestroyDevice(this->device_, nullptr);
        if(this->enableValidationLayers){
            DestroyDebugUtilsMessengerEXT(this->instance, this->debugMessenger, nullptr);
        }
        vkDestroySurfaceKHR(this->instance, this->surface_, nullptr);
        vkDestroyInstance(this->instance, nullptr);
    }

    uint32_t EngineDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties){
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

    VkFormat EngineDevice::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features){
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
    
    void EngineDevice::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory){
        VkBufferCreateInfo bufferInfo = this->buildBufferCreateInfo(size, usage);
        bool isCreateBufferSuccess = vkCreateBuffer(this->device_, &bufferInfo, nullptr, &buffer) == VK_SUCCESS;
        if(!isCreateBufferSuccess) throw std::runtime_error("Failed to create buffer!");

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(this->device_, buffer, &memoryRequirements);
        VkMemoryAllocateInfo memoryAllocateInfo = this->buildMemoryAllocateInfo(
            memoryRequirements.size, 
            this->findMemoryType(memoryRequirements.memoryTypeBits, properties)
        );
        bool isCreateMemoryAllocateSuccess = vkAllocateMemory(this->device_, &memoryAllocateInfo, nullptr, &bufferMemory) == VK_SUCCESS;
        if(!isCreateMemoryAllocateSuccess) throw std::runtime_error("Failed to allocate memory!");
        
        vkBindBufferMemory(this->device_, buffer, bufferMemory, 0);
    }

    VkCommandBuffer EngineDevice::beginSingleTimeCommands(){
        VkCommandBufferAllocateInfo commandBufferAllocateInfo = this->buildCommandBufferAllocateInfo(this->commandPool, 1);
        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(this->device_, &commandBufferAllocateInfo, &commandBuffer);

        VkCommandBufferBeginInfo commandBufferBeginInfo = this->buildCommandBufferBeginInfo();
        vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

        return commandBuffer;
    }

    void EngineDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer){
        vkEndCommandBuffer(commandBuffer);
        VkSubmitInfo submitInfo = this->buildSubmitInfo(1, &commandBuffer);
        vkQueueSubmit(this->graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(this->graphicsQueue_);

        vkFreeCommandBuffers(this->device_, this->commandPool, 1, &commandBuffer);
    }

    void EngineDevice::copyBuffer(VkBuffer sourceBuffer, VkBuffer destinationBuffer, VkDeviceSize size){
        VkCommandBuffer commandBuffer =this->beginSingleTimeCommands();
        VkBufferCopy bufferCopyRegions = {};
        bufferCopyRegions.srcOffset = 0;
        bufferCopyRegions.dstOffset = 0;
        bufferCopyRegions.size = size;

        vkCmdCopyBuffer(commandBuffer, sourceBuffer, destinationBuffer, 1, &bufferCopyRegions);
        this->endSingleTimeCommands(commandBuffer);
    }

    void EngineDevice::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount){
        VkCommandBuffer commandBuffer = this->beginSingleTimeCommands();
        VkBufferImageCopy bufferImageCopyRegions = {};
        bufferImageCopyRegions.bufferOffset = 0;
        bufferImageCopyRegions.bufferRowLength = 0;
        bufferImageCopyRegions.bufferImageHeight = 0;

        bufferImageCopyRegions.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferImageCopyRegions.imageSubresource.mipLevel = 0;
        bufferImageCopyRegions.imageSubresource.baseArrayLayer = 0;
        bufferImageCopyRegions.imageSubresource.layerCount = layerCount;

        bufferImageCopyRegions.imageOffset = {0,0,0};
        bufferImageCopyRegions.imageExtent={width, height, 1};

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopyRegions);
        this->endSingleTimeCommands(commandBuffer);
    }

    void EngineDevice::createImageWithInfo(const VkImageCreateInfo &imageInfo, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory){
        bool isCreateImageSuccess = vkCreateImage(this->device_, &imageInfo, nullptr, &image) == VK_SUCCESS;
        if(!isCreateImageSuccess) throw std::runtime_error("Failed to create image!");

        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(this->device_, image, &memoryRequirements);

        VkMemoryAllocateInfo memoryAllocateInfo = this->buildMemoryAllocateInfo(
            memoryRequirements.size,
            this->findMemoryType(memoryRequirements.memoryTypeBits, properties)
        );

        bool isAllocateMemorySuccess = vkAllocateMemory(this->device_, &memoryAllocateInfo, nullptr, &imageMemory) == VK_SUCCESS;
        if(!isAllocateMemorySuccess) throw std::runtime_error("Failed to allocate memory!");
        
        bool isBindImageMemorySuccess = vkBindImageMemory(this->device_, image, imageMemory,0) == VK_SUCCESS;
        if(!isBindImageMemorySuccess) throw std::runtime_error("Failed to bind image memory!");
    }

    // Privates
    void EngineDevice::createInstance(){
        std::cout << "\t -> createInstance(): Creating instance" << std::endl;

        if(this->enableValidationLayers && !this->checkValidationLayerSupport()) throw std::runtime_error("Validation layers requested, but not available");

        VkApplicationInfo appInfo = this->buildApplicationInfo();
        VkInstanceCreateInfo createInfo = this->buildInstanceCreateInfo(&appInfo);

        std::vector<const char *> extensions = this->getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        
            this->populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }


        bool isCreateInstanceSuccess = vkCreateInstance(&createInfo, nullptr, &this->instance) == VK_SUCCESS;

        if(!isCreateInstanceSuccess) throw std::runtime_error("Failed to create instance");
        this->validateGLfwRequiredInstanceExtensions();
        std::cout << "\t -> createInstance(): Successfully create instance" << std::endl;
    }

    void EngineDevice::setupDebugMessenger(){
        std::cout << "\t -> setupDebugMessenger(): Setting up debug messenger" << std::endl;

        if(!this->enableValidationLayers) return;
        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        this->populateDebugMessengerCreateInfo(createInfo);
        bool isSuccess = CreateDebugUtilsMessengerEXT(this->instance, &createInfo, nullptr, &this->debugMessenger) == VK_SUCCESS;
        if(!isSuccess) throw std::runtime_error("Failed to setup debugger messenger!");

        std::cout << "\t -> setupDebugMessenger(): Successfully set up debug messenger" << std::endl;
    }

    void EngineDevice::createSurface(){
        std::cout << "\t -> createSurface(): Creating surface" << std::endl;
        window.createWindowSurface(this->instance, &this->surface_);
        std::cout << "\t -> createSurface(): Successfully create surface" << std::endl;
    }

    void EngineDevice::pickPhysicalDevice(){
        std::cout << "\t -> pickPhysicalDevice(): Picking physical device" << std::endl;
        uint32_t deviceCount =0;
        vkEnumeratePhysicalDevices(this->instance, &deviceCount, nullptr);
        if(deviceCount ==0) throw std::runtime_error("Failed to find GPU with Vulkan support!");
        std::cout << "\t\t -> Total GPU devices available -> " << deviceCount << std::endl;
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
        std::cout << "\t -> pickPhysicalDevice(): Successfully pick physical device => " << this->properties.deviceName << std::endl;
    }

    void EngineDevice::createLogicalDevice(){
        std::cout << "\t -> createLogicalDevice(): Creating logical device" << std::endl;
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
        std::cout << "\t -> createLogicalDevice(): Successfully create logical device" << std::endl;
    }

    void EngineDevice::createCommandPool(){
        std::cout << "\t -> createCommandPool(): Creating command pool" << std::endl;

        QueueFamilyIndices queueFamilyIndices = this->findPhysicalQueueFamilies();
        VkCommandPoolCreateInfo poolInfo = this->buildCommandPoolCreateInfo(queueFamilyIndices.graphicsFamily);
        bool isCreateCommandPoolSuccess = vkCreateCommandPool(this->device_, &poolInfo, nullptr, &this->commandPool) == VK_SUCCESS;
        if(!isCreateCommandPoolSuccess) throw std::runtime_error("Failed to create command pool!");
        std::cout << "\t -> createCommandPool(): Successfully create command pool" << std::endl;
    }

    VkSubmitInfo EngineDevice::buildSubmitInfo(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffer){
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.commandBufferCount = commandBufferCount;
        info.pCommandBuffers = pCommandBuffer;
        return info;
    }

    VkCommandBufferBeginInfo EngineDevice::buildCommandBufferBeginInfo(){
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        return info;
    }
    VkCommandBufferAllocateInfo EngineDevice::buildCommandBufferAllocateInfo(VkCommandPool commandPool, uint32_t commandBufferCount){
        VkCommandBufferAllocateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandPool = commandPool;
        info.commandBufferCount = commandBufferCount;
        return info;
    }
    VkMemoryAllocateInfo EngineDevice::buildMemoryAllocateInfo(VkDeviceSize size, uint32_t memoryTypeIndex){
        VkMemoryAllocateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        info.allocationSize = size;
        info.memoryTypeIndex  = memoryTypeIndex;
        return info;
    }


    VkBufferCreateInfo EngineDevice::buildBufferCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage){
        VkBufferCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size = size;
        info.usage = usage;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        return info;
    }

    VkCommandPoolCreateInfo EngineDevice::buildCommandPoolCreateInfo(uint32_t queueFamilyIndex){
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndex;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        return poolInfo;
    }

    VkDeviceQueueCreateInfo EngineDevice::buildQueueCreateInfo(uint32_t queueFamilyIndex, float *queuePriority){
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = queuePriority;
        return queueCreateInfo;
    }

    VkApplicationInfo EngineDevice::buildApplicationInfo(){
        VkApplicationInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        info.pApplicationName = "GameEngine App";
        info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        info.pEngineName = "No Engine";
        info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        info.apiVersion = VK_API_VERSION_1_0;
        return info;
    }

    VkPhysicalDeviceFeatures EngineDevice::buildDeviceFeatures(){
        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        return deviceFeatures;
    }

    VkDeviceCreateInfo EngineDevice::buildBaseDeviceCreateInfo(uint32_t queueCreateInfoCount, const VkDeviceQueueCreateInfo *pQueueCreateInfos, const VkPhysicalDeviceFeatures *enabledFeatures, uint32_t enabledExtensionCount, const char *const *ppEnabledExtensionNames){
        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        
        createInfo.queueCreateInfoCount = queueCreateInfoCount;
        createInfo.pQueueCreateInfos = pQueueCreateInfos;
        
        createInfo.pEnabledFeatures = enabledFeatures;
        createInfo.enabledExtensionCount = enabledExtensionCount;
        createInfo.ppEnabledExtensionNames = ppEnabledExtensionNames;
        return createInfo;
    }

    VkInstanceCreateInfo EngineDevice::buildInstanceCreateInfo(const VkApplicationInfo* appInfo){
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = appInfo;
        createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        return createInfo;
    }
                                                
    bool EngineDevice::isDeviceSuitable(VkPhysicalDevice device){
        std::cout << "\t\t -> Attempting to check if device is suitable" << std::endl;
        QueueFamilyIndices indices = this->findQueueFamilies(device);
        bool isExtensionSupported = this->checkDeviceExtensionSupport(device);
        bool isSwapChainAdequate = false;
        if(isExtensionSupported){
            SwapChainSupportDetails swapChainSupport = this->querySwapChainSupport(device);
            isSwapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
        std::cout << "\t\t -> Indices Completed -> " << indices.isComplete() << std::endl;
        std::cout << "\t\t -> Extension is supported -> " << isExtensionSupported << std::endl;
        std::cout << "\t\t -> Swap chain is adequate -> " << isSwapChainAdequate << std::endl;
        std::cout << "\t\t -> Sampler Anisotropy -> " << supportedFeatures.samplerAnisotropy << std::endl;
        bool isSuitable = indices.isComplete() && isExtensionSupported && isSwapChainAdequate && supportedFeatures.samplerAnisotropy;
        std::cout << "\t\t -> Is Device Suitable -> " << isSuitable << std::endl;
        return isSuitable;
    }

    QueueFamilyIndices EngineDevice::findQueueFamilies(VkPhysicalDevice device){
        QueueFamilyIndices indices;
        uint32_t queueFamilyCount = 0;
        
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        
        for(const auto &queueFamily : queueFamilies){
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


    bool EngineDevice::checkDeviceExtensionSupport(VkPhysicalDevice device){
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
        std::set<std::string> requiredExtension(this->deviceExtensions.begin(), this->deviceExtensions.end());
        for(const VkExtensionProperties &extension:availableExtensions){
            requiredExtension.erase(extension.extensionName);
        }
        return requiredExtension.empty();
    }

    SwapChainSupportDetails EngineDevice::querySwapChainSupport(VkPhysicalDevice device){
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

    void EngineDevice::validateGLfwRequiredInstanceExtensions(){
        uint32_t count =0;
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
        std::vector<VkExtensionProperties> extensions(count);
        vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());
        std::unordered_set<std::string> availableExtensions;
        
        std::cout << "\t\tAvailable extensions:" << std::endl;
        for(const auto &extension: extensions){
            std::cout << "\t\t\t-> " << extension.extensionName << std::endl;
            availableExtensions.insert(extension.extensionName);
        }
        
        std::vector<const char *> requiredExtensions = this->getRequiredExtensions();
        
        std::cout << "\t\tRequired extensions:" << std::endl;
        for(const auto &requiredExtension:requiredExtensions){
            std::cout << "\t\t\t-> "<< requiredExtension << std::endl;
            bool isRequiredExtensionFound = availableExtensions.find(requiredExtension) != availableExtensions.end();
            if(!isRequiredExtensionFound) throw std::runtime_error("Missing required GLFW extension");
        }
    }

    void EngineDevice::populateDebugMessengerCreateInfo(
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

    std::vector<const char *> EngineDevice::getRequiredExtensions(){
        uint32_t glfwExtensionsCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
        // specify extensions into the vector
        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionsCount);
        if(this->enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            extensions.push_back("VK_KHR_portability_enumeration");
            extensions.push_back("VK_KHR_get_physical_device_properties2");
        }
        return extensions;
    }

    bool EngineDevice::checkValidationLayerSupport(){
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
