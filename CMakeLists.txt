cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(engineloader)

add_link_options("/MANIFEST:NO")
add_library(engine MODULE engine.cc)
set_property(TARGET engine PROPERTY CXX_STANDARD 20)

# Win32 custom hot-reload shenaniggans
target_link_options(engine PUBLIC "-PDB:engine_%random%.pdb")
add_custom_command(TARGET engine PRE_BUILD
                   COMMAND echo building > build.lock)
add_custom_command(TARGET engine PRE_BUILD
                   COMMAND del engine_*.pdb)
add_custom_command(TARGET engine PRE_BUILD
                   COMMAND del engine.dl_*)
add_custom_command(TARGET engine POST_BUILD
                   COMMAND del build.lock)

add_executable(loader main.cc win32_context.cc)
set_property(TARGET loader PROPERTY CXX_STANDARD 20)
