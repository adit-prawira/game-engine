#pragma once

#include "engine_device.hpp"

// std
#include <string>
#include <vector>

namespace engine {
  class EngineSwapChain {
    public: 
      static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
      EngineSwapChain(EngineDevice &deviceReference, VkExtent2D windowExtent);
      ~EngineSwapChain();

      EngineSwapChain(const EngineSwapChain &) = delete;
      void operator = (const EngineSwapChain &) = delete;

      VkFramebuffer getFrameBuffer(int index){
        return this->swapChainFramebuffers[index];
      }

      VkRenderPass getRenderPass(){
        return this->renderPass;
      }
      VkImageView getImageView(int index){
        return this->swapChainImageViews[index];
      }
      size_t imageCount(){
        return this->swapChainImages.size();
      }
      VkFormat getSwapChainImageFormat(){
        return this->swapChainImageFormat;
      }
      VkExtent2D getSwapChainExtent(){
        return this->swapChainExtent;
      }
      uint32_t width(){
        return this->swapChainExtent.width;
      }
      uint32_t height(){
        return this->swapChainExtent.height;
      }
      float extentAspectRatio(){
        float ratio = static_cast<float>(this->swapChainExtent.width)/static_cast<float>(this->swapChainExtent.height);
        return ratio;
      }
      VkFormat findDepthFormat();
      VkResult acquireNextImage(uint32_t *imageIndex);
      VkResult submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex);
    private:
      void createSwapChain();
      void createImageViews();
      void createDepthResources();
      void createRenderPass();
      void createFramebuffers();
      void createSyncObjects();

      // Builders
      VkSwapchainCreateInfoKHR buildSwapchainCreateInfo(uint32_t minImageCount, VkFormat imageFormat, VkColorSpaceKHR imageColorSpace, VkExtent2D extent);
      VkImageViewCreateInfo buildImageViewCreateInfo(VkImage image, VkFormat format, VkImageAspectFlags aspectMask);
      VkSubmitInfo buildSubmitInfo(const VkSemaphore* pWaitSemaphores, const VkPipelineStageFlags* pWaitDestStageMask, const VkCommandBuffer* commandBuffers,  const VkSemaphore* pSignalSemaphores);
      VkPresentInfoKHR buildPresentInfoKHR(const VkSemaphore* pWaitSemaphores, const VkSwapchainKHR* pSwapChains, const uint32_t* pImageIndices);
      VkImageCreateInfo buildImageCreateInfo(VkFormat format);
      VkFramebufferCreateInfo buildFrameBufferCreateInfo(VkRenderPass renderPass, uint32_t attachmentCount, const VkImageView* pAttachments, uint32_t width, uint32_t height);
      // Utilities
      VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
      VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
      VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

      VkFormat swapChainImageFormat;
      VkExtent2D swapChainExtent;

      std::vector<VkFramebuffer> swapChainFramebuffers;
      VkRenderPass renderPass;

      std::vector<VkImage> depthImages;
      std::vector<VkDeviceMemory> depthImageMemories;
      std::vector<VkImageView> depthImageViews;
      std::vector<VkImage> swapChainImages;
      std::vector<VkImageView> swapChainImageViews;


      EngineDevice &device;
      VkExtent2D windowExtent;

      VkSwapchainKHR swapChain;

      std::vector<VkSemaphore> imageAvailableSemaphores;
      std::vector<VkSemaphore> renderedImageSemaphores;
      std::vector<VkFence> inFlightFences;
      std::vector<VkFence> imagesInFlight;
      size_t currentFrame = 0;
  };
}