#include "app/render.h"

static GLuint compile_shader(GLenum type, const char* src) {
  GLuint s = glCreateShader(type);
  glShaderSource(s, 1, &src, NULL);
  glCompileShader(s);
  GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    GLint len = 0; glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
    char* log = (char*)malloc(len);
    glGetShaderInfoLog(s, len, NULL, log);
    fprintf(stderr, "Shader compile error: %s\n", log);
    free(log); glDeleteShader(s); return 0;
  }
  return s;
}

static GLuint link_program(GLuint vs, GLuint fs) {
  GLuint p = glCreateProgram();
  glAttachShader(p, vs);
  glAttachShader(p, fs);
  glLinkProgram(p);
  GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
  if (!ok) {
    GLint len = 0; glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
    char* log = (char*)malloc(len);
    glGetProgramInfoLog(p, len, NULL, log);
    fprintf(stderr, "Program link error: %s\n", log);
    free(log); glDeleteProgram(p); return 0;
  }
  return p;
}

bool read_text_file_fallback(const char* p1, const char* p2, char** out, size_t* out_len) {
  const char* paths[2] = { p1, p2 };
  for (int i = 0; i < 2; ++i) {
    if (!paths[i]) continue;
    FILE* f = fopen(paths[i], "rb");
    if (!f) continue;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc((size_t)n + 1);
    if (!buf) { fclose(f); return false; }
    size_t r = fread(buf, 1, (size_t)n, f);
    fclose(f);
    buf[r] = '\0';
    *out = buf;
    if (out_len) *out_len = r;
    return true;
  }
  return false;
}

bool load_basic_program(BasicProgram* p) {
  char *vsrc = NULL, *fsrc = NULL;
  size_t _;

  // Tenta carregar a partir de quem executa (build/) e a partir da raiz (../shaders/)
  if (!read_text_file_fallback("shaders/basic.vert", "../shaders/basic.vert", &vsrc, &_) ||
      !read_text_file_fallback("shaders/basic.frag", "../shaders/basic.frag", &fsrc, &_)) {
    fprintf(stderr, "Falha ao ler shaders (tente rodar do build/ ou da raiz).\n");
    free(vsrc); free(fsrc);
    return false;
  }

  GLuint vs = compile_shader(GL_VERTEX_SHADER, vsrc);
  GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fsrc);
  free(vsrc); free(fsrc);
  if (!vs || !fs) { if (vs) glDeleteShader(vs); if (fs) glDeleteShader(fs); return false; }

  p->prog = link_program(vs, fs);
  glDeleteShader(vs); glDeleteShader(fs);
  if (!p->prog) return false;

  p->uMVP = glGetUniformLocation(p->prog, "uMVP");
  p->uColor = glGetUniformLocation(p->prog, "uColor");
  p->uPointSize = glGetUniformLocation(p->prog, "uPointSize");
  return true;
}

void destroy_basic_program(BasicProgram* p) {
  if (p->prog) glDeleteProgram(p->prog);
  p->prog = 0;
}

GLuint make_vbo(GLsizeiptr size, const void* data, GLenum usage) {
  GLuint vbo = 0;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);                // GL 3.3 OK
  glBufferData(GL_ARRAY_BUFFER, size, data, usage);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  return vbo;
}

GLuint make_vao_points(GLuint vbo) {
  GLuint vao; glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float)*3, (void*)0);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  return vao;
}

GLuint make_vao_lines(GLuint vbo) {
  // idêntico ao de pontos; o primitive type muda na hora do draw
  return make_vao_points(vbo);
}
