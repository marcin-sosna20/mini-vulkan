#pragma once

/* Vulkan RAII Configuration
 * https://deepwiki.com/KhronosGroup/Vulkan-Hpp/6.2-macros-and-configuration-options
 * */

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS 1

#include <gdk/wayland/gdkwayland.h>
#include<gtk/gtk.h>
#include<graphene-1.0/graphene.h>
//#include <peel/GLib/functions.h>
//#include <peel/Graphene/Graphene.h>
//#include <peel/Gtk/Gtk.h>
#//include <peel/class.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
//#include <iostream>
#include <vector>
#include <vulkan/vulkan_raii.hpp>
//this goes after vulkan_raii.hpp 
#include <vulkan/vulkan_wayland.h>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<char const*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

namespace Demo {

struct my_context {
  struct wl_compositor* wl_compositor;
  struct wl_subcompositor* wl_subcompositor;
};

class VulkanBaseImpl {
 public:
  void updateBounds();
  void waylandVulkanSurfaceCommit();
 protected:
  VulkanBaseImpl(GtkWidget* win, bool below_mainwindow);
  static VKAPI_ATTR vk::Bool32 VKAPI_CALL
  debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                vk::DebugUtilsMessageTypeFlagsEXT type,
                const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void*);

  void initWaylandSubcompositor();
  void cleanup();
  void createInstance();
  void setupDebugMessenger();
  void createSurface();
  std::vector<const char*> getRequiredInstanceExtensions();


 protected:
  vk::raii::Context context;
  vk::raii::Instance instance = nullptr;
  vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
  vk::raii::SurfaceKHR surface = nullptr;
  float getSurfaceWidth() { return m_width;}  
  float getSurfaceHeight() { return m_height;}
  bool getFramebufferResized() { return framebufferResized;}  
  void setFramebufferResized(bool flag) { framebufferResized = flag;}  
                       
private:
  // Wayland and Gtk
  GtkWidget* m_window = nullptr;
  bool m_below_mainwindow = false;
  struct wl_display* wl_display = nullptr;
  struct wl_surface* wl_vulkan_surface = nullptr;
  struct wl_surface* wl_mainwindow_surface = nullptr;
  struct wl_region* widget_region = nullptr;
  struct wl_subsurface* subsurface = nullptr;
  struct my_context my_context = {0, 0};
  //Graphene::Rect m_bounds;
  graphene_rect_t m_bounds;
  float m_width, m_height;
  float m_native_transform_x;
  float m_native_transform_y;
  bool framebufferResized = false;
};

}  // namespace Demo

