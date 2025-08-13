#ifndef APP_ORBIT_H
#define APP_ORBIT_H

#include "app.h"

typedef struct {
  float a;      // semi-eixo maior (unidades de cena)
  float e;      // excentricidade
  float w;      // argumento do periastro (rad)
  float i;      // inclinação (rad)
  float Omega;  // longitude do nodo ascendente (rad)
  float n;      // velocidade angular média (rad/seg de simulação)
  float phi0;   // fase inicial (rad)
} Orbit;

typedef struct {
  const char* name;
  Orbit orbit;
  vec3 color;    // cor do ponto/orbita
  float radius;  // raio visual (apenas para eventual esfera; para pontos use gl_PointSize)
} Body;

void orbit_position(double t_sim, const Orbit* o, vec3 out_pos);
void orbit_sample_path(const Orbit* o, int samples, double period_hint, vec3* out); // gera pontos no percurso

#endif
