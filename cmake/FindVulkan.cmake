# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindVulkan
----------

.. versionadded:: 3.7

Find Vulkan, which is a low-overhead, cross-platform 3D graphics
and computing API.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` targets if Vulkan has been found:

``Vulkan::Vulkan``
  The main Vulkan library.

``Vulkan::glslc``
  .. versionadded:: 3.19

  The GLSLC SPIR-V compiler, if it has been found.

``Vulkan::Headers``
  .. versionadded:: 3.21

  Provides just Vulkan headers include paths, if found.  No library is
  included in this target.  This can be useful for applications that
  load Vulkan library dynamically.

``Vulkan::glslangValidator``
  .. versionadded:: 3.21

  The glslangValidator tool, if found.  It is used to compile GLSL and
  HLSL shaders into SPIR-V.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

``Vulkan_FOUND``
  set to true if Vulkan was found
``Vulkan_INCLUDE_DIRS``
  include directories for Vulkan
``Vulkan_LIBRARIES``
  link against this library to use Vulkan
``Vulkan_VERSION``
  .. versionadded:: 3.23

  value from ``vulkan/vulkan_core.h``

The module will also defines these cache variables:

``Vulkan_INCLUDE_DIR``
  the Vulkan include directory
``Vulkan_LIBRARY``
  the path to the Vulkan library
``Vulkan_GLSLC_EXECUTABLE``
  the path to the GLSL SPIR-V compiler
``Vulkan_GLSLANG_VALIDATOR_EXECUTABLE``
  the path to the glslangValidator tool

Hints
^^^^^

.. versionadded:: 3.18

The ``VULKAN_SDK`` environment variable optionally specifies the
location of the Vulkan SDK root directory for the given
architecture. It is typically set by sourcing the toplevel
``setup-env.sh`` script of the Vulkan SDK directory into the shell
environment.

#]=======================================================================]

include(FindPackageMessage)
include(FindPackageHandleStandardArgs)

if(WIN32)
  find_path(Vulkan_INCLUDE_DIR
    NAMES vulkan/vulkan.h
    HINTS
      "$ENV{VULKAN_SDK}/Include"
    )

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    find_library(Vulkan_LIBRARY
      NAMES vulkan-1
      HINTS
        "$ENV{VULKAN_SDK}/Lib"
        "$ENV{VULKAN_SDK}/Bin"
      )
    find_program(Vulkan_GLSLC_EXECUTABLE
      NAMES glslc
      HINTS
        "$ENV{VULKAN_SDK}/Bin"
      )
    find_library(Vulkan_SPIRV_TOOLS_IMPLIB
      NAMES SPIRV-Tools-shared
      PATHS "$ENV{VULKAN_SDK}/Lib")
    find_file(Vulkan_SPIRV_TOOLS_LIBRARY
      NAMES SPIRV-Tools-shared.dll
      PATHS "$ENV{VULKAN_SDK}/Bin")
    find_library(Vulkan_SHADERC_LIBRARY
      NAMES shaderc_combined
      PATHS
	  "$ENV{VULKAN_SDK}/Lib"
	  "$ENV{VULKAN_SDK}/Bin"
	  )
    find_library(Vulkan_GLSLANG_SPIRV_LIBRARY
      NAMES SPIRV
      HINTS
        "$ENV{VULKAN_SDK}/Lib"
        "$ENV{VULKAN_SDK}/Bin"
      )
    find_library(VULKAN_GLSLANG_OGLCOMPILER_LIBRARY
      NAMES OGLCompiler
      HINTS
        "$ENV{VULKAN_SDK}/Lib"
        "$ENV{VULKAN_SDK}/Bin"
      )
    find_library(VULKAN_GLSLANG_OSDEPENDENT_LIBRARY
      NAMES OSDependent
      HINTS
        "$ENV{VULKAN_SDK}/Lib"
        "$ENV{VULKAN_SDK}/Bin"
      )
    find_library(VULKAN_GLSLANG_MACHINEINDEPENDENT_LIBRARY
      NAMES MachineIndependent
      HINTS
        "$ENV{VULKAN_SDK}/Lib"
        "$ENV{VULKAN_SDK}/Bin"
      )
    find_library(VULKAN_GLSLANG_GENERICCODEGEN_LIBRARY
      NAMES GenericCodeGen
      HINTS
      "$ENV{VULKAN_SDK}/Lib"
      "$ENV{VULKAN_SDK}/Bin"
      )
    find_library(Vulkan_GLSLANG_LIBRARY
      NAMES glslang
      HINTS
        "$ENV{VULKAN_SDK}/Lib"
        "$ENV{VULKAN_SDK}/Bin"
      )
    find_program(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE
      NAMES glslangValidator
      HINTS
        "$ENV{VULKAN_SDK}/Bin"
      )
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    find_library(Vulkan_LIBRARY
      NAMES vulkan-1
      HINTS
        "$ENV{VULKAN_SDK}/Lib32"
        "$ENV{VULKAN_SDK}/Bin32"
      )
    find_program(Vulkan_GLSLC_EXECUTABLE
      NAMES glslc
      HINTS
        "$ENV{VULKAN_SDK}/Bin32"
      )
    find_library(Vulkan_SPIRV_TOOLS_IMPLIB
      NAMES SPIRV-Tools-shared
      HINTS "$ENV{VULKAN_SDK}/Lib32")
    find_file(Vulkan_SPIRV_TOOLS_LIBRARY
      NAMES SPIRV-Tools-shared
      HINTS "$ENV{VULKAN_SDK}/Bin32")
    find_file(Vulkan_SHADERC_LIBRARY
      NAMES shaderc_combined
      HINTS
	  "$ENV{VULKAN_SDK}/Lib32"
	  "$ENV{VULKAN_SDK}/Bin32"
	  )
    find_library(Vulkan_GLSLANG_SPIRV_LIBRARY
      NAMES SPIRV
      HINTS
        "$ENV{VULKAN_SDK}/Lib32"
        "$ENV{VULKAN_SDK}/Bin32"
      )
    find_library(VULKAN_GLSLANG_OGLCOMPILER_LIBRARY
      NAMES OGLCompiler
      HINTS
        "$ENV{VULKAN_SDK}/Lib32"
        "$ENV{VULKAN_SDK}/Bin32"
      )
    find_library(VULKAN_GLSLANG_OSDEPENDENT_LIBRARY
      NAMES OSDependent
      HINTS
        "$ENV{VULKAN_SDK}/Lib32"
        "$ENV{VULKAN_SDK}/Bin32"
      )
    find_library(VULKAN_GLSLANG_MACHINEINDEPENDENT_LIBRARY
      NAMES MachineIndependent
      HINTS
        "$ENV{VULKAN_SDK}/Lib32"
        "$ENV{VULKAN_SDK}/Bin32"
        )
    find_library(VULKAN_GLSLANG_GENERICCODEGEN_LIBRARY
      NAMES GenericCodeGen
      HINTS
      "$ENV{VULKAN_SDK}/Lib32"
      "$ENV{VULKAN_SDK}/Bin32"
      )
    find_library(Vulkan_GLSLANG_LIBRARY
      NAMES glslang
      HINTS
        "$ENV{VULKAN_SDK}/Lib32"
        "$ENV{VULKAN_SDK}/Bin32"
      )
    find_program(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE
      NAMES glslangValidator
      HINTS
        "$ENV{VULKAN_SDK}/Bin32"
      )
  endif()
else()
  find_path(Vulkan_INCLUDE_DIR
    NAMES vulkan/vulkan.h
    HINTS "$ENV{VULKAN_SDK}/include")
  find_library(Vulkan_LIBRARY
    NAMES vulkan
    HINTS "$ENV{VULKAN_SDK}/lib")
  find_program(Vulkan_GLSLC_EXECUTABLE
    NAMES glslc
    HINTS "$ENV{VULKAN_SDK}/bin")
  find_library(Vulkan_SPIRV_TOOLS_LIBRARY
    NAMES SPIRV-Tools-shared
    HINTS "$ENV{VULKAN_SDK}/Lib")
  find_library(Vulkan_SHADERC_LIBRARY
    NAMES shaderc_combined
    HINTS "$ENV{VULKAN_SDK}/Lib")
  find_library(Vulkan_GLSLANG_SPIRV_LIBRARY
    NAMES SPIRV
    HINTS "$ENV{VULKAN_SDK}/Lib")
  find_library(VULKAN_GLSLANG_OGLCOMPILER_LIBRARY
    NAMES OGLCompiler
    HINTS "$ENV{VULKAN_SDK}/Lib")
  find_library(VULKAN_GLSLANG_OSDEPENDENT_LIBRARY
    NAMES OSDependent
    HINTS "$ENV{VULKAN_SDK}/Lib")
  find_library(VULKAN_GLSLANG_MACHINEINDEPENDENT_LIBRARY
    NAMES MachineIndependent
    HINTS "$ENV{VULKAN_SDK}/Lib")
  find_library(VULKAN_GLSLANG_GENERICCODEGEN_LIBRARY
    NAMES GenericCodeGen
    HINTS "$ENV{VULKAN_SDK}/Lib")
  find_library(Vulkan_GLSLANG_LIBRARY
    NAMES glslang
    HINTS "$ENV{VULKAN_SDK}/Lib")
  find_program(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE
    NAMES glslangValidator
    HINTS "$ENV{VULKAN_SDK}/bin")
endif()

set(Vulkan_LIBRARIES ${Vulkan_LIBRARY})
set(Vulkan_INCLUDE_DIRS ${Vulkan_INCLUDE_DIR})

# detect version e.g 1.2.189
set(Vulkan_VERSION "")
if(Vulkan_INCLUDE_DIR)
  set(VULKAN_CORE_H ${Vulkan_INCLUDE_DIR}/vulkan/vulkan_core.h)
  if(EXISTS ${VULKAN_CORE_H})
    file(STRINGS  ${VULKAN_CORE_H} VulkanHeaderVersionLine REGEX "^#define VK_HEADER_VERSION ")
    string(REGEX MATCHALL "[0-9]+" VulkanHeaderVersion "${VulkanHeaderVersionLine}")
    file(STRINGS  ${VULKAN_CORE_H} VulkanHeaderVersionLine2 REGEX "^#define VK_HEADER_VERSION_COMPLETE ")
    string(REGEX MATCHALL "[0-9]+" VulkanHeaderVersion2 "${VulkanHeaderVersionLine2}")
    list(LENGTH VulkanHeaderVersion2 _len)
    #  versions >= 1.2.175 have an additional numbers in front of e.g. '0, 1, 2' instead of '1, 2'
    if(_len EQUAL 3)
        list(REMOVE_AT VulkanHeaderVersion2 0)
    endif()
    list(APPEND VulkanHeaderVersion2 ${VulkanHeaderVersion})
    list(JOIN VulkanHeaderVersion2 "." Vulkan_VERSION)
  endif()
endif()

find_package_handle_standard_args(Vulkan
  REQUIRED_VARS
    Vulkan_LIBRARY
    Vulkan_INCLUDE_DIR
  VERSION_VAR
    Vulkan_VERSION
)

mark_as_advanced(Vulkan_INCLUDE_DIR Vulkan_LIBRARY Vulkan_GLSLC_EXECUTABLE
                 Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)

if(Vulkan_FOUND AND NOT TARGET Vulkan::Vulkan)
  add_library(Vulkan::Vulkan UNKNOWN IMPORTED)
  set_target_properties(Vulkan::Vulkan PROPERTIES
    IMPORTED_LOCATION "${Vulkan_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIRS}")

  find_package_message(Vulkan::Vulkan
    "Found Vulkan library: ${Vulkan_LIBRARIES}"
    "[${Vulkan_LIBRARIES}][${Vulkan_INCLUDE_DIRS}]"
  )
endif()

if(Vulkan_FOUND AND NOT TARGET Vulkan::Headers)
  add_library(Vulkan::Headers INTERFACE IMPORTED)
  set_target_properties(Vulkan::Headers PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIRS}")

  find_package_message(Vulkan::Headers
    "Found Vulkan headers: ${Vulkan_INCLUDE_DIRS}"
    "[${Vulkan_INCLUDE_DIRS}]"
  )
endif()

if(Vulkan_FOUND AND Vulkan_GLSLC_EXECUTABLE AND NOT TARGET Vulkan::glslc)
  add_executable(Vulkan::glslc IMPORTED)
  set_property(TARGET Vulkan::glslc PROPERTY IMPORTED_LOCATION "${Vulkan_GLSLC_EXECUTABLE}")

  find_package_message(Vulkan::glslc
    "Found Vulkan glslc tool: ${Vulkan_GLSLC_EXECUTABLE}"
    "[${Vulkan_GLSLC_EXECUTABLE}]"
  )
endif()

if(Vulkan_FOUND AND Vulkan_SPIRV_TOOLS_LIBRARY AND NOT TARGET Vulkan::SPIRV-Tools)
  add_library(Vulkan::SPIRV-Tools SHARED IMPORTED)
  set_target_properties(Vulkan::SPIRV-Tools PROPERTIES
    IMPORTED_IMPLIB "${Vulkan_SPIRV_TOOLS_IMPLIB}"
    IMPORTED_LOCATION "${Vulkan_SPIRV_TOOLS_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIRS}")
  target_compile_definitions(Vulkan::SPIRV-Tools
    INTERFACE
      SPIRV_TOOLS_SHAREDLIB
  )

  find_package_message(Vulkan::SPIRV-Tools
    "Found Vulkan SPIRV-Tools library: ${Vulkan_SPIRV_TOOLS_LIBRARY}"
    "[${Vulkan_SPIRV_TOOLS_LIBRARY}][${Vulkan_SPIRV_TOOLS_IMPLIB}][${Vulkan_INCLUDE_DIRS}]"
  )
endif()

if(Vulkan_FOUND AND Vulkan_SHADERC_LIBRARY AND NOT TARGET Vulkan::shaderc)
  add_library(Vulkan::shaderc STATIC IMPORTED)
  set_target_properties(Vulkan::shaderc PROPERTIES
    IMPORTED_LOCATION "${Vulkan_SHADERC_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIRS}")
  target_link_libraries(Vulkan::shaderc)

  find_package_message(Vulkan::shaderc
    "Found Vulkan shaderc library: ${Vulkan_SHADERC_LIBRARY}"
    "[${Vulkan_SHADERC_LIBRARY}][${Vulkan_INCLUDE_DIRS}]"
  )
endif()

if(Vulkan_FOUND AND NOT TARGET Vulkan::glslang_spirv)
  add_library(Vulkan::glslang_spirv STATIC IMPORTED)
  set_target_properties(Vulkan::glslang_spirv PROPERTIES
    IMPORTED_LOCATION "${Vulkan_GLSLANG_SPIRV_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIRS}")

  find_package_message(Vulkan::glslang_spirv
    "Found Vulkan glslang SPIRV library: ${Vulkan_GLSLANG_SPIRV_LIBRARY}"
    "[${Vulkan_GLSLANG_SPIRV_LIBRARY}][${Vulkan_INCLUDE_DIRS}]"
  )
endif()

if(Vulkan_FOUND AND NOT TARGET Vulkan::glslang_oglcompiler)
  add_library(Vulkan::glslang_oglcompiler STATIC IMPORTED)
  set_target_properties(Vulkan::glslang_oglcompiler PROPERTIES
    IMPORTED_LOCATION "${VULKAN_GLSLANG_OGLCOMPILER_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIRS}")

  find_package_message(Vulkan::glslang_oglcompiler
    "Found Vulkan glslang OpenGL Compiler library: ${VULKAN_GLSLANG_OGLCOMPILER_LIBRARY}"
    "[${VULKAN_GLSLANG_OGLCOMPILER_LIBRARY}][${Vulkan_INCLUDE_DIRS}]"
  )
endif()

if(Vulkan_FOUND AND NOT TARGET Vulkan::glslang_osdependent)
  add_library(Vulkan::glslang_osdependent STATIC IMPORTED)
  set_target_properties(Vulkan::glslang_osdependent PROPERTIES
    IMPORTED_LOCATION "${VULKAN_GLSLANG_OSDEPENDENT_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIRS}")

  find_package_message(Vulkan::glslang_osdependent
    "Found Vulkan glslang OS-dependent library: ${VULKAN_GLSLANG_OSDEPENDENT_LIBRARY}"
    "[${VULKAN_GLSLANG_OSDEPENDENT_LIBRARY}][${Vulkan_INCLUDE_DIRS}]"
  )
endif()

if(Vulkan_FOUND AND NOT TARGET Vulkan::glslang_machineindependent)
  add_library(Vulkan::glslang_machineindependent STATIC IMPORTED)
  set_target_properties(Vulkan::glslang_machineindependent PROPERTIES
    IMPORTED_LOCATION "${VULKAN_GLSLANG_MACHINEINDEPENDENT_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIRS}")

  find_package_message(Vulkan::glslang_machineindependent
    "Found Vulkan glslang machine independent library: ${VULKAN_GLSLANG_MACHINEINDEPENDENT_LIBRARY}"
    "[${VULKAN_GLSLANG_MACHINEINDEPENDENT_LIBRARY}][${Vulkan_INCLUDE_DIRS}]"
  )
endif()

if(Vulkan_FOUND AND NOT TARGET Vulkan::glslang_genericcodegen)
  add_library(Vulkan::glslang_genericcodegen STATIC IMPORTED)
  set_target_properties(Vulkan::glslang_genericcodegen PROPERTIES
    IMPORTED_LOCATION "${VULKAN_GLSLANG_GENERICCODEGEN_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIRS}")

  find_package_message(Vulkan::glslang_genericcodegen
    "Found Vulkan glslang generic code gen library: ${VULKAN_GLSLANG_GENERICCODEGEN_LIBRARY}"
    "[${VULKAN_GLSLANG_GENERICCODEGEN_LIBRARY}][${Vulkan_INCLUDE_DIRS}]"
  )
endif()

if(Vulkan_FOUND AND NOT TARGET Vulkan::glslang)
  add_library(Vulkan::glslang STATIC IMPORTED)
  set_target_properties(Vulkan::glslang PROPERTIES
    IMPORTED_LOCATION "${Vulkan_GLSLANG_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIRS}")
  target_link_libraries(Vulkan::glslang
    INTERFACE
      Vulkan::glslang_spirv
      Vulkan::glslang_oglcompiler
      Vulkan::glslang_osdependent
      Vulkan::glslang_machineindependent
      Vulkan::glslang_genericcodegen
  )

  find_package_message(Vulkan::glslang
    "Found Vulkan glslang library: ${Vulkan_GLSLANG_LIBRARY}"
    "[${Vulkan_GLSLANG_LIBRARY}][${Vulkan_INCLUDE_DIRS}]"
  )
endif()

if(Vulkan_FOUND AND Vulkan_GLSLANG_VALIDATOR_EXECUTABLE AND NOT TARGET Vulkan::glslangValidator)
  add_executable(Vulkan::glslangValidator IMPORTED)
  set_property(TARGET Vulkan::glslangValidator PROPERTY IMPORTED_LOCATION "${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}")

  find_package_message(Vulkan::glslangValidator
    "Found Vulkan glslangValidator tool: ${Vulkan_GLSLC_EXECUTABLE}"
    "[${Vulkan_GLSLC_EXECUTABLE}]"
  )
endif()
