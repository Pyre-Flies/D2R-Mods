cmake_minimum_required(VERSION 3.29)
project(pyre-lootfilter VERSION 1.3.1 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PLUGIN_SDK "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/D2RLoader-PluginSDK"
    CACHE PATH "D2RLoader PluginSDK")
if(NOT EXISTS "${PLUGIN_SDK}/CMakeLists.txt")
    message(FATAL_ERROR "D2RLoader PluginSDK not found at: ${PLUGIN_SDK}")
endif()

set(CHAT_VENDOR "${CMAKE_CURRENT_SOURCE_DIR}/../always-show-sockets/vendor")

add_library(pyre-lootfilter SHARED
    src/plugin.cpp
    src/version.rc
    "${CHAT_VENDOR}/src/native_commands.cpp"
    "${CHAT_VENDOR}/src/image_scan.cpp"
)

include("${PLUGIN_SDK}/cmake/D2RLPluginEmbedConfig.cmake")
d2rlplugin_embed_config(
    TARGET pyre-lootfilter
    FILE "${CMAKE_CURRENT_SOURCE_DIR}/pyre-lootfilter.toml"
)

target_include_directories(pyre-lootfilter PRIVATE
    "${PLUGIN_SDK}/include"
    "${CHAT_VENDOR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(pyre-lootfilter PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_compile_options(pyre-lootfilter PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:/W4;/WX;/guard:cf>"
)
target_link_options(pyre-lootfilter PRIVATE /guard:cf)
target_link_libraries(pyre-lootfilter PRIVATE bcrypt)

set_target_properties(pyre-lootfilter PROPERTIES
    OUTPUT_NAME "pyre-lootfilter"
    PREFIX ""
    SUFFIX ".dll"
)

enable_testing()

add_executable(pyre-lootfilter-artifact-tests tests/artifact_tests.cpp)
target_include_directories(pyre-lootfilter-artifact-tests PRIVATE "${PLUGIN_SDK}/include")
target_compile_definitions(pyre-lootfilter-artifact-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE)
target_compile_options(pyre-lootfilter-artifact-tests PRIVATE /W4 /WX)
add_dependencies(pyre-lootfilter-artifact-tests pyre-lootfilter)
add_test(NAME pyre-lootfilter-artifact COMMAND pyre-lootfilter-artifact-tests "$<TARGET_FILE:pyre-lootfilter>")

add_executable(pyre-lootfilter-schema-tests tests/schema_and_tables_tests.cpp)
target_include_directories(pyre-lootfilter-schema-tests PRIVATE "${PLUGIN_SDK}/include" "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(pyre-lootfilter-schema-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(pyre-lootfilter-schema-tests PRIVATE /W4 /WX)
add_test(NAME pyre-lootfilter-schema COMMAND pyre-lootfilter-schema-tests)

add_executable(pyre-lootfilter-runtime-tests tests/unit_cache_and_reader_tests.cpp)
target_include_directories(pyre-lootfilter-runtime-tests PRIVATE "${PLUGIN_SDK}/include" "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(pyre-lootfilter-runtime-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(pyre-lootfilter-runtime-tests PRIVATE /W4 /WX)
add_test(NAME pyre-lootfilter-runtime COMMAND pyre-lootfilter-runtime-tests)
