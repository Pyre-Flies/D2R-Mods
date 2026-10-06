cmake_minimum_required(VERSION 3.29)
project(controller-qol LANGUAGES CXX RC ASM_MASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Disable building SDK examples and tests
set(D2RLPLUGIN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

# Use the repository's single reviewed SDK copy by default. A caller may still
# provide -DPLUGIN_SDK=<path> for an independently checked SDK checkout.
set(PLUGIN_SDK "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/D2RLoader-PluginSDK"
    CACHE PATH "D2RLoader PluginSDK")
if(NOT EXISTS "${PLUGIN_SDK}/CMakeLists.txt")
    message(FATAL_ERROR "D2RLoader PluginSDK not found at: ${PLUGIN_SDK}")
endif()
add_subdirectory("${PLUGIN_SDK}" "${CMAKE_CURRENT_BINARY_DIR}/d2rloader-sdk" EXCLUDE_FROM_ALL)

include("${PLUGIN_SDK}/cmake/D2RLPluginEmbedConfig.cmake")

add_library(controller-qol SHARED
    src/plugin_compatibility.cpp src/portal_priority.cpp src/materials_actions.cpp
    src/custom_page_actions.cpp
    src/identify_stat.cpp
    src/native_identify.cpp
    src/bulk_stash.cpp
    src/belt_compatibility.cpp
    src/vendor_buy.cpp
    src/belt_actions.cpp
    src/aim/controller_aim.cpp
    src/plugin_main.cpp
    src/controller_input.cpp
    src/d2r_memory.cpp
    src/item_dimensions.cpp
    src/qol_navigation.cpp
    src/qol_glyphs.cpp
    src/controller_qol.rc
)

target_include_directories(controller-qol PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(controller-qol PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)

target_link_libraries(controller-qol PRIVATE
    D2RLPlugin::D2RLPlugin
    bcrypt
)

d2rlplugin_embed_config(
    TARGET controller-qol
    FILE "${CMAKE_CURRENT_SOURCE_DIR}/controller-qol-updates.toml"
)

set_target_properties(controller-qol PROPERTIES
    OUTPUT_NAME "Controller QOL Updates"
    PREFIX ""
    SUFFIX ".dll"
)

option(CONTROLLER_QOL_BUILD_TESTS "Build belt contract tests" ON)
if(CONTROLLER_QOL_BUILD_TESTS)
    enable_testing()
    add_executable(qol-belt-tests tests/belt_tests.cpp)
    target_include_directories(qol-belt-tests PRIVATE src include "${PLUGIN_SDK}/include")
    target_compile_definitions(qol-belt-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    add_test(NAME qol-belt COMMAND qol-belt-tests)
    add_executable(qol-materials-tests tests/materials_tests.cpp)
    target_include_directories(qol-materials-tests PRIVATE src "${PLUGIN_SDK}/include")
    add_test(NAME qol-materials COMMAND qol-materials-tests)
endif()

enable_testing()
add_executable(qol-shared-page-tests tests/shared_page_tests.cpp)
target_include_directories(qol-shared-page-tests PRIVATE src)
target_compile_features(qol-shared-page-tests PRIVATE cxx_std_20)
add_test(NAME qol-shared-pages COMMAND qol-shared-page-tests)

add_executable(qol-portal-tests tests/portal_tests.cpp)
target_include_directories(qol-portal-tests PRIVATE src)
add_test(NAME qol-portal COMMAND qol-portal-tests)

add_library(qol-compatibility-fixture SHARED tests/compatibility_fixture.cpp)
target_include_directories(qol-compatibility-fixture PRIVATE "${PLUGIN_SDK}/include")
add_executable(qol-compatibility-tests tests/compatibility_tests.cpp src/plugin_compatibility.cpp)
target_include_directories(qol-compatibility-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(qol-compatibility-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_dependencies(qol-compatibility-tests qol-compatibility-fixture)
add_test(NAME qol-compatibility COMMAND qol-compatibility-tests "$<TARGET_FILE:qol-compatibility-fixture>")

add_executable(qol-guided-arrow-tests tests/guided_arrow_projection_tests.cpp)
target_include_directories(qol-guided-arrow-tests PRIVATE src)
add_test(NAME qol-guided-arrow COMMAND qol-guided-arrow-tests)

add_executable(qol-core-guard-tests tests/core_guard_tests.cpp src/controller_input.cpp)
target_include_directories(qol-core-guard-tests PRIVATE src include "${PLUGIN_SDK}/include")
target_compile_definitions(qol-core-guard-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(qol-core-guard-tests PRIVATE bcrypt)
add_test(NAME qol-core-guard COMMAND qol-core-guard-tests)

add_executable(qol-shared-owner-tests tests/shared_owner_tests.cpp)
target_include_directories(qol-shared-owner-tests PRIVATE src)
target_compile_definitions(qol-shared-owner-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_test(NAME qol-shared-owner COMMAND qol-shared-owner-tests)

add_executable(qol-vendor-compatibility-tests tests/vendor_compatibility_tests.cpp)
target_include_directories(qol-vendor-compatibility-tests PRIVATE src)
target_compile_definitions(qol-vendor-compatibility-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_test(NAME qol-vendor-compatibility COMMAND qol-vendor-compatibility-tests)

add_executable(qol-xinput-hook-tests tests/xinput_hook_tests.cpp tests/xinput_fixture.asm)
target_include_directories(qol-xinput-hook-tests PRIVATE src)
target_compile_definitions(qol-xinput-hook-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_test(NAME qol-xinput-hook COMMAND qol-xinput-hook-tests)

add_executable(qol-glyph-call-tests tests/glyph_call_tests.cpp)
target_include_directories(qol-glyph-call-tests PRIVATE src)
add_test(NAME qol-glyph-calls COMMAND qol-glyph-call-tests)

add_executable(qol-native-input-tests tests/native_input_tests.cpp)
target_include_directories(qol-native-input-tests PRIVATE src include)
add_test(NAME qol-native-input COMMAND qol-native-input-tests)

add_executable(qol-placard-tests tests/placard_tests.cpp)
target_include_directories(qol-placard-tests PRIVATE src)
add_test(NAME qol-placards COMMAND qol-placard-tests)

add_executable(qol-custom-page-tests tests/custom_page_tests.cpp src/custom_page_actions.cpp)
target_include_directories(qol-custom-page-tests PRIVATE src include "${PLUGIN_SDK}/include")
target_compile_definitions(qol-custom-page-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(qol-custom-page-tests PRIVATE bcrypt)
add_test(NAME qol-custom-page COMMAND qol-custom-page-tests)
add_executable(qol-menu-route-tests tests/menu_route_tests.cpp)
target_include_directories(qol-menu-route-tests PRIVATE src)
add_test(NAME qol-menu-route COMMAND qol-menu-route-tests)

add_executable(qol-identify-tests tests/identify_tests.cpp)
target_include_directories(qol-identify-tests PRIVATE src "${PLUGIN_SDK}/include")
add_test(NAME qol-identify COMMAND qol-identify-tests)

add_executable(qol-move-identity-tests tests/move_identity_tests.cpp)
target_include_directories(qol-move-identity-tests PRIVATE src "${PLUGIN_SDK}/include")
add_test(NAME qol-move-identity COMMAND qol-move-identity-tests)

add_executable(qol-belt-compatibility-tests tests/belt_compatibility_tests.cpp src/belt_compatibility.cpp)
target_include_directories(qol-belt-compatibility-tests PRIVATE src)
target_compile_definitions(qol-belt-compatibility-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(qol-belt-compatibility-tests PRIVATE bcrypt)
add_test(NAME qol-belt-compatibility COMMAND qol-belt-compatibility-tests)

# Keep the shipped resource/exports/defaults in the routine regression run.
add_executable(qol-artifact-tests tests/artifact_tests.cpp)
target_include_directories(qol-artifact-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(qol-artifact-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_dependencies(qol-artifact-tests controller-qol)
add_test(NAME qol-artifact COMMAND qol-artifact-tests "$<TARGET_FILE:controller-qol>" "1.3.1+rev.61")

add_executable(qol-shared-sdk-tests tests/shared_sdk_tests.cpp)
target_include_directories(qol-shared-sdk-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(qol-shared-sdk-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_test(NAME qol-shared-sdk COMMAND qol-shared-sdk-tests)

add_executable(qol-bulk-stash-tests tests/bulk_stash_tests.cpp)
target_include_directories(qol-bulk-stash-tests PRIVATE src "${PLUGIN_SDK}/include")
add_test(NAME qol-bulk-stash COMMAND qol-bulk-stash-tests)

# Integrated aim policy and configuration isolation regression coverage.
add_executable(qol-aim-tests tests/aim_policy_tests.cpp)
target_include_directories(qol-aim-tests PRIVATE src/aim)
target_compile_options(qol-aim-tests PRIVATE /W4 /WX)
add_test(NAME qol-aim COMMAND qol-aim-tests)
set_source_files_properties(src/aim/controller_aim.cpp PROPERTIES COMPILE_OPTIONS "/W4;/WX")

add_executable(qol-aim-gating-tests tests/aim_gating_tests.cpp src/aim/controller_aim.cpp)
target_include_directories(qol-aim-gating-tests PRIVATE src "${PLUGIN_SDK}/include")
target_include_directories(qol-aim-gating-tests PRIVATE include)
target_compile_definitions(qol-aim-gating-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX QOL_AIM_TEST_INSTALL)
target_compile_options(qol-aim-gating-tests PRIVATE /W4 /WX)
target_link_libraries(qol-aim-gating-tests PRIVATE bcrypt)
add_test(NAME qol-aim-gating COMMAND qol-aim-gating-tests)
