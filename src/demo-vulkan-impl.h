
/* Code documentation:  https://docs.vulkan.org/tutorial/latest/00_Introduction.html */

#pragma once
#include <chrono>
#include "demo-vulkan-base-impl.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

/*#include "../common/stb_image.h"
#include "../common/tiny_obj_loader.h"
const std::string MODEL_PATH = "assets/blub_triangulated.obj";
const std::string TEXTURE_PATH = "assets/blub_texture.png";
*/

struct Vertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec3 color;
  glm::vec2 texCoord;

  static vk::VertexInputBindingDescription getBindingDescription() {
    return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
  }

  static std::array<vk::VertexInputAttributeDescription, 4>
  getAttributeDescriptions() {
    return {vk::VertexInputAttributeDescription(
                0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
            vk::VertexInputAttributeDescription(
                1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)),
            vk::VertexInputAttributeDescription(
                2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
            vk::VertexInputAttributeDescription(3, 0, vk::Format::eR32G32Sfloat,
                                                offsetof(Vertex, texCoord))};
  }

  bool operator==(const Vertex& other) const {
    return pos == other.pos && normal == other.normal && color == other.color &&
           texCoord == other.texCoord;
  }
};

template <>
struct std::hash<Vertex> {
  size_t operator()(Vertex const& vertex) const noexcept {
    return ((hash<glm::vec3>()(vertex.pos) ^
             (hash<glm::vec3>()(vertex.color) << 1)) >>
            1) ^
           (hash<glm::vec2>()(vertex.texCoord) << 1);
  }
};

struct UniformBufferObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;

 alignas(16) glm::mat4 MVP;
 alignas(16) glm::mat4 M;
 alignas(16) glm::vec3 frontDiffuse;
 alignas(16) glm::vec3 frontSpecular;
 alignas(16) glm::vec3 backDiffuse;
 alignas(16) glm::vec3 backSpecular;
 alignas(16) glm::vec3 lightPosition;
 alignas(16) glm::vec3 lightDiffuse;
 alignas(16) glm::vec3 lightSpecular;
 alignas(16) glm::vec3 eyePosition;
 alignas(16) glm::vec3 solidColor;
 alignas(4) float shininess;
};

namespace Demo {

class VulkanImpl : public VulkanBaseImpl {
  /*static VKAPI_ATTR vk::Bool32 VKAPI_CALL
  debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                vk::DebugUtilsMessageTypeFlagsEXT type,
                const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void*);
   * */
  //    static std::vector<char> readFile(const std::string &filename);
 public:
  VulkanImpl(GtkWidget* win,
             bool below_mainwindow);

  ~VulkanImpl();

  void updateModelRotation(glm::mat4& newRotation);

 private:
  std::string m_model_path;
  std::string m_texture_path;
  glm::mat4 modelRotation;
  void cleanup();
  vk::raii::PhysicalDevice physicalDevice = nullptr;
  vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
  vk::raii::Device device = nullptr;
  uint32_t queueIndex = ~0;
  vk::raii::Queue queue = nullptr;
  vk::raii::SwapchainKHR swapChain = nullptr;
  std::vector<vk::Image> swapChainImages;
  vk::SurfaceFormatKHR swapChainSurfaceFormat;
  vk::Extent2D swapChainExtent;
  std::vector<vk::raii::ImageView> swapChainImageViews;

  vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
  vk::raii::PipelineLayout pipelineLayout = nullptr;
  vk::raii::Pipeline graphicsPipeline = nullptr;

  vk::raii::Image colorImage = nullptr;
  vk::raii::DeviceMemory colorImageMemory = nullptr;
  vk::raii::ImageView colorImageView = nullptr;

  vk::raii::Image depthImage = nullptr;
  vk::raii::DeviceMemory depthImageMemory = nullptr;
  vk::raii::ImageView depthImageView = nullptr;

  uint32_t mipLevels = 0;
  vk::raii::Image textureImage = nullptr;
  vk::raii::DeviceMemory textureImageMemory = nullptr;
  vk::raii::ImageView textureImageView = nullptr;
  vk::raii::Sampler textureSampler = nullptr;

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  vk::raii::Buffer vertexBuffer = nullptr;
  vk::raii::DeviceMemory vertexBufferMemory = nullptr;
  vk::raii::Buffer indexBuffer = nullptr;
  vk::raii::DeviceMemory indexBufferMemory = nullptr;

  std::vector<vk::raii::Buffer> uniformBuffers;
  std::vector<vk::raii::DeviceMemory> uniformBuffersMemory;
  std::vector<void*> uniformBuffersMapped;

  vk::raii::DescriptorPool descriptorPool = nullptr;
  std::vector<vk::raii::DescriptorSet> descriptorSets;

  vk::raii::CommandPool commandPool = nullptr;
  std::vector<vk::raii::CommandBuffer> commandBuffers;

  std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
  std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
  std::vector<vk::raii::Fence> inFlightFences;
  uint32_t frameIndex = 0;

       std::vector<const char *> requiredDeviceExtension = {
            vk::KHRSwapchainExtensionName};


    public : 
  void initVulkan();
  void cleanupSwapChain();
  void recreateSwapChain();
  bool isDeviceSuitable(vk::raii::PhysicalDevice const& physicalDevice);
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createSwapChain();
  void createImageViews();
  void createDescriptorSetLayout();
  void createGraphicsPipeline();
  void createCommandPool();
  void createColorResources();
  void createDepthResources();
  vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates,
                                 vk::ImageTiling tiling,
                                 vk::FormatFeatureFlags features) const;
  [[nodiscard]] vk::Format findDepthFormat() const;
  static bool hasStencilComponent(vk::Format format);
  void createTextureImage();
  void generateMipmaps(vk::raii::Image& image,
                       vk::Format imageFormat,
                       int32_t texWidth,
                       int32_t texHeight,
                       uint32_t mipLevels);
  vk::SampleCountFlagBits getMaxUsableSampleCount();
  void createTextureImageView();
  void createTextureSampler();
  [[nodiscard]] vk::raii::ImageView createImageView(
      const vk::raii::Image& image,
      vk::Format format,
      vk::ImageAspectFlags aspectFlags,
      uint32_t mipLevels) const;
  void createImage(uint32_t width,
                   uint32_t height,
                   uint32_t mipLevels,
                   vk::SampleCountFlagBits numSamples,
                   vk::Format format,
                   vk::ImageTiling tiling,
                   vk::ImageUsageFlags usage,
                   vk::MemoryPropertyFlags properties,
                   vk::raii::Image& image,
                   vk::raii::DeviceMemory& imageMemory);

  void transitionImageLayout(const vk::raii::Image& image,
                             const vk::ImageLayout oldLayout,
                             const vk::ImageLayout newLayout,
                             uint32_t mipLevels);

  void copyBufferToImage(const vk::raii::Buffer& buffer,
                         const vk::raii::Image& image,
                         uint32_t width,
                         uint32_t height);
  void addTorus(float R,float r);
  void loadModel();
  void createVertexBuffer();
  void createIndexBuffer();
  void createUniformBuffers();
  void createDescriptorPool();
  void createDescriptorSets();
  void createBuffer(vk::DeviceSize size,
                    vk::BufferUsageFlags usage,
                    vk::MemoryPropertyFlags properties,
                    vk::raii::Buffer& buffer,
                    vk::raii::DeviceMemory& bufferMemory);

  std::unique_ptr<vk::raii::CommandBuffer> beginSingleTimeCommands();
  void endSingleTimeCommands(
      const vk::raii::CommandBuffer& commandBuffer) const;

  void copyBuffer(vk::raii::Buffer& srcBuffer,
                  vk::raii::Buffer& dstBuffer,
                  vk::DeviceSize size);

  uint32_t findMemoryType(uint32_t typeFilter,
                          vk::MemoryPropertyFlags properties);
  void createCommandBuffers();
  void recordCommandBuffer(uint32_t imageIndex);
  void transition_image_layout(vk::Image image,
                               vk::ImageLayout old_layout,
                               vk::ImageLayout new_layout,
                               vk::AccessFlags2 src_access_mask,
                               vk::AccessFlags2 dst_access_mask,
                               vk::PipelineStageFlags2 src_stage_mask,
                               vk::PipelineStageFlags2 dst_stage_mask,
                               vk::ImageAspectFlags image_aspect_flags);

  void createSyncObjects();
  void updateUniformBuffer(uint32_t currentImage) const;
  void drawFrame(int f = 0);

  [[nodiscard]] vk::raii::ShaderModule createShaderModule(
      const std::vector<char>& code) const;

  static uint32_t chooseSwapMinImageCount(
      vk::SurfaceCapabilitiesKHR const& surfaceCapabilities);

  static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<vk::SurfaceFormatKHR>& availableFormats);

  static vk::PresentModeKHR chooseSwapPresentMode(
      std::vector<vk::PresentModeKHR> const& availablePresentModes);

  vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities);
};

}
