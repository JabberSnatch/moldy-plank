cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

project(engineloader)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Vulkan REQUIRED)
add_library(vktk STATIC vktk.cc)
target_link_libraries(vktk Vulkan::Vulkan Vulkan::shaderc)
set_property(TARGET vktk PROPERTY CXX_STANDARD 20)

add_library(engine MODULE engine.cc)
target_link_libraries(engine vktk)
set_property(TARGET engine PROPERTY CXX_STANDARD 20)

set(PLATFORM_SOURCES)
set(PLATFORM_LIBRARIES)

if (WIN32)
  # Win32 custom hot-reload shenaniggans
  target_link_options(engine PRIVATE "/MANIFEST:NO" "-PDB:engine_%random%.pdb")

  add_custom_command(
	TARGET engine PRE_BUILD
    COMMAND echo building > build.lock)
  add_custom_command(
	TARGET engine PRE_BUILD
    COMMAND if exist engine_*.pdb del engine_*.pdb)
  add_custom_command(
	TARGET engine PRE_BUILD
    COMMAND if exist engine.dl_* del engine.dl_*)
  add_custom_command(
	TARGET engine POST_BUILD
    COMMAND del build.lock)

  list(APPEND PLATFORM_SOURCES win32_context.cc)
endif()

if (UNIX)
  target_compile_options(vktk PRIVATE "-fPIC")
  target_compile_options(engine PRIVATE "-fvisibility=hidden")
  find_package(X11 REQUIRED)
  list(APPEND PLATFORM_SOURCES xlib_context.cc)
  list(APPEND PLATFORM_LIBRARIES dl X11 X11::Xfixes)

  add_custom_command(
	TARGET engine PRE_BUILD
	COMMAND echo building > build.lock)
  add_custom_command(
	TARGET engine POST_BUILD
	COMMAND rm build.lock)
endif()

add_executable(loader main.cc ${PLATFORM_SOURCES})
set_property(TARGET loader PROPERTY CXX_STANDARD 20)
if (PLATFORM_LIBRARIES)
  target_link_libraries(loader PRIVATE ${PLATFORM_LIBRARIES})
endif()
