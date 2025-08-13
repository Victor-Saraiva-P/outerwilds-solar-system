#include "app/app.h"
#include "app/orbit.h"
#include "app/camera.h"
#include "app/render.h"

// ---------- Estado Global Simples ----------
static struct {
  GLFWwindow* win;
  int width, height;
  double time_scale; // 1.0 = 1 seg sim = 1 seg real. Ex.: 300.0 = 5 min/seg
  bool paused;
  Camera cam;
  BasicProgram prog;

  // Geometrias (VAOs/VBOs)
  GLuint orbit_vbo;
  GLuint orbit_vao;
  int    orbit_vertices;

  GLuint bodies_vbo;
  GLuint bodies_vao;
  int    bodies_count;

  // Dados
  #define MAX_BODIES 16
  Body bodies[MAX_BODIES];
  int  nbodies;
} G;

// ---------- Callback de erro ----------
static void glfw_error_cb(int code, const char* desc) {
  fprintf(stderr, "GLFW error %d: %s\n", code, desc);
}

// ---------- Input ----------
static void key_cb(GLFWwindow* w, int key, int sc, int action, int mods) {
  (void)w; (void)sc; (void)mods;
  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    switch (key) {
      case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(G.win, 1); break;
      case GLFW_KEY_P: G.paused = !G.paused; break;
      case GLFW_KEY_EQUAL: // '+'
      case GLFW_KEY_KP_ADD:   G.time_scale *= 1.25; break;
      case GLFW_KEY_MINUS:
      case GLFW_KEY_KP_SUBTRACT: G.time_scale /= 1.25; if (G.time_scale < 0.01) G.time_scale = 0.01; break;
      case GLFW_KEY_R: camera_reset(&G.cam); break;
      default: break;
    }
  }
}

static void mouse_button_cb(GLFWwindow* w, int button, int action, int mods) {
  (void)w; (void)mods;
  camera_mouse_button(&G.cam, button, action);
}

static void cursor_pos_cb(GLFWwindow* w, double x, double y) {
  (void)w;
  camera_cursor(&G.cam, x, y);
}

static void scroll_cb(GLFWwindow* w, double xoff, double yoff) {
  (void)w; (void)xoff;
  camera_scroll(&G.cam, yoff);
}

static void framebuffer_size_cb(GLFWwindow* w, int width, int height) {
  (void)w;
  G.width = width; G.height = height;
  glViewport(0, 0, width, height);
}

// ---------- Seed de corpos (valores simples) ----------
static void init_bodies() {
  G.nbodies = 0;

  // Sol fixo no centro (n=0)
  G.bodies[G.nbodies++] = (Body){ "Sun", { .a=0.0f, .e=0.0f, .w=0, .i=0, .Omega=0, .n=0.0f, .phi0=0 }, {1.0f, 0.95f, 0.3f}, 1.5f };

  // Planetas de exemplo (valores ilustrativos)
  G.bodies[G.nbodies++] = (Body){ "Timber Hearth", { .a=6.0f, .e=0.02f, .w=0.2f, .i=0.05f, .Omega=0.0f, .n=0.6f, .phi0=0.0f }, {0.3f, 0.8f, 0.9f}, 0.5f };
  G.bodies[G.nbodies++] = (Body){ "Brittle Hollow", { .a=10.0f, .e=0.05f, .w=0.7f, .i=0.1f, .Omega=0.4f, .n=0.35f, .phi0=0.9f }, {0.9f, 0.5f, 0.2f}, 0.6f };
  G.bodies[G.nbodies++] = (Body){ "Giant's Deep", { .a=14.0f, .e=0.01f, .w=0.1f, .i=0.0f, .Omega=0.2f, .n=0.25f, .phi0=1.7f }, {0.2f, 0.9f, 0.5f}, 0.6f };
  G.bodies[G.nbodies++] = (Body){ "Hourglass Twins", { .a=4.0f, .e=0.03f, .w=0.5f, .i=0.0f, .Omega=0.0f, .n=0.9f, .phi0=0.4f }, {0.95f, 0.75f, 0.2f}, 0.45f };
  G.bodies[G.nbodies++] = (Body){ "Dark Bramble", { .a=20.0f, .e=0.2f, .w=1.2f, .i=0.25f, .Omega=0.8f, .n=0.12f, .phi0=2.4f }, {0.8f, 0.8f, 0.9f}, 0.7f };
}

// ---------- Buffers para órbitas e pontos ----------
static void build_geometry() {
  // (1) Trajetória das órbitas: concatenamos as linhas de todos os corpos orbitantes (ignorando o Sol fixo)
  const int samples = 256;
  int orbits = (G.nbodies - 1);
  G.orbit_vertices = orbits * samples;
  vec3* path = (vec3*)malloc(sizeof(vec3) * (size_t)G.orbit_vertices);

  int idx = 0;
  for (int i = 1; i < G.nbodies; ++i) {
    orbit_sample_path(&G.bodies[i].orbit, samples, 0.0, &path[idx]);
    idx += samples;
  }

  G.orbit_vbo = make_vbo(sizeof(vec3) * G.orbit_vertices, path, GL_STATIC_DRAW);
  G.orbit_vao = make_vao_lines(G.orbit_vbo);
  free(path);

  // (2) Posições dos corpos como GL_POINTS (atualizaremos a cada frame)
  G.bodies_count = G.nbodies;
  vec3* pts = (vec3*)calloc((size_t)G.bodies_count, sizeof(vec3)); // inicia tudo em zero
  G.bodies_vbo = make_vbo(sizeof(vec3) * G.bodies_count, pts, GL_DYNAMIC_DRAW);
  G.bodies_vao = make_vao_points(G.bodies_vbo);
  free(pts);
}

// ---------- Atualização das posições dos corpos ----------
static void update_body_positions(double t_sim) {
  glBindBuffer(GL_ARRAY_BUFFER, G.bodies_vbo);
  vec3* ptr = (vec3*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
  if (!ptr) return;

  // Sol em (0,0,0)
  glm_vec3_zero(ptr[0]);

  for (int i = 1; i < G.nbodies; ++i) {
    orbit_position(t_sim, &G.bodies[i].orbit, ptr[i]);
  }

  glUnmapBuffer(GL_ARRAY_BUFFER);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ---------- Desenho ----------
static void draw_scene(double t_sim) {
  mat4 V, P, VP;
  camera_viewproj(&G.cam, G.width, G.height, V, P);
  glm_mat4_mul(P, V, VP);

  glUseProgram(G.prog.prog);
  glUniformMatrix4fv(G.prog.uMVP, 1, GL_FALSE, (const GLfloat*)VP);

  // 1) órbitas (line strip por planeta)
  glUniform3f(G.prog.uColor, 0.5f, 0.5f, 0.55f);
  glBindVertexArray(G.orbit_vao);
  int samples = 256;
  int offset = 0;
  for (int i = 1; i < G.nbodies; ++i) {
    glDrawArrays(GL_LINE_STRIP, offset, samples);
    offset += samples;
  }

  // 2) corpos como pontos (tamanhos diferentes via uPointSize)
  glBindVertexArray(G.bodies_vao);
  glEnable(GL_PROGRAM_POINT_SIZE);
  for (int i = 0; i < G.nbodies; ++i) {
    vec3 col; glm_vec3_copy(G.bodies[i].color, col);
    glUniform3f(G.prog.uColor, col[0], col[1], col[2]);
    float size = (i == 0) ? 14.0f : (6.0f * G.bodies[i].radius); // Sol maior
    glUniform1f(G.prog.uPointSize, size);
    glDrawArrays(GL_POINTS, i, 1);
  }

  glBindVertexArray(0);
  glUseProgram(0);
}

// ---------- Main ----------
int main(void) {
  glfwSetErrorCallback(glfw_error_cb);
  if (!glfwInit()) return 1;

  // GL 3.3 Core
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  G.width = 1280; G.height = 720;
  G.win = glfwCreateWindow(G.width, G.height, "OuterWilds Orbits (C+OpenGL)", NULL, NULL);
  if (!G.win) { glfwTerminate(); return 1; }
  glfwMakeContextCurrent(G.win);
  glfwSwapInterval(1);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    fprintf(stderr, "Falha ao inicializar glad.\n");
    return 1;
  }

  glfwSetKeyCallback(G.win, key_cb);
  glfwSetMouseButtonCallback(G.win, mouse_button_cb);
  glfwSetCursorPosCallback(G.win, cursor_pos_cb);
  glfwSetScrollCallback(G.win, scroll_cb);
  glfwSetFramebufferSizeCallback(G.win, framebuffer_size_cb);

  camera_init(&G.cam);
  G.time_scale = 60.0; // 1 seg real = 1 minuto sim (ajuste com +/-)
  G.paused = false;

  if (!load_basic_program(&G.prog)) {
    fprintf(stderr, "Falha ao criar programa básico.\n");
    return 1;
  }

  glEnable(GL_DEPTH_TEST);
  init_bodies();
  build_geometry();

  double t0 = glfwGetTime();
  double last_title = 0.0;
  while (!glfwWindowShouldClose(G.win)) {
    double t_now = glfwGetTime();
    static double t_sim = 0.0;
    double dt_real = t_now - t0;
    t0 = t_now;
    if (!G.paused) t_sim += dt_real * G.time_scale;

    // atualizar posições de corpos (buffer de pontos)
    update_body_positions(t_sim);

    glClearColor(0.06f, 0.07f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_scene(t_sim);

    // título com FPS e time_scale
    if (t_now - last_title > 0.25) {
      double fps = 1.0 / (dt_real > 0.0001 ? dt_real : 0.0001);
      char title[128];
      snprintf(title, sizeof(title), "OW Orbits | FPS: %.0f | scale: %.1fx | %s",
               fps, G.time_scale, G.paused ? "PAUSED" : "RUN");
      glfwSetWindowTitle(G.win, title);
      last_title = t_now;
    }

    glfwSwapBuffers(G.win);
    glfwPollEvents();
  }

  destroy_basic_program(&G.prog);
  glfwDestroyWindow(G.win);
  glfwTerminate();
  return 0;
}
