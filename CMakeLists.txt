cmake_minimum_required(VERSION 3.14...3.30)

project(BNM LANGUAGES CXX)

# Default to Release if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Options
option(BNM_LINK_LOG "Add dependents to liblog.so" OFF)
option(BNM_SANITIZERS "Enable ASan + UBSan (debug builds)" OFF)

add_library(BNM STATIC
    src/Class.cpp
    src/ClassesManagement.cpp
    src/Coroutine.cpp
    src/Defaults.cpp
    src/Delegates.cpp
    src/EventBase.cpp
    src/Exceptions.cpp
    src/FieldBase.cpp
    src/Hooks.cpp
    src/Image.cpp
    src/Internals.cpp
    src/Loading.cpp
    src/MethodBase.cpp
    src/MonoStructures.cpp
    src/PropertyBase.cpp
    src/UnityStructures.cpp
    src/Utils.cpp
)

target_compile_features(BNM PUBLIC cxx_std_20)

set_target_properties(BNM PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# Include directories — PUBLIC for consumers, PRIVATE for implementation
target_include_directories(BNM
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/utf8
        ${CMAKE_CURRENT_SOURCE_DIR}/src/private
)

# Compiler warnings
target_compile_options(BNM PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wsign-conversion
    -Wnon-virtual-dtor
    -Woverloaded-virtual
    -Wno-dollar-in-identifier-extension
    -Wno-zero-length-array
)

# Sanitizer support — enable with -DBNM_SANITIZERS=ON
if(BNM_SANITIZERS)
    target_compile_options(BNM PUBLIC
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -fno-sanitize-recover=all
    )
    target_link_options(BNM PUBLIC
        -fsanitize=address,undefined
    )
endif()

# Link log
if(BNM_LINK_LOG)
    target_link_libraries(BNM PUBLIC log)
endif()

set_property(TARGET BNM PROPERTY
    BNM_INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/include
)

