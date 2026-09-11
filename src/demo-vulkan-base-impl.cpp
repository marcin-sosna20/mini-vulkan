#include "demo-vulkan-base-impl.h"
#include <assert.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>
// #include <fstream>
#include <iostream>
// #include <limits>
#include <stdexcept>
#include <vector>

static void global_registry_handler(void* data,
                                    struct wl_registry* registry,
                                    uint32_t id,
                                    const char* interface,
                                    uint32_t version) {
  // g_print("Got a registry event for %s id %d\n", interface, id);
  if (strcmp(interface, "wl_compositor") == 0) {
    reinterpret_cast<struct Demo::my_context*>(data)->wl_compositor =
        (struct wl_compositor*)wl_registry_bind(
            registry, id, &wl_compositor_interface, version);
  } else if (strcmp(interface, "wl_subcompositor") == 0) {
    ((struct Demo::my_context*)data)->wl_subcompositor =
        (struct wl_subcompositor*)wl_registry_bind(
            registry, id, &wl_subcompositor_interface, version);
  }
}

static void global_registry_remover(void* data,
                                    struct wl_registry* registry,
                                    uint32_t id) {
  (void)data;
  (void)registry;
  g_print("Got a registry losing event for %d\n", id);
}

const struct wl_registry_listener listener = {global_registry_handler,
                                              global_registry_remover};

::Demo::VulkanBaseImpl::VulkanBaseImpl(GtkWidget* win,
                                       bool below_mainwindow) {
  m_below_mainwindow = below_mainwindow;
  m_window = win;
  m_bounds = { {0,0},{4,4}};
  m_width = (int)m_bounds.size.width;
  m_height = (int)m_bounds.size.height;
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL Demo::VulkanBaseImpl::debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*) {
  if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError ||
      severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
    std::cerr << "validation layer: type " << to_string(type)
              << " msg: " << pCallbackData->pMessage << std::endl;
  }

  return vk::False;
}

void ::Demo::VulkanBaseImpl::initWaylandSubcompositor() {
  GtkNative* window = gtk_widget_get_native((GtkWidget*)(m_window));
  double x, y;
  gtk_native_get_surface_transform(window, &x, &y);
  m_native_transform_x = x;
  m_native_transform_y = y;

  GdkDisplay* display = gtk_root_get_display(GTK_ROOT(window));
  GdkSurface* gdk_surface = gtk_native_get_surface(GTK_NATIVE(window));

  this->wl_display = gdk_wayland_display_get_wl_display(display);
  wl_mainwindow_surface = gdk_wayland_surface_get_wl_surface(gdk_surface);

  struct Demo::my_context my_context = {nullptr, nullptr};
  struct wl_registry* wl_registry = wl_display_get_registry(wl_display);

  wl_registry_add_listener(wl_registry, &listener, &my_context);
  wl_display_dispatch(wl_display);
  wl_display_roundtrip(wl_display);

  if (my_context.wl_compositor == NULL || my_context.wl_subcompositor == NULL) {
    g_print("Ups!! No compositor , no subcompositor\n");
  }

  wl_vulkan_surface = wl_compositor_create_surface(my_context.wl_compositor);
  widget_region = wl_compositor_create_region(my_context.wl_compositor);
  struct wl_region* r2 = wl_compositor_create_region(my_context.wl_compositor);
  wl_region_add(widget_region, 0, 0, m_width, m_height);
  wl_surface_set_opaque_region(wl_vulkan_surface, widget_region);
  wl_region_add(r2, 0, 0, 0, 0);
  wl_surface_set_input_region(wl_vulkan_surface, r2);
  subsurface = wl_subcompositor_get_subsurface(
      my_context.wl_subcompositor, wl_vulkan_surface, wl_mainwindow_surface);
  wl_subsurface_set_position(subsurface,
                             m_native_transform_x + m_bounds.origin.x,
                             m_native_transform_y + m_bounds.origin.y);
                             
    //TODO: sync or desync ??? 
  wl_subsurface_set_sync(subsurface);
  if (m_below_mainwindow)
    wl_subsurface_place_below(subsurface, wl_mainwindow_surface);

  wl_registry_destroy(wl_registry);
}

void Demo::VulkanBaseImpl::cleanup() {
  wl_subsurface_destroy(subsurface);
  // wl_subcompositor_destroy(my_context.wl_subcompositor);
  wl_region_destroy(widget_region);
  wl_surface_destroy(wl_vulkan_surface);
}

void Demo::VulkanBaseImpl::createInstance() {
  constexpr vk::ApplicationInfo appInfo{
      .pApplicationName = "Hello Triangle",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "No Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = vk::ApiVersion14};

  // Get the required layers
  std::vector<char const*> requiredLayers;
  if (enableValidationLayers) {
    requiredLayers.assign(validationLayers.begin(), validationLayers.end());
  }

  // Check if the required layers are supported by the Vulkan implementation.
  auto layerProperties = context.enumerateInstanceLayerProperties();
  auto unsupportedLayerIt = std::ranges::find_if(
      requiredLayers, [&layerProperties](auto const& requiredLayer) {
        return std::ranges::none_of(
            layerProperties, [requiredLayer](auto const& layerProperty) {
              return strcmp(layerProperty.layerName, requiredLayer) == 0;
            });
      });
  if (unsupportedLayerIt != requiredLayers.end()) {
    throw std::runtime_error("Required layer not supported: " +
                             std::string(*unsupportedLayerIt));
  }

  // Get the required extensions.
  auto requiredExtensions = getRequiredInstanceExtensions();

  // Check if the required extensions are supported by the Vulkan
  // implementation.
  auto extensionProperties = context.enumerateInstanceExtensionProperties();
  auto unsupportedPropertyIt = std::ranges::find_if(
      requiredExtensions,
      [&extensionProperties](auto const& requiredExtension) {
        return std::ranges::none_of(
            extensionProperties,
            [requiredExtension](auto const& extensionProperty) {
              return strcmp(extensionProperty.extensionName,
                            requiredExtension) == 0;
            });
      });
  if (unsupportedPropertyIt != requiredExtensions.end()) {
    throw std::runtime_error("Required extension not supported: " +
                             std::string(*unsupportedPropertyIt));
  }

  vk::InstanceCreateInfo createInfo{
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
      .ppEnabledLayerNames = requiredLayers.data(),
      .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
      .ppEnabledExtensionNames = requiredExtensions.data()};
  instance = vk::raii::Instance(context, createInfo);
}

void Demo::VulkanBaseImpl::setupDebugMessenger() {
  if (!enableValidationLayers)
    return;

  vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
  vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
      vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
      vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
      vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
  vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
      .messageSeverity = severityFlags,
      .messageType = messageTypeFlags,
      .pfnUserCallback = &debugCallback};
  debugMessenger =
      instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}

void Demo::VulkanBaseImpl::createSurface() {
  VkWaylandSurfaceCreateInfoKHR info = {
      .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
      .pNext = 0,
      .flags = 0,
      .display = wl_display,
      .surface = wl_vulkan_surface};
  // VkWaylandSurfaceCreateInfoKHR info {.flags=0,.display=wl_display,.surface =
  // s};
  VkSurfaceKHR vk_surface;
  // err = create_wayland_surface(instance,&info,NULL,&vk_surface);
  // typedef VkResult(*func1_t)(VkInstance,const
  // VkWaylandSurfaceCreateInfoKHR*,const VkAllocationCallbacks*,VkSurfaceKHR*);
  // func1_t create_wayland_surface = (func1_t)
  // ::vkGetInstanceProcAddr(*instance,"vkCreateWaylandSurfaceKHR"); vk::Result
  // err = vkCreateWaylandSurfaceKHR(instance,&info,NULL,&vk_surface);
  ::VkResult err =
      ::vkCreateWaylandSurfaceKHR(*instance, &info, NULL, &vk_surface);

  if (err == VK_SUCCESS) {
    // g_print("vk_surface created.\n");
  }

  surface = vk::raii::SurfaceKHR(instance, vk_surface);
}
std::vector<const char*> Demo::VulkanBaseImpl::getRequiredInstanceExtensions() {
  std::vector<const char*> extensions = {VK_KHR_SURFACE_EXTENSION_NAME,
                                         VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME};
  //	uint32_t glfwExtensionCount = 0;
  //	auto     glfwExtensions     =
  // glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  //	std::vector extensions(glfwExtensions, glfwExtensions +
  // glfwExtensionCount);
  if (enableValidationLayers) {
    extensions.push_back(vk::EXTDebugUtilsExtensionName);
  }

  return extensions;
}

void ::Demo::VulkanBaseImpl::updateBounds() {
   GtkNative *native =  gtk_widget_get_native(m_window);
  double x, y;
  gtk_native_get_surface_transform(native,&x, &y);
  // gtk_native_get_surface_transform(window,&x,&y);
  graphene_rect_t bounds;
  gtk_widget_compute_bounds(m_window,(::GtkWidget*)native, &bounds);
  bool flag = false;
  if (m_native_transform_x != x || m_native_transform_y != y) {
    m_native_transform_x = x;
    m_native_transform_y = y;
    flag = true;
  }
  if (flag || m_bounds.origin.x != bounds.origin.x ||
      m_bounds.origin.y != bounds.origin.y) {
    // translate
    wl_subsurface_set_position(subsurface,
                               m_native_transform_x + bounds.origin.x,
                               m_native_transform_y + bounds.origin.y);
    // wl_surface_commit(wl_mainwindow_surface);
  }
  if (m_bounds.size.width != bounds.size.width ||
      m_bounds.size.height != bounds.size.height) {
    framebufferResized = true;
  }
  m_bounds = bounds;
  m_width = (int)m_bounds.size.width;
  m_height = (int)m_bounds.size.height;
}

void ::Demo::VulkanBaseImpl::waylandVulkanSurfaceCommit() {
  wl_surface_commit(wl_vulkan_surface);
  wl_surface_commit(wl_mainwindow_surface);
}
