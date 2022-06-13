cmake_minimum_required(VERSION 3.22)


project(txt-to-epub2 C)

set(CMAKE_C_STANDARD 11)

set( CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0 uuid libzip)

add_executable(txt-to-epub2 txt-to-epub.c)

target_link_Libraries(txt-to-epub2 PkgConfig::deps)

install(TARGETS txt-to-epub2)