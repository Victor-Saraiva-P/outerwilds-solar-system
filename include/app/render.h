#ifndef APP_RENDER_H
#define APP_RENDER_H

#include "app.h"

typedef struct {
  GLuint prog;
  GLint  uMVP;
  GLint  uColor;
  GLint  uPointSize;
} BasicProgram;

bool load_basic_program(BasicProgram* p);
void destroy_basic_program(BasicProgram* p);

GLuint make_vbo(GLsizeiptr size, const void* data, GLenum usage);
GLuint make_vao_points(GLuint vbo);
GLuint make_vao_lines(GLuint vbo);

bool read_text_file_fallback(const char* p1, const char* p2, char** out, size_t* out_len);

#endif
