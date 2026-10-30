cmake_minimum_required(VERSION 3.29)
cmake_policy(SET CMP0141 NEW)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
project(pyre-stashsearch VERSION 1.3.1.1 LANGUAGES CXX RC)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(D2RLPLUGIN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(PLUGIN_SDK "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/D2RLoader-PluginSDK" CACHE PATH "D2RLoader PluginSDK")
add_subdirectory("${PLUGIN_SDK}" "${CMAKE_CURRENT_BINARY_DIR}/sdk" EXCLUDE_FROM_ALL)

add_library(pyre-stashsearch SHARED
    src/plugin.cpp
    src/native.cpp
    src/version.rc
)

include("${PLUGIN_SDK}/cmake/D2RLPluginEmbedConfig.cmake")
d2rlplugin_embed_config(
    TARGET pyre-stashsearch
    FILE "${CMAKE_CURRENT_SOURCE_DIR}/pyre-stashsearch.toml"
)

target_include_directories(pyre-stashsearch PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(pyre-stashsearch PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS)
target_compile_options(pyre-stashsearch PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/W4;/WX;/guard:cf;/EHa>")
target_link_options(pyre-stashsearch PRIVATE /guard:cf /PDBALTPATH:%_PDB%)
target_link_libraries(pyre-stashsearch PRIVATE D2RLPlugin::D2RLPlugin user32 imm32)
set_target_properties(pyre-stashsearch PROPERTIES OUTPUT_NAME "pyre-stashsearch" PREFIX "" SUFFIX ".dll")

enable_testing()

add_executable(pyre-stashsearch-unit-tests tests/unit_tests.cpp src/native.cpp)
target_include_directories(pyre-stashsearch-unit-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(pyre-stashsearch-unit-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS)
target_compile_options(pyre-stashsearch-unit-tests PRIVATE /W4 /WX /EHa)
target_link_libraries(pyre-stashsearch-unit-tests PRIVATE D2RLPlugin::D2RLPlugin user32 imm32)
add_test(NAME pyre-stashsearch-unit COMMAND pyre-stashsearch-unit-tests)

add_executable(pyre-stashsearch-artifact-tests tests/artifact_tests.cpp)
target_include_directories(pyre-stashsearch-artifact-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(pyre-stashsearch-artifact-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS)
target_compile_options(pyre-stashsearch-artifact-tests PRIVATE /W4 /WX /EHsc)
target_link_libraries(pyre-stashsearch-artifact-tests PRIVATE D2RLPlugin::D2RLPlugin)
add_dependencies(pyre-stashsearch-artifact-tests pyre-stashsearch)
add_test(NAME pyre-stashsearch-artifact COMMAND pyre-stashsearch-artifact-tests "$<TARGET_FILE:pyre-stashsearch>")
