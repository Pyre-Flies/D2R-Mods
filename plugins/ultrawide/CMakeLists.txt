cmake_minimum_required(VERSION 3.29)
project(ultrawide VERSION 1.3.5 LANGUAGES CXX RC ASM_MASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(D2RLPLUGIN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

set(PLUGIN_SDK "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/D2RLoader-PluginSDK"
    CACHE PATH "D2RLoader PluginSDK")
if(NOT EXISTS "${PLUGIN_SDK}/CMakeLists.txt")
    message(FATAL_ERROR "D2RLoader PluginSDK not found at: ${PLUGIN_SDK}")
endif()
add_subdirectory("${PLUGIN_SDK}" "${CMAKE_CURRENT_BINARY_DIR}/d2rloader-sdk" EXCLUDE_FROM_ALL)
include("${PLUGIN_SDK}/cmake/D2RLPluginEmbedConfig.cmake")

add_library(ultrawide SHARED
    src/plugin.cpp
    src/version.rc
    src/menu_gate.asm
)

d2rlplugin_embed_config(
    TARGET ultrawide
    FILE "${CMAKE_CURRENT_SOURCE_DIR}/ultrawide.toml"
)

target_include_directories(ultrawide PRIVATE
    "${PLUGIN_SDK}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(ultrawide PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_compile_options(ultrawide PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:/W4;/WX;/guard:cf>"
)
target_link_options(ultrawide PRIVATE /guard:cf)
target_link_libraries(ultrawide PRIVATE
    D2RLPlugin::D2RLPlugin
    bcrypt
)

set_target_properties(ultrawide PROPERTIES
    OUTPUT_NAME "Ultrawide"
    PREFIX ""
    SUFFIX ".dll"
)

# Stage the 14 paired 22x22 shader binaries beside the built DLL under data/hd/tc22sh/
add_custom_command(TARGET ultrawide POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:ultrawide>/data/hd/tc22sh"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/tc22sh"
        "$<TARGET_FILE_DIR:ultrawide>/data/hd/tc22sh"
    COMMENT "Staging 14 paired 22x22 terrain shaders (data/hd/tc22sh/)"
)

enable_testing()

foreach(test IN ITEMS artifact config expansion ground_supply)
    add_executable(ultrawide-${test}-tests tests/${test}_tests.cpp)
    target_include_directories(ultrawide-${test}-tests PRIVATE "${PLUGIN_SDK}/include" src)
    target_compile_definitions(ultrawide-${test}-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE)
    target_compile_options(ultrawide-${test}-tests PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/W4;/WX>")
endforeach()

target_link_libraries(ultrawide-expansion-tests PRIVATE bcrypt)
add_dependencies(ultrawide-artifact-tests ultrawide)

add_executable(ultrawide-gates-tests
    tests/gates_tests.cpp
    tests/gates_fixture.asm
    src/menu_gate.asm
)
target_include_directories(ultrawide-gates-tests PRIVATE "${PLUGIN_SDK}/include" src)
target_compile_definitions(ultrawide-gates-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE)
target_compile_options(ultrawide-gates-tests PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/W4;/WX>")

add_test(NAME ultrawide-artifact COMMAND ultrawide-artifact-tests "$<TARGET_FILE:ultrawide>")
add_test(NAME ultrawide-config COMMAND ultrawide-config-tests)
add_test(NAME ultrawide-gates COMMAND ultrawide-gates-tests)
add_test(NAME ultrawide-expansion COMMAND ultrawide-expansion-tests)
add_test(NAME ultrawide-ground-supply COMMAND ultrawide-ground_supply-tests)
