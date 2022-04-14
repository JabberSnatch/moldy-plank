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

# Win32 custom hot-reload shenaniggans
target_link_options(engine PRIVATE "/MANIFEST:NO" "-PDB:engine_%random%.pdb")
add_custom_command(TARGET engine PRE_BUILD
                   COMMAND echo building > build.lock)
add_custom_command(TARGET engine PRE_BUILD
                   COMMAND if exist engine_*.pdb del engine_*.pdb)
add_custom_command(TARGET engine PRE_BUILD
                   COMMAND if exist engine.dl_* del engine.dl_*)
add_custom_command(TARGET engine POST_BUILD
                   COMMAND del build.lock)

add_executable(loader main.cc win32_context.cc)
set_property(TARGET loader PROPERTY CXX_STANDARD 20)
