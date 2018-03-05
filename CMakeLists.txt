cmake_minimum_required(VERSION 3.1)

project(opentracing-c VERSION 0.0.1 LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain.cmake"
    CACHE FILEPATH "Toolchain to use for building this package")

include(CheckCCompilerFlag)

include(GenerateDocumentation)

function(add_c_flag flags new_flag)
  string(MAKE_C_IDENTIFIER "have${new_flag}" new_flag_var)
  check_c_compiler_flag(${new_flag} ${new_flag_var})
  if(${new_flag_var})
    set(${flags} ${${flags}} ${new_flag} PARENT_SCOPE)
  endif()
endfunction()

add_c_flag(c_flags "-Wc++-compat")
if("${CMAKE_C_COMPILER}" MATCHES "Clang")
  add_c_flag(c_flags "-Weverything")
else()
  add_c_flag(c_flags "-Wall")
  add_c_flag(c_flags "-Wextra")
endif()

set(srcs
  "src/opentracing-c/common.h"
  "src/opentracing-c/span.h"
  "src/opentracing-c/tracer.c"
  "src/opentracing-c/tracer.h"
  "src/opentracing-c/value.h")
add_library(opentracingc SHARED ${srcs})
target_include_directories(opentracingc PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src>
  $<INSTALL_INTERFACE:include>)
target_compile_options(opentracingc PUBLIC ${c_flags})

generate_documentation()
