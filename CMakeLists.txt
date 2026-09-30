cmake_minimum_required(VERSION 3.23)
project(qxlib VERSION 1.0.1 LANGUAGES CXX)

if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(NOT DEFINED CMAKE_CXX_EXTENSIONS)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)


# Component Registration

set(PROJECT_NAMESPACE qx)
set(PROJECT_COMPONENTS)

function(qx_register_component target name)
  set_target_properties(${target} PROPERTIES EXPORT_NAME ${name})
  add_library(${PROJECT_NAMESPACE}::${name} ALIAS ${target})
  install(TARGETS ${target}
    EXPORT "${PROJECT_NAME}-${name}-targets"
    FILE_SET HEADERS
    COMPONENT ${name}
  )
  install(EXPORT "${PROJECT_NAME}-${name}-targets"
    FILE "${PROJECT_NAME}-${name}-targets.cmake"
    NAMESPACE ${PROJECT_NAMESPACE}::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    COMPONENT ${name}
  )
  set(PROJECT_COMPONENTS ${PROJECT_COMPONENTS} ${name} PARENT_SCOPE)
endfunction()


# inplace_vector

add_library(qx_inplace_vector INTERFACE)
target_compile_features(qx_inplace_vector INTERFACE cxx_std_17)
target_sources(qx_inplace_vector
  INTERFACE
  FILE_SET HEADERS
  BASE_DIRS "include"
  FILES "include/qx/inplace_vector.h"
)
qx_register_component(qx_inplace_vector inplace_vector)


# Testing & Benchmark

option(QX_BUILD_TESTS "Build tests for qxlib" ${PROJECT_IS_TOP_LEVEL})
option(QX_BUILD_BENCHMARKS "Build benchmarks for qxlib" ${PROJECT_IS_TOP_LEVEL})

if(QX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# if(QX_BUILD_BENCHMARKS)
#   add_subdirectory(bench)
# endif()


# Package Config
