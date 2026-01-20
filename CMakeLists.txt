cmake_minimum_required(VERSION 3.15)
project(grpc_orchestrator VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Build Options - Compilazione Modulare
# ============================================================================
option(BUILD_ORCHESTRATOR "Build orchestrator component" ON)
option(BUILD_TASKS "Build task runner component" ON)
option(BUILD_DEPLOY_MANAGER "Build deploy manager component" ON)
option(BUILD_BUILDER_MANAGER "Build builder manager component" ON)
option(BUILD_ALL "Build all components (override individual options)" ON)

# Find required packages
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)

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
# Libraries - Separate per Componente
# ============================================================================

# Core library (protobuf + utility comuni)
add_library(core_lib
    src/rt_utils.cpp
    src/schedule.cpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

target_link_libraries(core_lib
    PUBLIC
        gRPC::grpc++
        protobuf::libprotobuf
        Threads::Threads
        yaml-cpp
        nlohmann_json::nlohmann_json
)

target_include_directories(core_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
        /usr/include
)

# Orchestrator library (solo se richiesta o se serve per task_runner)
if(BUILD_ALL OR BUILD_ORCHESTRATOR OR BUILD_TASKS)
    add_library(orchestrator_lib
        src/orchestrator.cpp
        src/task_wrapper.cpp
    )
    
    target_link_libraries(orchestrator_lib
        PUBLIC
            core_lib
    )
    
    message(STATUS "Building: Orchestrator Library")
endif()

# Deploy Manager library (solo se richiesta)
if(BUILD_ALL OR BUILD_DEPLOY_MANAGER)
    add_library(deploy_manager_lib
        src/deploy_manager.cpp
    )
    
    target_link_libraries(deploy_manager_lib
        PUBLIC
            core_lib
    )
    
    message(STATUS "Building: Deploy Manager Library")
endif()

# Builder Manager library (solo se richiesta)
if(BUILD_ALL OR BUILD_BUILDER_MANAGER)
    add_library(builder_manager_lib
        src/builder_manager.cpp
    )
    
    target_link_libraries(builder_manager_lib
        PUBLIC
            yaml-cpp
    )
    
    target_include_directories(builder_manager_lib
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    message(STATUS "Building: Builder Manager Library")
endif()

# ============================================================================
# Executables - Compilazione Condizionale
# ============================================================================

set(INSTALL_TARGETS)

# Orchestrator executable
if(BUILD_ALL OR BUILD_ORCHESTRATOR)
    add_executable(orchestrator_main
        examples/orchestrator_main.cpp
    )
    
    target_link_libraries(orchestrator_main
        PRIVATE
            orchestrator_lib
            core_lib
    )
    
    list(APPEND INSTALL_TARGETS orchestrator_main)
    message(STATUS "Building: orchestrator_main")
endif()

# Deploy Manager executable
if(BUILD_ALL OR BUILD_DEPLOY_MANAGER)
    add_executable(deploy_manager_main
        examples/deploy_manager_main.cpp
    )
    
    target_link_libraries(deploy_manager_main
        PRIVATE
            deploy_manager_lib
            core_lib
    )
    
    list(APPEND INSTALL_TARGETS deploy_manager_main)
    message(STATUS "Building: deploy_manager_main")
endif()

# Builder Manager executable
if(BUILD_ALL OR BUILD_BUILDER_MANAGER)
    add_executable(builder_manager_main
        examples/builder_manager_main.cpp
    )
    
    target_link_libraries(builder_manager_main
        PRIVATE
            builder_manager_lib
    )
    
    list(APPEND INSTALL_TARGETS builder_manager_main)
    message(STATUS "Building: builder_manager_main")
endif()

# Legacy task executable (for backward compatibility - if needed)
# add_executable(task_main
#     examples/task_main.cpp
# )
# 
# target_link_libraries(task_main
#     PRIVATE
#         orchestrator_lib
# )

# ============================================================================
# Modular Task Runner
# ============================================================================

# Generic task runner with modular task definitions
if(BUILD_ALL OR BUILD_TASKS)
    add_executable(task_runner
        tasks/task_runner.cpp
    )
    
    target_link_libraries(task_runner
        PRIVATE
            orchestrator_lib
            core_lib
    )
    
    target_include_directories(task_runner
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/tasks
    )
    
    list(APPEND INSTALL_TARGETS task_runner)
    message(STATUS "Building: task_runner")
endif()

# ============================================================================
# Installation
# ============================================================================

if(INSTALL_TARGETS)
    install(TARGETS ${INSTALL_TARGETS} core_lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
endif()

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
message(STATUS "=== Components to Build ===")
if(BUILD_ALL)
    message(STATUS "Mode: ALL COMPONENTS")
else()
    message(STATUS "Mode: SELECTIVE")
    message(STATUS "  Orchestrator: ${BUILD_ORCHESTRATOR}")
    message(STATUS "  Tasks: ${BUILD_TASKS}")
    message(STATUS "  Deploy Manager: ${BUILD_DEPLOY_MANAGER}")
    message(STATUS "  Builder Manager: ${BUILD_BUILDER_MANAGER}")
endif()
message(STATUS "")
