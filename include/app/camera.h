#ifndef APP_CAMERA_H
#define APP_CAMERA_H

#include "app.h"
#include <stdbool.h>

typedef struct Camera {
  float yaw;     // rad
  float pitch;   // rad
  float dist;    // distância do alvo
  vec3  target;  // alvo (normalmente o Sol)
  bool  rotating;
  double last_x, last_y;
} Camera;

void camera_init(Camera* c);
void camera_mouse_button(Camera* c, int button, int action);
void camera_cursor(Camera* c, double xpos, double ypos);
void camera_scroll(Camera* c, double yoff);
void camera_reset(Camera* c);
void camera_viewproj(Camera* c, int width, int height, mat4 out_view, mat4 out_proj);

#endif
