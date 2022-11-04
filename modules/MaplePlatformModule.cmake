message (" MAPLE PLATFORM MODULE ")
set(DARWIN 0)
set(APPLE 0)
set(BUILD_JNI 0)
set(LINUX 0)
set(WINDOWS 0)
set(PICO 0)

message("CMAKE SYSTEM IS :${CMAKE_SYSTEM_NAME}.")
if(ANDROID)
 message("Android is active")
endif(ANDROID)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
 set(LINUX 1)
 message("Linux is active")
endif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
 set(WINDOWS 1)
 message("WINDOWS is active")
endif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

if(${CMAKE_SYSTEM_NAME} STREQUAL "PICO")
 set(PICO 1)
 message("PICO is active")
endif(${CMAKE_SYSTEM_NAME} STREQUAL "PICO")

if(ANDROID)
  set(CMAKE_EXE_LINKER_FLAGS "-fPIE -pie")
endif(ANDROID)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
 set(DARWIN 1)
 message("DARWIN is active")
    set(APPLE 1)
endif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")


if(NOT MSVC)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-std=c++11> $<$<COMPILE_LANGUAGE:C>:-std=c99> -fvisibility=hidden)

include(CheckCXXCompilerFlag)

CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    message("compiler set to c++11")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

endif(NOT MSVC)

if(LINUX)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
message("pthread is linked")
endif(LINUX)
