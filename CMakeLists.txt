cmake_minimum_required(VERSION 3.22)


project(txt-to-epub-body C)

set(CMAKE_C_STANDARD 11)

set( CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0)

add_executable(txt-to-epub-body txt-to-epub-body.c)

target_link_Libraries(txt-to-epub-body PkgConfig::deps)


