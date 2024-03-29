cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

project(engineloader)

set(PLATFORM_SOURCES)
set(PLATFORM_LIBRARIES)
set(CONTEXTS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/contexts)

if (WIN32)
  list(APPEND PLATFORM_SOURCES ${CONTEXTS_PATH}/win32_context.cc)
endif()

if (UNIX)
  find_package(X11 REQUIRED)
  list(APPEND PLATFORM_SOURCES ${CONTEXTS_PATH}/xlib_context.cc)
  list(APPEND PLATFORM_LIBRARIES dl X11 X11::Xfixes)
endif()

add_library(loader_interface INTERFACE)
target_include_directories(loader_interface INTERFACE include)

add_executable(loader main.cc ${PLATFORM_SOURCES})
set_property(TARGET loader PROPERTY CXX_STANDARD 20)
target_link_libraries(loader PRIVATE loader_interface)
if (PLATFORM_LIBRARIES)
  target_link_libraries(loader PRIVATE ${PLATFORM_LIBRARIES})
endif()

if (UNIX)
  target_link_options(loader PUBLIC "-pthread")
endif()
