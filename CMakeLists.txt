cmake_minimum_required(VERSION 3.20)
project(
  tinyclang
  VERSION 0.0.1
  LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE "Debug")
set(project_version "${${PROJECT_NAME}_VERSION}")

message(STATUS "Project '${PROJECT_NAME}', version: '${project_version}'")

option(TINYCLANG_OPT_BUILD_UNITTESTS "Build all tinyclang unittests" ON)

# temp define: https://discourse.llvm.org/t/python-api-problem/945
add_compile_options(-fno-rtti)

# CMake helpers:
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

include(CTest)

include_directories(include)

# Create the tinyclang C++ library
set(tinyclang_SRCS)

set(STATIC_LIB_NAME ${PROJECT_NAME})

# apt install llvm-15-dev
set(LLVM_DIR /usr/lib/llvm-15/lib/cmake/llvm)
# manually install
# set(LLVM_DIR /usr/local/lib/cmake/llvm)

find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

file(GLOB_RECURSE LIB_PATH
    ./lib/Basic/*.cc
    ./lib/Lexer/*.cc
    ./lib/Source/*.cc
    ./lib/Diagnostic/*.cc
  )
list(APPEND tinyclang_SRCS ${LIB_PATH})

add_library(${STATIC_LIB_NAME} STATIC ${tinyclang_SRCS})
target_link_libraries(${STATIC_LIB_NAME}
  LLVMSupport
  LLVMOption
  LLVMCore
)

add_subdirectory(Driver)

if (TINYCLANG_OPT_BUILD_UNITTESTS)
  add_subdirectory(unittests #[[EXCLUDE_FROM_ALL]])
endif()
