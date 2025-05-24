#include "engine_swap_chain.hpp"

// std
#include <iostream>
#include <array>

namespace engine {
  // Publics
  EngineSwapChain::EngineSwapChain(EngineDevice &deviceReference, VkExtent2D windowExtent): device{deviceReference}, windowExtent{windowExtent}{
    std::cout << "EngineSwapChain: Initialising engine swap chain" << std::endl;
    this->createSwapChain();
    this->createImageViews();
    this->createRenderPass();
    this->createDepthResources();
    this->createFramebuffers();
    this->createSyncObjects();
    std::cout << "EngineSwapChain: Successfully initialise engine swap chain" << std::endl;
  }

  EngineSwapChain::~EngineSwapChain(){
    for(auto imageView: this->swapChainImageViews){
      vkDestroyImageView(this->device.device(), imageView, nullptr);
    }
    this->swapChainImageViews.clear();
    if(this->swapChain != nullptr){
      vkDestroySwapchainKHR(this->device.device(), this->swapChain, nullptr);
      this->swapChain = nullptr;
    }

    for(int i = 0; i < this->depthImages.size(); i++){
      vkDestroyImageView(this->device.device(), this->depthImageViews[i], nullptr);
      vkDestroyImage(this->device.device(), this->depthImages[i], nullptr);
      vkFreeMemory(this->device.device(), this->depthImageMemories[i], nullptr);
    }

    for(auto frameBuffer:this->swapChainFramebuffers){
      vkDestroyFramebuffer(this->device.device(), frameBuffer, nullptr);
    }

    vkDestroyRenderPass(this->device.device(), this->renderPass, nullptr);

    // clean up synchronisation objects
    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
      vkDestroySemaphore(this->device.device(), this->renderedImageSemaphores[i], nullptr);
      vkDestroySemaphore(this->device.device(), this->imageAvailableSemaphores[i], nullptr);
      vkDestroyFence(this->device.device(), this->inFlightFences[i], nullptr);
    }
  }

  VkFormat EngineSwapChain::findDepthFormat(){
    return this->device.findSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
  }

  VkResult EngineSwapChain::acquireNextImage(uint32_t *imageIndex){
    vkWaitForFences(
      this->device.device(),
      1, 
      &this->inFlightFences[this->currentFrame],
      VK_TRUE,
      std::numeric_limits<uint64_t>::max()
    );
    VkResult result = vkAcquireNextImageKHR(
      this->device.device(),
      this->swapChain,
      std::numeric_limits<uint64_t>::max(),
      this->imageAvailableSemaphores[this->currentFrame],
      VK_NULL_HANDLE,
      imageIndex
    );
    return result;
  }

  VkResult EngineSwapChain::submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex){
      bool hasImageInFlight = this->imagesInFlight[*imageIndex] != VK_NULL_HANDLE;
      if(hasImageInFlight) vkWaitForFences(this->device.device(), 1, &this->imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
      this->imagesInFlight[*imageIndex] = this->inFlightFences[this->currentFrame];

      VkSemaphore waitSemaphores[] = {this->imageAvailableSemaphores[this->currentFrame]};
      VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
      VkSemaphore signalSemaphores[] = {this->renderedImageSemaphores[this->currentFrame]};
      VkSubmitInfo submitInfo = this->buildSubmitInfo(waitSemaphores, waitStages, signalSemaphores);
      vkResetFences(this->device.device(), 1, &this->inFlightFences[this->currentFrame]);
      bool isSubmitQueueSuccess = vkQueueSubmit(this->device.graphicsQueue(), 1, &submitInfo, this->inFlightFences[this->currentFrame]) == VK_SUCCESS;
      if(!isSubmitQueueSuccess) throw std::runtime_error("Failed to submit draw command buffer to job!");

      VkSwapchainKHR swapChains[] = {this->swapChain};
      VkPresentInfoKHR presentInfo = this->buildPresentInfoKHR(signalSemaphores, swapChains, imageIndex);
      auto result = vkQueuePresentKHR(this->device.presentQueue(), &presentInfo);

      this->currentFrame = (this->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
      return result;
  }

  //Privates
  void EngineSwapChain::createSwapChain(){
    std::cout << "\t -> createSwapChain(): Creating swap chain" << std::endl;
    
    SwapChainSupportDetails swapChainSupportDetails = this->device.getSwapChainSupportDetails();
    VkSurfaceFormatKHR surfaceFormat = this->chooseSwapSurfaceFormat(swapChainSupportDetails.formats);
    VkPresentModeKHR presentMode = this->chooseSwapPresentMode(swapChainSupportDetails.presentModes);
    VkExtent2D extent = this->chooseSwapExtent(swapChainSupportDetails.capabilities);

    uint32_t imageCount = swapChainSupportDetails.capabilities.minImageCount + 1;
    bool isExceedingMaxImageCount = swapChainSupportDetails.capabilities.maxImageCount > 0 && imageCount > swapChainSupportDetails.capabilities.maxImageCount;
    if(isExceedingMaxImageCount) imageCount = swapChainSupportDetails.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR swapChainCreateInfo = this->buildSwapchainCreateInfo(
      imageCount,
      surfaceFormat.format,
      surfaceFormat.colorSpace,
      extent
    );

    QueueFamilyIndices indices = this->device.findPhysicalQueueFamilies();
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

    if(indices.graphicsFamily != indices.presentFamily){
      swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      swapChainCreateInfo.queueFamilyIndexCount = 2;
      swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }else {
      swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      swapChainCreateInfo.queueFamilyIndexCount = 0; //Optional
      swapChainCreateInfo.pQueueFamilyIndices = nullptr; //Optional
    }
    swapChainCreateInfo.preTransform = swapChainSupportDetails.capabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.clipped = VK_TRUE;

    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    bool isCreateSwapChainSuccess = vkCreateSwapchainKHR(this->device.device(), &swapChainCreateInfo, nullptr, &this->swapChain) == VK_SUCCESS;
    if(!isCreateSwapChainSuccess) throw std::runtime_error("Failed to create swap chain!");

    vkGetSwapchainImagesKHR(this->device.device(), this->swapChain, &imageCount, nullptr);
    this->swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(this->device.device(), this->swapChain, &imageCount, this->swapChainImages.data());
    this->swapChainImageFormat = surfaceFormat.format;
    this->swapChainExtent = extent;

    std::cout << "\t -> createSwapChain(): Successfully create swap chain" << std::endl;
  }

  void EngineSwapChain::createImageViews(){
    std::cout << "\t -> createImageViews(): Creating image views" << std::endl;
    this->swapChainImageViews.resize(this->swapChainImages.size());
    for(size_t i = 0; i < this->swapChainImages.size(); i++){
      VkImageViewCreateInfo imageViewCreateInfo = this->buildImageViewCreateInfo(this->swapChainImages[i], this->swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
      bool isCreateImageViewSuccess = vkCreateImageView(this->device.device(), &imageViewCreateInfo, nullptr, &this->swapChainImageViews[i]) == VK_SUCCESS;
      if(!isCreateImageViewSuccess) throw std::runtime_error("Failed to create texture image view!");
    }
    std::cout << "\t -> createImageViews(): Successfully create image views" << std::endl;
  }

  void EngineSwapChain::createDepthResources(){
    std::cout << "\t -> createDepthResources(): Creating depth resources" << std::endl;

    VkFormat depthFormat = this->findDepthFormat();
    VkExtent2D swapChainExtent = this->getSwapChainExtent();
    
    this->depthImages.resize(this->imageCount());
    this->depthImageMemories.resize(this->imageCount());
    this->depthImageViews.resize(this->imageCount());
    for(int i = 0; i < this->depthImages.size(); i++){
      VkImageCreateInfo imageCreateInfo = this->buildImageCreateInfo(depthFormat);
      this->device.createImageWithInfo(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->depthImages[i], this->depthImageMemories[i]);
      VkImageViewCreateInfo imageViewCreateInfo = this->buildImageViewCreateInfo(this->depthImages[i], depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
      bool isCreateImageViewSuccess = vkCreateImageView(this->device.device(), &imageViewCreateInfo, nullptr, &this->depthImageViews[i]) == VK_SUCCESS;
      if(!isCreateImageViewSuccess) throw std::runtime_error("Failed to create image");
    }

    std::cout << "\t -> createDepthResources(): Successfully create depth resources" << std::endl;
  }

  void EngineSwapChain::createRenderPass(){
    std::cout << "\t -> createRenderPass(): Creating render pass" << std::endl;

    VkAttachmentDescription depthAttachmentDescription = {};
    depthAttachmentDescription.format = this->findDepthFormat();
    depthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    VkAttachmentReference depthAttachmentReference = {};
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentDescription = {};
    colorAttachmentDescription.format = getSwapChainImageFormat();
    colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;



    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentReference;
    subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;

    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpassDependency.dstSubpass = 0;
    subpassDependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpassDependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachmentDescription, depthAttachmentDescription};
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassCreateInfo.pAttachments = attachments.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;

    bool isCreateRenderPassSuccess = vkCreateRenderPass(this->device.device(), &renderPassCreateInfo, nullptr, &this->renderPass) == VK_SUCCESS;
    if(!isCreateRenderPassSuccess) throw std::runtime_error("Failed to create render pass!");

    std::cout << "\t -> createRenderPass(): Successfully create render pass" << std::endl;
  }

  void EngineSwapChain::createFramebuffers(){
    std::cout << "\t -> createFramebuffers(): Creating frame buffers" << std::endl;

    this->swapChainFramebuffers.resize(this->imageCount());
    for(size_t i = 0; i < this->imageCount(); i ++){
      std::array<VkImageView, 2> attachments = {this->swapChainImageViews[i], this->depthImageViews[i]};
      VkExtent2D swapChainExtent = this->getSwapChainExtent();

      VkFramebufferCreateInfo framebufferCreateInfo = this->buildFrameBufferCreateInfo(
        this->renderPass,
        static_cast<uint32_t>(attachments.size()),
        attachments.data(),
        swapChainExtent.width,
        swapChainExtent.height
      );

      bool isCreateFrameBufferSuccess = vkCreateFramebuffer(this->device.device(), &framebufferCreateInfo, nullptr, &this->swapChainFramebuffers[i]) == VK_SUCCESS;
      if(!isCreateFrameBufferSuccess) throw std::runtime_error("Failed to create frame buffer!");
    }

    std::cout << "\t -> createFramebuffers(): Successfully create frame buffers" << std::endl;
  }

  void EngineSwapChain::createSyncObjects(){
    std::cout << "\t -> createSyncObjects(): Creating sync objects" << std::endl;

    this->imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    this->renderedImageSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    this->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    this->imagesInFlight.resize(this->imageCount(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(size_t i =0; i<MAX_FRAMES_IN_FLIGHT; i++){
      bool isCreateImageAvailableSemaphoreSuccess = vkCreateSemaphore(this->device.device(), &semaphoreCreateInfo, nullptr, &this->imageAvailableSemaphores[i]) == VK_SUCCESS;
      bool isCreateRenderedImageSemaphoreSuccess = vkCreateSemaphore(this->device.device(), &semaphoreCreateInfo, nullptr, &this->renderedImageSemaphores[i]) == VK_SUCCESS;
      bool isCreateInFlightFenceSuccess = vkCreateFence(this->device.device(), &fenceCreateInfo, nullptr, &this->inFlightFences[i]) == VK_SUCCESS;

      if(!isCreateImageAvailableSemaphoreSuccess || !isCreateRenderedImageSemaphoreSuccess || !isCreateInFlightFenceSuccess)
        throw std::runtime_error("Failed to create synchronisation objects for a frame!");
    }

    std::cout << "\t -> createSyncObjects(): Successfully create sync objects" << std::endl;

  }

  VkFramebufferCreateInfo EngineSwapChain::buildFrameBufferCreateInfo(VkRenderPass renderPass, uint32_t attachmentCount, const VkImageView* pAttachments, uint32_t width, uint32_t height){
    VkFramebufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = renderPass;
    info.attachmentCount = attachmentCount;
    info.pAttachments = pAttachments;
    info.width = width;
    info.height = height;
    info.layers = 1;
    return info;
  }

  VkPresentInfoKHR EngineSwapChain::buildPresentInfoKHR(const VkSemaphore* pWaitSemaphores, const VkSwapchainKHR* pSwapChains, const uint32_t* pImageIndices){
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = pWaitSemaphores;
    info.swapchainCount = 1;
    info.pSwapchains = pSwapChains;
  
    info.pImageIndices = pImageIndices;
    return info;
  }

  VkSubmitInfo EngineSwapChain::buildSubmitInfo(VkSemaphore* pWaitSemaphores, VkPipelineStageFlags* pWaitDestStageMask, VkSemaphore* pSignalSemaphores){
    VkSubmitInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = pWaitSemaphores;
    info.pWaitDstStageMask = pWaitDestStageMask;
    info.signalSemaphoreCount = 1;
    info.pSignalSemaphores = pSignalSemaphores;
    return info;
  }

  VkImageCreateInfo EngineSwapChain::buildImageCreateInfo(VkFormat format){
    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.extent.width = swapChainExtent.width;
    info.extent.height = swapChainExtent.height;
    info.extent.depth = 1;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.format = format;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.flags = 0;
    return info;
  }

  VkImageViewCreateInfo EngineSwapChain::buildImageViewCreateInfo(VkImage image, VkFormat format, VkImageAspectFlags aspectMask){
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = format;
    info.subresourceRange.aspectMask = aspectMask;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    return info;
  }

  VkSwapchainCreateInfoKHR EngineSwapChain::buildSwapchainCreateInfo(uint32_t minImageCount, VkFormat imageFormat, VkColorSpaceKHR imageColorSpace, VkExtent2D extent){
    VkSwapchainCreateInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = this->device.surface();

    info.minImageCount = minImageCount;
    info.imageFormat = imageFormat;
    info.imageColorSpace = imageColorSpace;
    info.imageExtent = extent;

    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    return info;
  }


  VkSurfaceFormatKHR EngineSwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats){
    for(const auto &availableFormat:availableFormats){
      if(availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ) 
        return availableFormat;
    }
    return availableFormats[0];
  }

  VkPresentModeKHR EngineSwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes){
    for(const auto &availablePresentMode: availablePresentModes){
      if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR){
        std::cout << "\t\t -> Present mode: Mailbox" << std::endl;
        return availablePresentMode;
      }

      if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
        std::cout << "\t\t -> Present mode: Immediate" << std::endl;
        return availablePresentMode;
      }
    }

    std::cout << "\t\t -> Present mode: V-sync" << std::endl;
    return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D EngineSwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities){
    if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) return capabilities.currentExtent;
    VkExtent2D actualExtent = this->windowExtent;
    actualExtent.width = std::max(
      capabilities.currentExtent.width, 
      std::min(capabilities.maxImageExtent.width, actualExtent.width)
    );
    actualExtent.height = std::max(
      capabilities.currentExtent.height, 
      std::min(capabilities.maxImageExtent.height, actualExtent.height)
    );
    return actualExtent;
  }
}           