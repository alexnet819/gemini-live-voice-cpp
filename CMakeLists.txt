cmake_minimum_required(VERSION 3.15)
project(gemini-voice VERSION 1.0.0 LANGUAGES CXX)

# Set CMake policies to suppress warnings (version-gated for compatibility)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.27")
    cmake_policy(SET CMP0144 NEW)  # Use upper-case <PACKAGENAME>_ROOT variables
endif()
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.30")
    cmake_policy(SET CMP0167 NEW)  # Use BoostConfig.cmake instead of FindBoost
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
# Note: Modern Boost.Asio and Boost.Beast are header-only (since 1.69+)
# so we don't need compiled components like 'system'
find_package(Boost 1.70 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Find miniaudio from third_party
set(MINIAUDIO_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/miniaudio")

if(NOT EXISTS ${MINIAUDIO_INCLUDE_DIR}/miniaudio.h)
    message(FATAL_ERROR "miniaudio not found in third_party/miniaudio.")
endif()

# Find nlohmann_json from third_party
set(NLOHMANN_JSON_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party")

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/third_party/nlohmann/json.hpp)
    message(FATAL_ERROR "nlohmann/json not found in third_party/nlohmann.")
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${MINIAUDIO_INCLUDE_DIR}
    ${NLOHMANN_JSON_INCLUDE_DIR}
)

# Source files
set(SOURCES
    src/main.cpp
    src/websocket_client.cpp
    src/audio_handler.cpp
    src/message_handler.cpp
    src/config.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    Boost::headers
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    pthread
    m
    dl
)

# Test executables
add_executable(test_audio tests/test_audio.cpp src/audio_handler.cpp)
target_link_libraries(test_audio
    Threads::Threads
    pthread
    m
    dl
)

add_executable(test_websocket tests/test_websocket.cpp src/websocket_client.cpp src/message_handler.cpp)
target_link_libraries(test_websocket
    Boost::headers
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    pthread
    m
    dl
)

add_executable(test_playback tests/test_playback.cpp)
target_link_libraries(test_playback
    Threads::Threads
    pthread
    m
    dl
)

# Compiler options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -pedantic
    )
endif()

# Installation
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
