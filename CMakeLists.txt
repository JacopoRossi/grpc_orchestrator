cmake_minimum_required(VERSION 3.15)
project(grpc_orchestrator VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}  # For generated protobuf files
)

# ============================================================================
# Generate gRPC/Protobuf files
# ============================================================================

set(PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/orchestrator.proto
)

# Get protobuf and gRPC plugin paths
get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)

# Generate protobuf and gRPC files
set(PROTO_SRCS)
set(PROTO_HDRS)
set(GRPC_SRCS)
set(GRPC_HDRS)

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    get_filename_component(PROTO_PATH ${PROTO_FILE} PATH)
    
    set(PROTO_SRC "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc")
    set(PROTO_HDR "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h")
    set(GRPC_SRC "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.cc")
    set(GRPC_HDR "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.h")
    
    add_custom_command(
        OUTPUT ${PROTO_SRC} ${PROTO_HDR} ${GRPC_SRC} ${GRPC_HDR}
        COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
        ARGS --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
             --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
             -I${PROTO_PATH}
             --plugin=protoc-gen-grpc=${grpc_cpp_plugin_location}
             ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating protobuf and gRPC files for ${PROTO_NAME}"
    )
    
    list(APPEND PROTO_SRCS ${PROTO_SRC})
    list(APPEND PROTO_HDRS ${PROTO_HDR})
    list(APPEND GRPC_SRCS ${GRPC_SRC})
    list(APPEND GRPC_HDRS ${GRPC_HDR})
endforeach()

# ============================================================================
# Orchestrator Library
# ============================================================================

add_library(orchestrator_lib
    src/orchestrator.cpp
    src/task_wrapper.cpp
    src/schedule.cpp
    src/rt_utils.cpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

target_link_libraries(orchestrator_lib
    PUBLIC
        gRPC::grpc++
        protobuf::libprotobuf
        Threads::Threads
        yaml-cpp
)

target_include_directories(orchestrator_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
        /usr/include
)

# ============================================================================
# Example Executables
# ============================================================================

# Orchestrator executable
add_executable(orchestrator_main
    examples/orchestrator_main.cpp
)

target_link_libraries(orchestrator_main
    PRIVATE
        orchestrator_lib
)

# Task executable
add_executable(task_main
    examples/task_main.cpp
)

target_link_libraries(task_main
    PRIVATE
        orchestrator_lib
)

# ============================================================================
# Installation
# ============================================================================

install(TARGETS orchestrator_lib orchestrator_main task_main
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

install(FILES ${PROTO_HDRS} ${GRPC_HDRS}
    DESTINATION include
)

# ============================================================================
# Print configuration
# ============================================================================

message(STATUS "")
message(STATUS "=== gRPC Orchestrator Configuration ===")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Protobuf Version: ${Protobuf_VERSION}")
message(STATUS "gRPC Found: ${gRPC_FOUND}")
message(STATUS "")
