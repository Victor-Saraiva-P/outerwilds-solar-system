#include "app/camera.h"

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

void camera_init(Camera* c) {
  c->yaw   = glm_rad(45.0f);
  c->pitch = glm_rad(-20.0f);
  c->dist  = 30.0f; // distância inicial
  glm_vec3_zero(c->target);
  c->rotating = false;
  c->last_x = c->last_y = 0.0;
}

void camera_mouse_button(Camera* c, int button, int action) {
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    c->rotating = (action == GLFW_PRESS);
  }
}

void camera_cursor(Camera* c, double xpos, double ypos) {
  if (!c->rotating) { c->last_x = xpos; c->last_y = ypos; return; }
  double dx = xpos - c->last_x;
  double dy = ypos - c->last_y;
  c->last_x = xpos;
  c->last_y = ypos;

  const float sens = 0.005f;
  c->yaw   += (float)dx * sens;
  c->pitch += (float)dy * sens;
  c->pitch = clampf(c->pitch, glm_rad(-89.0f), glm_rad(89.0f));
}

void camera_scroll(Camera* c, double yoff) {
  // zoom logarítmico simples
  float factor = (yoff > 0) ? 0.9f : 1.1f;
  c->dist = clampf(c->dist * factor, 3.0f, 500.0f);
}

void camera_reset(Camera* c) {
  camera_init(c);
}

void camera_viewproj(Camera* c, int width, int height, mat4 out_view, mat4 out_proj) {
  // Converte yaw/pitch/dist para posição esférica ao redor do alvo
  float cy = cosf(c->yaw), sy = sinf(c->yaw);
  float cp = cosf(c->pitch), sp = sinf(c->pitch);
  vec3 eye = {
    c->target[0] + c->dist * cp * cy,
    c->target[1] + c->dist * sp,
    c->target[2] + c->dist * cp * sy
  };
  glm_lookat(eye, c->target, (vec3){0.0f, 1.0f, 0.0f}, out_view);

  float aspect = (float)width / (float)height;
  glm_perspective(glm_rad(50.0f), aspect, 0.1f, 2000.0f, out_proj);
}
