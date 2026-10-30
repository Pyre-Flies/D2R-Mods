cmake_minimum_required(VERSION 3.29)
project(running-defense VERSION 1.3.1.1 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PLUGIN_SDK "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/D2RLoader-PluginSDK" CACHE PATH "D2RLoader PluginSDK")

add_library(running-defense SHARED
    src/plugin.cpp
    src/version.rc
)

include("${PLUGIN_SDK}/cmake/D2RLPluginEmbedConfig.cmake")
d2rlplugin_embed_config(TARGET running-defense FILE "${CMAKE_CURRENT_SOURCE_DIR}/running-defense.toml")

target_include_directories(running-defense PRIVATE "${PLUGIN_SDK}/include" src)
target_compile_definitions(running-defense PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(running-defense PRIVATE /W4 /WX /guard:cf)
target_link_options(running-defense PRIVATE /guard:cf)
target_link_libraries(running-defense PRIVATE bcrypt)
set_target_properties(running-defense PROPERTIES OUTPUT_NAME "Running Defense" PREFIX "")

enable_testing()

add_executable(running-defense-config-tests tests/config_tests.cpp)
target_include_directories(running-defense-config-tests PRIVATE src)
target_compile_options(running-defense-config-tests PRIVATE /W4 /WX)
add_test(NAME running-defense-config COMMAND running-defense-config-tests)

add_executable(running-defense-artifact-tests tests/artifact_tests.cpp)
target_include_directories(running-defense-artifact-tests PRIVATE "${PLUGIN_SDK}/include" src)
target_compile_definitions(running-defense-artifact-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE)
target_compile_options(running-defense-artifact-tests PRIVATE /W4 /WX)
add_dependencies(running-defense-artifact-tests running-defense)
add_test(NAME running-defense-artifact COMMAND running-defense-artifact-tests "$<TARGET_FILE:running-defense>")
