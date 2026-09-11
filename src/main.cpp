#include <adwaita.h>
#include "demo-vulkan-impl.h"
#include <glm/gtx/quaternion.hpp>

/* We define two types
 *  SkVulkan - it is a wrapper for Demo::VulkanImpl + arcball 
 *  SkWindow - loads window.ui template
*/

enum class AxisSet { NONE, CAMERA, BODY, WORLD };

struct Arc {
  glm::vec3 from;
  glm::vec3 to;
};

struct Constraint {
  AxisSet current;
  std::vector<glm::vec3> available;
  unsigned int nearest;
};

struct Orientation {
  glm::quat reset;
  glm::quat start;
  glm::quat now;
};

struct Viewport {
  glm::vec2 start;
  glm::vec2 end;
  float get_x() const { return start.x; }
  float get_y() const { return start.y; }
  float get_width() const { return end.x - start.x; }
  float get_height() const { return end.y - start.y; }
};

G_BEGIN_DECLS

#define SK_TYPE_VULKAN (sk_vulkan_get_type())
G_DECLARE_FINAL_TYPE(SkVulkan, sk_vulkan, SK, VULKAN, GtkWidget)

struct _SkVulkanClass {
  GtkWidgetClass parent_class;
};

struct _SkVulkan {
  GtkWidget parent_instance;
  ::Demo::VulkanImpl* m_vulkanImpl;
  struct Members {
    // arcball
    Viewport m_viewport;
    bool m_mousePressed = false;
    double m_mouseX, m_mouseY;
    // glm::mat4 arcball(double x, double y);
    // void mouseup();
    // void updateArcball();
    float radius;
    bool dragging;
    glm::ivec2 mouse_position_start;
    // glm::quat eyeOrientation;
    Arc drag;
    Arc result;
    Orientation orientation;
    Constraint constraint;
  } m;
};

SkVulkan* sk_vulkan_new(void);

G_END_DECLS

G_DEFINE_FINAL_TYPE(SkVulkan, sk_vulkan, GTK_TYPE_WIDGET)

SkVulkan* 
sk_vulkan_new(void) {
  return (SkVulkan*)g_object_new(SK_TYPE_VULKAN, NULL);
}

static glm::mat4 
sk_vulkan_arcball(SkVulkan* self, double x, double y) {
  auto ball_coord = [](Viewport viewport, glm::vec2 mouse_position) {
    float radius = 0.75;
    float aspect = (float)viewport.get_height() / viewport.get_width();
    glm::vec3 window_mouse_position = glm::vec3(
        2.0f * (mouse_position.x - viewport.get_x()) / viewport.get_width() -
            1.0f,
        -(2.0f * (mouse_position.y - viewport.get_y()) / viewport.get_height() -
          1.0f) *
            aspect,
        0.0f);

    // g_print("window cords %f %f
    // \n",window_mouse_position.x,window_mouse_position.y);
    //  TODO: center should be at the object position and in window coordinates
    glm::vec3 center = glm::vec3(0.0f);
    glm::vec3 point = (window_mouse_position - center) / radius;

    float r = glm::length(point);
    if (r > 1.0f) {
      // set to nearest point on ball
      point *= (1.0f / sqrt(r));
    } else {
      // point on ball
      point.z = sqrt(1.0f - r);
    }

    // g_print("Ball cords: x=%f y=%f z=%f\n",point.x,point.y,point.z);

    return point;
  };
  glm::vec2 pos_start(self->m.m_mouseX, self->m.m_mouseY);
  self->m.drag.from = ball_coord(self->m.m_viewport, pos_start);
  glm::vec2 pos_current(x, y);
  self->m.drag.to = ball_coord(self->m.m_viewport, pos_current);

  if (true) {
    float w = glm::dot(self->m.drag.from, self->m.drag.to);
    // glm::vec3 v = eyeOrientation * glm::cross(drag.from, drag.to);
    glm::vec3 v = glm::cross(self->m.drag.from, self->m.drag.to);
    glm::quat orientation_drag(w, v);

    // product of two quaternions give the combination of the rotations
    // they represent
    self->m.orientation.now =
        glm::normalize(orientation_drag * self->m.orientation.start);
  }
  const glm::quat q = self->m.orientation.start;

  // pick an initial point that is perpendicular to the quaternion vector
  float s = sqrt(q.x * q.x + q.y * q.y);
  if (s == 0.0f) {
    self->m.result.from.x = 0.0f;
    self->m.result.from.y = 1.0f;
    self->m.result.from.z = 0.0f;
  } else {
    self->m.result.from.x = -q.y / s;
    self->m.result.from.y = q.x / s;
    self->m.result.from.z = 0.0f;
  }

  self->m.result.to.x =
      q.w * self->m.result.from.x - q.z * self->m.result.from.y;
  self->m.result.to.y =
      q.w * self->m.result.from.y + q.z * self->m.result.from.x;
  self->m.result.to.z =
      q.x * self->m.result.from.y - q.y * self->m.result.from.x;

  // negate initial ball point for a shorter arc
  if (q.w < 0.0f) {
    self->m.result.from.x = -self->m.result.from.x;
    self->m.result.from.y = -self->m.result.from.y;
    self->m.result.from.z = 0.0f;
  }
  auto m = glm::toMat4(self->m.orientation.now);
  return m;
}

static void 
sk_vulkan_size_allocate(GtkWidget* self,
                                    int width,
                                    int height,
                                    int baseline) {
  GTK_WIDGET_CLASS(sk_vulkan_parent_class)
      ->size_allocate(self, width, height, baseline);
  SkVulkan* o = (SkVulkan*)self;
  ::Demo::VulkanImpl* v = o->m_vulkanImpl;
  if (v != nullptr) {
      
    o->m.m_viewport.start.x = 0;
    o->m.m_viewport.start.y = 0;
    o->m.m_viewport.end.x = width;
    o->m.m_viewport.end.y = height;
    v->updateBounds();

    v->drawFrame();
    // TODO:This shouldn't be here but without a second frame we have a problem
    // with synchronization after resizing eg. MainWindow maximization
    // PS. A whole redrawing should be moved out from this function.
    v->drawFrame();
  }
}
static void 
sk_vulkan_dispose(GObject* self) {
  SkVulkan* o = (SkVulkan*)self;
  if (o->m_vulkanImpl) {
    delete o->m_vulkanImpl;
    o->m_vulkanImpl = nullptr;
  }
  G_OBJECT_CLASS(sk_vulkan_parent_class)->dispose(self);
  g_print("Vulkan::dispose()\n");
}
static void 
sk_vulkan_realize(GtkWidget* self) {
  SkVulkan* o = (SkVulkan*)self;
  GTK_WIDGET_CLASS(sk_vulkan_parent_class)->realize(self);
  o->m_vulkanImpl = new ::Demo::VulkanImpl(self, false);
  o->m_vulkanImpl->initVulkan();
  // orientation.now = glm::quat(1.0, 0.0, 0.0, 0.0);
  auto m = glm::toMat4(glm::quat(1.0, 0.0, 0.0, 0.0));
  o->m_vulkanImpl->updateModelRotation(m);
}

static void 
sk_vulkan_motion_cb(SkVulkan* self,
                                double x,
                                double y,
                                gpointer ) {
  if (self->m.m_mousePressed) {
    auto rotation = sk_vulkan_arcball(self, x, y);
    self->m_vulkanImpl->updateModelRotation(rotation);
    self->m_vulkanImpl->drawFrame();
    self->m_vulkanImpl->waylandVulkanSurfaceCommit();
  }
}
static void 
sk_vulkan_mouse_pressed_cb(SkVulkan* self,
                                       int,
                                       double x,
                                       double y,
                                       gpointer) {
  self->m.m_mousePressed = true;
  self->m.m_mouseX = x;
  self->m.m_mouseY = y;
}
static void 
sk_vulkan_mouse_released_cb(SkVulkan* self,
                                        int,
                                        double ,
                                        double ,
                                        gpointer) {
  self->m.m_mousePressed = false;
  self->m.orientation.start = self->m.orientation.now;
}

static void 
sk_vulkan_class_init(SkVulkanClass* klass) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->dispose = sk_vulkan_dispose;
  GTK_WIDGET_CLASS(klass)->size_allocate = sk_vulkan_size_allocate;
  GTK_WIDGET_CLASS(klass)->realize = sk_vulkan_realize;
}


static void 
sk_vulkan_init(SkVulkan* self) {
  self->m_vulkanImpl = nullptr;
  // new (&self->m) SkVulkan::Members;
  self->m.orientation.start = glm::quat(1.0, 0.0, 0.0, 0.0);
  self->m.orientation.now = glm::quat(1.0, 0.0, 0.0, 0.0);
  self->m.radius = 0.75;

  GtkEventControllerMotion* motion =
      (GtkEventControllerMotion*)gtk_event_controller_motion_new();
  g_signal_connect_swapped(motion, "motion", G_CALLBACK(sk_vulkan_motion_cb),
                           (gpointer)self);
  gtk_widget_add_controller((GtkWidget*)self, (GtkEventController*)motion);
  GtkGestureClick* mouse1 = (GtkGestureClick*)gtk_gesture_click_new();
  g_signal_connect_swapped(mouse1, "pressed",
                           G_CALLBACK(sk_vulkan_mouse_pressed_cb),
                           (gpointer)self);
  g_signal_connect_swapped(mouse1, "released",
                           G_CALLBACK(sk_vulkan_mouse_released_cb),
                           (gpointer)self);
  gtk_widget_add_controller((GtkWidget*)self, (GtkEventController*)mouse1);
}

G_BEGIN_DECLS

#define SK_TYPE_WINDOW (sk_window_get_type())

G_DECLARE_FINAL_TYPE(SkWindow, sk_window, SK, WINDOW, AdwApplicationWindow)

struct _SkWindowClass {
  AdwApplicationWindowClass parent_class;
};

struct _SkWindow {
  AdwApplicationWindow parent_instance;
};

SkWindow* sk_window_new(GtkApplication* app);

G_END_DECLS

G_DEFINE_FINAL_TYPE(SkWindow, sk_window, ADW_TYPE_APPLICATION_WINDOW)

SkWindow* sk_window_new(GtkApplication* app) {
  return (SkWindow*)g_object_new(SK_TYPE_WINDOW, "application", app, NULL);
}

static void sk_window_dispose(GObject* self) {
  G_OBJECT_CLASS(sk_window_parent_class)->dispose(self);
}

static void sk_window_class_init(SkWindowClass* klass) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->dispose = sk_window_dispose;
  /*ADW_APPLICATION_WINDOW_CLASS(klass)-> ... = ...;*/

  gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(klass),
                                              "/window.ui");

  // gtk_widget_class_bind_template_child ();
}

static void sk_window_init(SkWindow* self) {
  g_type_ensure(SK_TYPE_VULKAN);
  gtk_widget_init_template(GTK_WIDGET(self));
}

static void activate_cb(GtkApplication* app) {
  GtkWidget* window = (GtkWidget*)sk_window_new(app);
  // GtkWidget *label = gtk_label_new ("Hello World");

  gtk_window_set_title(GTK_WINDOW(window), "Hello");
  gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);
  // gtk_window_set_child (GTK_WINDOW (window), label);
  gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char* argv[]) {
  g_autoptr(AdwApplication) app = NULL;

  app = adw_application_new("org.example.App", G_APPLICATION_DEFAULT_FLAGS);

  g_signal_connect(app, "activate", G_CALLBACK(activate_cb), NULL);

  return g_application_run(G_APPLICATION(app), argc, argv);
}
