cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(engineloader)

add_executable(loader main.cc win32_context.cc)
set_property(TARGET loader PROPERTY CXX_STANDARD 20)
