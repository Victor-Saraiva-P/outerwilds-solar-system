cmake_minimum_required(VERSION 3.21)
project(outerwilds_orbits C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Ative warnings úteis
if (CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-Wall -Wextra -Wpedantic -Werror=return-type)
endif()

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(cglm CONFIG REQUIRED)

add_executable(app
  src/main.c
  src/orbit.c
  src/camera.c
  src/render.c
  include/app/app.h
  include/app/orbit.h
  include/app/camera.h
  include/app/render.h
)

target_include_directories(app PRIVATE include)

target_link_libraries(app PRIVATE glfw glad::glad cglm::cglm)

# Linux normalmente não precisa de libs extras, mas garantimos GL
find_package(OpenGL REQUIRED)
target_link_libraries(app PRIVATE OpenGL::GL)
