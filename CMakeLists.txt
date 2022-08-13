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

add_executable(loader main.cc ${PLATFORM_SOURCES})
target_include_directories(loader PUBLIC include)
set_property(TARGET loader PROPERTY CXX_STANDARD 20)
if (PLATFORM_LIBRARIES)
  target_link_libraries(loader PRIVATE ${PLATFORM_LIBRARIES})
endif()
