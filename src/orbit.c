#include "app/orbit.h"

// Aproximação: Kepler quase circular (1 iteração para E). Bom o suficiente para e <= ~0.2
static void pos_kepler2D(double t, const Orbit* o, float out2[2]) {
  double M = (double)o->n * t + (double)o->phi0;   // anomalia média
  double E = M + (double)o->e * sin(M);            // 1 iteração
  double x = (double)o->a * (cos(E) - (double)o->e);
  double y = (double)o->a * sqrt(1.0 - (double)o->e * (double)o->e) * sin(E);
  out2[0] = (float)x;
  out2[1] = (float)y;
}

void orbit_position(double t_sim, const Orbit* o, vec3 out_pos) {
  float p2[2];
  pos_kepler2D(t_sim, o, p2);

  mat4 Rz1, Rx, Rz2, R, T;
  glm_mat4_identity(Rz1);
  glm_mat4_identity(Rx);
  glm_mat4_identity(Rz2);
  glm_mat4_identity(R);

  // Rot = Rz(Omega) * Rx(i) * Rz(w)
  glm_rotate_z(Rz1, o->Omega, Rz1);
  glm_rotate_x(Rx,  o->i,     Rx);
  glm_rotate_z(Rz2, o->w,     Rz2);

  glm_mat4_mul(Rz2, Rx, T);
  glm_mat4_mul(Rz1, T, R);

  vec4 v = { p2[0], p2[1], 0.0f, 1.0f };
  vec4 V;
  glm_mat4_mulv(R, v, V);
  glm_vec3(V, out_pos);
}

void orbit_sample_path(const Orbit* o, int samples, double period_hint, vec3* out) {
  // Se não tiver dica de período, use 2π/n (uma volta média)
  double T = (period_hint > 0.0) ? period_hint : (2.0 * M_PI) / (double)o->n;
  for (int k = 0; k < samples; ++k) {
    double t = (T * (double)k) / (double)(samples - 1);
    orbit_position(t, o, out[k]);
  }
}
