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

# Reuse the tested tab animator at build time; no Inventory Deposit DLL, button
# resources, or artwork conversion are linked into Controller QOL.
set(INVENTORY_DEPOSIT "${CMAKE_CURRENT_SOURCE_DIR}/../inventory-deposit")

add_library(controller-qol SHARED
    src/config_reload.cpp
    src/plugin_compatibility.cpp src/portal_priority.cpp src/materials_actions.cpp
    src/custom_page_actions.cpp
    src/cube_controls.cpp
    src/identify_stat.cpp
    src/native_identify.cpp
    src/client_transfer.cpp
    src/bulk_stash.cpp
    src/bulk_stash_feedback.cpp
    "${INVENTORY_DEPOSIT}/src/stash_feedback.cpp"
    src/belt_compatibility.cpp
    src/vendor_buy.cpp
    src/client_loot.cpp
    src/belt_actions.cpp
    src/aim/controller_aim.cpp
    src/aim/skill_discovery.cpp
    src/loot_run_bridge.asm
    src/placard_post_name.asm
    src/plugin_main.cpp
    src/controller_input.cpp
    src/item_dimensions.cpp
    src/qol_navigation.cpp
    src/qol_glyphs.cpp
    src/controller_qol.rc
)

target_include_directories(controller-qol PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    "${INVENTORY_DEPOSIT}/src"
)

target_compile_definitions(controller-qol PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
    STASH_FEEDBACK_LOG_PREFIX="[QOL/StashFeedback]"
)

target_link_libraries(controller-qol PRIVATE
    D2RLPlugin::D2RLPlugin
    bcrypt
)

d2rlplugin_embed_config(
    TARGET controller-qol
    FILE "${CMAKE_CURRENT_SOURCE_DIR}/controller-qol-updates.toml"
)

# Skill defaults are private to QOL; the loader creates only the shared config.
configure_file(skill-defaults.toml "${CMAKE_CURRENT_BINARY_DIR}/skill-defaults.toml" COPYONLY)
file(TO_CMAKE_PATH "${CMAKE_CURRENT_BINARY_DIR}/skill-defaults.toml" QOL_SKILL_DEFAULTS_PATH)
configure_file(src/skill_defaults.rc.in "${CMAKE_CURRENT_BINARY_DIR}/skill-defaults.rc" @ONLY)
set_property(SOURCE "${CMAKE_CURRENT_BINARY_DIR}/skill-defaults.rc" APPEND PROPERTY OBJECT_DEPENDS "${QOL_SKILL_DEFAULTS_PATH}")
target_sources(controller-qol PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/skill-defaults.rc")

set_target_properties(controller-qol PROPERTIES
    OUTPUT_NAME "Controller QOL Updates"
    PREFIX ""
    SUFFIX ".dll"
)

option(CONTROLLER_QOL_BUILD_TESTS "Build belt contract tests" ON)
if(CONTROLLER_QOL_BUILD_TESTS)
    enable_testing()
    # Compile the actual handler glue against private SDK/native fixtures.
    # The four included .cpp files are excluded from the remaining source list.
    get_target_property(QOL_AUDIT_SOURCES controller-qol SOURCES)
    list(FILTER QOL_AUDIT_SOURCES INCLUDE REGEX "\\.cpp$")
    list(FILTER QOL_AUDIT_SOURCES EXCLUDE REGEX "src/(plugin_main|controller_input|client_loot|qol_navigation)\\.cpp$")
    add_executable(qol-audit-regression-tests tests/audit_regression_tests.cpp src/placard_post_name.asm src/loot_run_bridge.asm ${QOL_AUDIT_SOURCES})
    target_include_directories(qol-audit-regression-tests PRIVATE include src "${INVENTORY_DEPOSIT}/src")
    target_compile_definitions(qol-audit-regression-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(qol-audit-regression-tests PRIVATE D2RLPlugin::D2RLPlugin bcrypt)
    add_test(NAME qol-audit-regressions COMMAND qol-audit-regression-tests)
    add_executable(qol-client-identify-tests tests/client_identify_tests.cpp)
    target_include_directories(qol-client-identify-tests PRIVATE src)
    add_test(NAME qol-client-identify COMMAND qol-client-identify-tests)
    add_executable(qol-client-transfer-tests tests/client_transfer_tests.cpp)
    target_include_directories(qol-client-transfer-tests PRIVATE src "${PLUGIN_SDK}/include")
    target_compile_definitions(qol-client-transfer-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    add_test(NAME qol-client-transfer COMMAND qol-client-transfer-tests)
    add_executable(qol-client-loot-tests tests/client_loot_tests.cpp)
    target_include_directories(qol-client-loot-tests PRIVATE src)
    add_test(NAME qol-client-loot COMMAND qol-client-loot-tests)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/client_loot.cpp")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/client_loot.cpp" loot_source)
    string(FIND "${loot_source}" "bool Refresh(uint32_t radius,bool held) noexcept {" refresh_start)
    string(FIND "${loot_source}" "void Submit(const Request&" refresh_end)
    if(refresh_start LESS 0 OR refresh_end LESS refresh_start)
        message(FATAL_ERROR "Ground refresh fixture boundaries changed")
    endif()
    math(EXPR refresh_length "${refresh_end} - ${refresh_start}")
    string(SUBSTRING "${loot_source}" ${refresh_start} ${refresh_length} refresh_body)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/loot_refresh_runtime.inc" "${refresh_body}")
    add_executable(qol-loot-refresh-work-tests tests/loot_refresh_work_tests.cpp)
    target_include_directories(qol-loot-refresh-work-tests PRIVATE src "${CMAKE_CURRENT_BINARY_DIR}" "${PLUGIN_SDK}/include")
    target_compile_definitions(qol-loot-refresh-work-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    add_test(NAME qol-loot-refresh-work COMMAND qol-loot-refresh-work-tests)
    add_executable(qol-belt-tests tests/belt_tests.cpp)
    target_include_directories(qol-belt-tests PRIVATE src include "${PLUGIN_SDK}/include")
    target_compile_definitions(qol-belt-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    add_test(NAME qol-belt COMMAND qol-belt-tests)
    add_executable(qol-materials-tests tests/materials_tests.cpp)
    target_include_directories(qol-materials-tests PRIVATE src "${PLUGIN_SDK}/include")
    add_test(NAME qol-materials COMMAND qol-materials-tests)
add_executable(qol-materials-focus-tests tests/materials_focus_tests.cpp)
target_link_libraries(qol-materials-focus-tests PRIVATE bcrypt)
target_include_directories(qol-materials-focus-tests PRIVATE src include "${PLUGIN_SDK}/include")
target_compile_definitions(qol-materials-focus-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_test(NAME qol-materials-focus COMMAND qol-materials-focus-tests)
endif()

add_executable(qol-perf-stats-tests tests/perf_stats_tests.cpp)
target_include_directories(qol-perf-stats-tests PRIVATE src)
target_compile_definitions(qol-perf-stats-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_test(NAME qol-perf-stats COMMAND qol-perf-stats-tests)

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

add_executable(qol-relay-allocation-tests tests/relay_allocation_tests.cpp)
target_include_directories(qol-relay-allocation-tests PRIVATE src)
target_compile_definitions(qol-relay-allocation-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(qol-relay-allocation-tests PRIVATE /W4 /WX)
add_test(NAME qol-relay-allocation COMMAND qol-relay-allocation-tests)

add_executable(qol-native-input-tests tests/native_input_tests.cpp)
target_include_directories(qol-native-input-tests PRIVATE src include)
add_test(NAME qol-native-input COMMAND qol-native-input-tests)

add_executable(qol-placard-tests tests/placard_tests.cpp)
target_include_directories(qol-placard-tests PRIVATE src)
add_test(NAME qol-placards COMMAND qol-placard-tests)
add_executable(qol-placard-bridge-tests tests/placard_bridge_tests.cpp tests/placard_bridge_fixture.asm src/placard_post_name.asm)
target_include_directories(qol-placard-bridge-tests PRIVATE src)
add_test(NAME qol-placard-bridge COMMAND qol-placard-bridge-tests)
add_executable(qol-unhoarder-dispatch-tests tests/unhoarder_dispatch_tests.cpp)
target_include_directories(qol-unhoarder-dispatch-tests PRIVATE src)
add_test(NAME qol-unhoarder-dispatch COMMAND qol-unhoarder-dispatch-tests)
add_executable(qol-loot-run-bridge-tests tests/loot_run_bridge_tests.cpp tests/loot_run_bridge_fixture.asm src/loot_run_bridge.asm)
target_include_directories(qol-loot-run-bridge-tests PRIVATE src)
add_test(NAME qol-loot-run-bridge COMMAND qol-loot-run-bridge-tests)

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

add_executable(qol-owned-payloads-tests tests/owned_payloads_tests.cpp)
target_include_directories(qol-owned-payloads-tests PRIVATE src)
add_test(NAME qol-owned-payloads COMMAND qol-owned-payloads-tests)

add_executable(qol-plugin-coexistence-tests tests/plugin_coexistence_tests.cpp)
target_include_directories(qol-plugin-coexistence-tests PRIVATE src)
target_compile_definitions(qol-plugin-coexistence-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(qol-plugin-coexistence-tests PRIVATE bcrypt)
add_test(NAME qol-plugin-coexistence COMMAND qol-plugin-coexistence-tests)

add_executable(qol-maps-quantity-tests tests/maps_quantity_tests.cpp)
target_include_directories(qol-maps-quantity-tests PRIVATE src)
target_compile_definitions(qol-maps-quantity-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_test(NAME qol-maps-quantity COMMAND qol-maps-quantity-tests)

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
add_test(NAME qol-artifact COMMAND qol-artifact-tests "$<TARGET_FILE:controller-qol>" "1.3.1+rev.95")

add_executable(qol-shared-sdk-tests tests/shared_sdk_tests.cpp)
target_include_directories(qol-shared-sdk-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(qol-shared-sdk-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_test(NAME qol-shared-sdk COMMAND qol-shared-sdk-tests)

add_executable(qol-bulk-stash-tests tests/bulk_stash_tests.cpp)
target_include_directories(qol-bulk-stash-tests PRIVATE src "${PLUGIN_SDK}/include")
add_test(NAME qol-bulk-stash COMMAND qol-bulk-stash-tests)

# Exercise QOL's production observer bridge and the shared animator itself.
add_executable(qol-bulk-stash-feedback-tests tests/bulk_stash_feedback_tests.cpp src/bulk_stash_feedback.cpp)
target_include_directories(qol-bulk-stash-feedback-tests PRIVATE include "${INVENTORY_DEPOSIT}/src" "${PLUGIN_SDK}/include")
target_compile_definitions(qol-bulk-stash-feedback-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(qol-bulk-stash-feedback-tests PRIVATE /W4 /WX)
add_test(NAME qol-bulk-stash-feedback COMMAND qol-bulk-stash-feedback-tests)

add_executable(qol-stash-feedback-tests "${INVENTORY_DEPOSIT}/tests/stash_feedback_tests.cpp" "${INVENTORY_DEPOSIT}/src/stash_feedback.cpp")
target_include_directories(qol-stash-feedback-tests PRIVATE "${INVENTORY_DEPOSIT}/src" "${PLUGIN_SDK}/include")
target_compile_definitions(qol-stash-feedback-tests PRIVATE INVENTORY_DEPOSIT_FEEDBACK_TESTS WIN32_LEAN_AND_MEAN NOMINMAX STASH_FEEDBACK_LOG_PREFIX="[QOL/StashFeedback]")
target_compile_options(qol-stash-feedback-tests PRIVATE /W4 /WX)
add_test(NAME qol-stash-feedback COMMAND qol-stash-feedback-tests)

add_executable(qol-stash-feedback-policy-tests "${INVENTORY_DEPOSIT}/tests/stash_feedback_policy_tests.cpp")
target_include_directories(qol-stash-feedback-policy-tests PRIVATE "${INVENTORY_DEPOSIT}/src")
add_test(NAME qol-stash-feedback-policy COMMAND qol-stash-feedback-policy-tests)

# Integrated aim policy and configuration isolation regression coverage.
add_executable(qol-aim-tests tests/aim_policy_tests.cpp)
target_include_directories(qol-aim-tests PRIVATE src/aim)
target_compile_options(qol-aim-tests PRIVATE /W4 /WX)
add_test(NAME qol-aim COMMAND qol-aim-tests)
set_source_files_properties(src/aim/controller_aim.cpp PROPERTIES COMPILE_OPTIONS "/W4;/WX")

add_executable(qol-aim-gating-tests tests/aim_gating_tests.cpp src/aim/controller_aim.cpp src/aim/skill_discovery.cpp)
target_include_directories(qol-aim-gating-tests PRIVATE src "${PLUGIN_SDK}/include")
target_include_directories(qol-aim-gating-tests PRIVATE include)
target_compile_definitions(qol-aim-gating-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX QOL_AIM_TEST_INSTALL)
target_compile_options(qol-aim-gating-tests PRIVATE /W4 /WX)
target_link_libraries(qol-aim-gating-tests PRIVATE bcrypt)
add_test(NAME qol-aim-gating COMMAND qol-aim-gating-tests)

add_executable(qol-config-reload-tests tests/config_reload_tests.cpp)
target_include_directories(qol-config-reload-tests PRIVATE src include "${PLUGIN_SDK}/include")
target_compile_definitions(qol-config-reload-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
target_compile_options(qol-config-reload-tests PRIVATE /W4 /WX)
add_test(NAME qol-config-reload COMMAND qol-config-reload-tests)

add_executable(qol-skill-discovery-tests tests/skill_discovery_tests.cpp src/aim/skill_discovery.cpp)
target_include_directories(qol-skill-discovery-tests PRIVATE src/aim "${PLUGIN_SDK}/include")
target_compile_definitions(qol-skill-discovery-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(qol-skill-discovery-tests PRIVATE /W4 /WX)
set_source_files_properties(src/aim/skill_discovery.cpp PROPERTIES COMPILE_OPTIONS "/W4;/WX")
add_test(NAME qol-skill-discovery COMMAND qol-skill-discovery-tests)
target_sources(qol-aim-gating-tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/skill-defaults.rc")
target_sources(qol-skill-discovery-tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/skill-defaults.rc")

add_executable(qol-item-code-contract-tests tests/item_code_contract_tests.cpp)
target_include_directories(qol-item-code-contract-tests PRIVATE src)
target_compile_options(qol-item-code-contract-tests PRIVATE /W4 /WX)
target_compile_definitions(qol-item-code-contract-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_test(NAME qol-item-code-contract COMMAND qol-item-code-contract-tests)

set_source_files_properties(src/cube_controls.cpp PROPERTIES COMPILE_OPTIONS "/W4;/WX")
add_executable(qol-cube-controls-tests tests/cube_controls_tests.cpp)
target_include_directories(qol-cube-controls-tests PRIVATE src)
target_compile_options(qol-cube-controls-tests PRIVATE /W4 /WX)
add_test(NAME qol-cube-controls COMMAND qol-cube-controls-tests)
add_executable(qol-cube-native-calls-tests tests/cube_native_calls_tests.cpp)
target_include_directories(qol-cube-native-calls-tests PRIVATE src)
target_compile_options(qol-cube-native-calls-tests PRIVATE /W4 /WX)
add_test(NAME qol-cube-native-calls COMMAND qol-cube-native-calls-tests)

# New exact companion profiles retain independent regression coverage.
add_executable(qol-item-code-background-tests tests/item_code_contract_tests_background.cpp)
target_include_directories(qol-item-code-background-tests PRIVATE src)
add_test(NAME qol-item-code-background COMMAND qol-item-code-background-tests)
add_executable(qol-unhoarder-dispatch-background-tests tests/unhoarder_dispatch_tests_background.cpp)
target_include_directories(qol-unhoarder-dispatch-background-tests PRIVATE src)
add_test(NAME qol-unhoarder-dispatch-background COMMAND qol-unhoarder-dispatch-background-tests)

add_executable(qol-unhoarder-loaded-admission-tests tests/unhoarder_loaded_admission_tests.cpp)
target_include_directories(qol-unhoarder-loaded-admission-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(qol-unhoarder-loaded-admission-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(qol-unhoarder-loaded-admission-tests PRIVATE bcrypt)
# Optional legacy artifacts are supplied by the tester, never bundled here.
set(QOL_UNHOARDER_LEGACY_DLL "" CACHE FILEPATH "Reviewed original companion fixture")
set(QOL_UNHOARDER_BACKGROUND_DLL "" CACHE FILEPATH "Reviewed background-fix companion fixture")
if(QOL_UNHOARDER_LEGACY_DLL)
 add_test(NAME qol-unhoarder-loaded-old COMMAND qol-unhoarder-loaded-admission-tests "${QOL_UNHOARDER_LEGACY_DLL}" old)
endif()
if(QOL_UNHOARDER_BACKGROUND_DLL)
 add_test(NAME qol-unhoarder-loaded-new COMMAND qol-unhoarder-loaded-admission-tests "${QOL_UNHOARDER_BACKGROUND_DLL}" new)
endif()

add_executable(qol-item-code-controller-tests tests/item_code_contract_tests_controller.cpp)
target_include_directories(qol-item-code-controller-tests PRIVATE src)
add_test(NAME qol-item-code-controller COMMAND qol-item-code-controller-tests)
add_executable(qol-unhoarder-dispatch-controller-tests tests/unhoarder_dispatch_tests_controller.cpp)
target_include_directories(qol-unhoarder-dispatch-controller-tests PRIVATE src)
add_test(NAME qol-unhoarder-dispatch-controller COMMAND qol-unhoarder-dispatch-controller-tests)
add_executable(qol-unhoarder-controller-admission-tests tests/unhoarder_controller_admission_tests.cpp)
target_include_directories(qol-unhoarder-controller-admission-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(qol-unhoarder-controller-admission-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(qol-unhoarder-controller-admission-tests PRIVATE bcrypt)
set(QOL_UNHOARDER_CONTROLLER_DLL "" CACHE FILEPATH "Reviewed controller label companion")
if(QOL_UNHOARDER_CONTROLLER_DLL)
 add_test(NAME qol-unhoarder-controller-actual COMMAND qol-unhoarder-controller-admission-tests "${QOL_UNHOARDER_CONTROLLER_DLL}" new)
endif()

add_executable(qol-item-code-drop-tests tests/item_code_contract_tests_drop.cpp)
target_include_directories(qol-item-code-drop-tests PRIVATE src)
add_test(NAME qol-item-code-drop COMMAND qol-item-code-drop-tests)
add_executable(qol-unhoarder-dispatch-drop-tests tests/unhoarder_dispatch_tests_drop.cpp)
target_include_directories(qol-unhoarder-dispatch-drop-tests PRIVATE src)
add_test(NAME qol-unhoarder-dispatch-drop COMMAND qol-unhoarder-dispatch-drop-tests)
add_executable(qol-unhoarder-drop-admission-tests tests/unhoarder_drop_admission_tests.cpp)
target_include_directories(qol-unhoarder-drop-admission-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(qol-unhoarder-drop-admission-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(qol-unhoarder-drop-admission-tests PRIVATE bcrypt)
set(QOL_UNHOARDER_DROP_DLL "" CACHE FILEPATH "Reviewed drop label companion")
if(QOL_UNHOARDER_DROP_DLL)
 add_test(NAME qol-unhoarder-drop-actual COMMAND qol-unhoarder-drop-admission-tests "${QOL_UNHOARDER_DROP_DLL}" new)
endif()

add_executable(qol-item-code-socket-tests tests/item_code_contract_tests_socket.cpp)
target_include_directories(qol-item-code-socket-tests PRIVATE src)
add_test(NAME qol-item-code-socket COMMAND qol-item-code-socket-tests)
add_executable(qol-unhoarder-dispatch-socket-tests tests/unhoarder_dispatch_tests_socket.cpp)
target_include_directories(qol-unhoarder-dispatch-socket-tests PRIVATE src)
add_test(NAME qol-unhoarder-dispatch-socket COMMAND qol-unhoarder-dispatch-socket-tests)
add_executable(qol-unhoarder-socket-admission-tests tests/unhoarder_socket_admission_tests.cpp)
target_include_directories(qol-unhoarder-socket-admission-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(qol-unhoarder-socket-admission-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(qol-unhoarder-socket-admission-tests PRIVATE bcrypt)
set(QOL_UNHOARDER_SOCKET_DLL "" CACHE FILEPATH "Reviewed socket label companion")
if(QOL_UNHOARDER_SOCKET_DLL)
 add_test(NAME qol-unhoarder-socket-actual COMMAND qol-unhoarder-socket-admission-tests "${QOL_UNHOARDER_SOCKET_DLL}" new)
endif()

add_executable(qol-item-code-label-only-tests tests/item_code_contract_tests_label_only.cpp)
target_include_directories(qol-item-code-label-only-tests PRIVATE src)
add_test(NAME qol-item-code-label-only COMMAND qol-item-code-label-only-tests)
add_executable(qol-unhoarder-label-only-admission-tests tests/unhoarder_label_only_admission_tests.cpp)
target_include_directories(qol-unhoarder-label-only-admission-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(qol-unhoarder-label-only-admission-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(qol-unhoarder-label-only-admission-tests PRIVATE bcrypt)
set(QOL_UNHOARDER_LABEL_ONLY_DLL "" CACHE FILEPATH "Reviewed presentation-only hide companion")
if(QOL_UNHOARDER_LABEL_ONLY_DLL)
 add_test(NAME qol-unhoarder-label-only-actual COMMAND qol-unhoarder-label-only-admission-tests "${QOL_UNHOARDER_LABEL_ONLY_DLL}")
endif()

add_executable(qol-item-code-visibility-tests tests/item_code_contract_tests_visibility.cpp)
target_include_directories(qol-item-code-visibility-tests PRIVATE src)
add_test(NAME qol-item-code-visibility COMMAND qol-item-code-visibility-tests)
add_executable(qol-unhoarder-visibility-admission-tests tests/unhoarder_visibility_admission_tests.cpp)
target_include_directories(qol-unhoarder-visibility-admission-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(qol-unhoarder-visibility-admission-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(qol-unhoarder-visibility-admission-tests PRIVATE bcrypt)
set(QOL_UNHOARDER_VISIBILITY_DLL "" CACHE FILEPATH "Reviewed presentation-only hide companion")
if(QOL_UNHOARDER_VISIBILITY_DLL)
 add_test(NAME qol-unhoarder-visibility-actual COMMAND qol-unhoarder-visibility-admission-tests "${QOL_UNHOARDER_VISIBILITY_DLL}")
endif()

add_executable(qol-loot-visibility-tests tests/loot_visibility_tests.cpp)
target_include_directories(qol-loot-visibility-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(qol-loot-visibility-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_test(NAME qol-loot-visibility COMMAND qol-loot-visibility-tests)
add_executable(qol-native-visibility-wiring-tests tests/native_visibility_wiring_tests.cpp)
target_include_directories(qol-native-visibility-wiring-tests PRIVATE src)
add_test(NAME qol-native-visibility-wiring COMMAND qol-native-visibility-wiring-tests)

add_executable(qol-item-code-opacity-tests tests/item_code_contract_tests_opacity.cpp)
target_include_directories(qol-item-code-opacity-tests PRIVATE src)
add_test(NAME qol-item-code-opacity COMMAND qol-item-code-opacity-tests)
add_executable(qol-unhoarder-opacity-admission-tests tests/unhoarder_opacity_admission_tests.cpp)
target_include_directories(qol-unhoarder-opacity-admission-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(qol-unhoarder-opacity-admission-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(qol-unhoarder-opacity-admission-tests PRIVATE bcrypt)
set(QOL_UNHOARDER_OPACITY_DLL "" CACHE FILEPATH "Reviewed presentation-only hide companion")
if(QOL_UNHOARDER_OPACITY_DLL)
 add_test(NAME qol-unhoarder-opacity-actual COMMAND qol-unhoarder-opacity-admission-tests "${QOL_UNHOARDER_OPACITY_DLL}")
endif()

add_executable(qol-unhoarder-polling-admission-tests tests/unhoarder_opacity_admission_tests.cpp)
target_include_directories(qol-unhoarder-polling-admission-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(qol-unhoarder-polling-admission-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX QOL_TEST_POLLING_PROFILE)
target_link_libraries(qol-unhoarder-polling-admission-tests PRIVATE bcrypt)
set(QOL_UNHOARDER_POLLING_DLL "" CACHE FILEPATH "Reviewed inventory-polling-fix companion")
if(QOL_UNHOARDER_POLLING_DLL)
 add_test(NAME qol-unhoarder-polling-actual COMMAND qol-unhoarder-polling-admission-tests "${QOL_UNHOARDER_POLLING_DLL}")
endif()

add_executable(qol-guarded-local-read-tests tests/guarded_local_read_tests.cpp)
target_include_directories(qol-guarded-local-read-tests PRIVATE src)
target_compile_definitions(qol-guarded-local-read-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(qol-guarded-local-read-tests PRIVATE /W4 /WX /EHsc)
add_test(NAME qol-guarded-local-read COMMAND qol-guarded-local-read-tests)

add_executable(qol-image-header-work-tests tests/image_header_work_tests.cpp)
target_include_directories(qol-image-header-work-tests PRIVATE src)
target_compile_definitions(qol-image-header-work-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_test(NAME qol-image-header-work COMMAND qol-image-header-work-tests)
add_executable(qol-current-page-query-tests tests/current_page_query_tests.cpp)
target_include_directories(qol-current-page-query-tests PRIVATE src)
target_compile_definitions(qol-current-page-query-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_test(NAME qol-current-page-query COMMAND qol-current-page-query-tests)

add_executable(qol-unhoarder-native-reads-admission-tests tests/unhoarder_opacity_admission_tests.cpp)
target_include_directories(qol-unhoarder-native-reads-admission-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(qol-unhoarder-native-reads-admission-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX QOL_TEST_NATIVE_READ_PROFILE)
target_link_libraries(qol-unhoarder-native-reads-admission-tests PRIVATE bcrypt)
set(QOL_UNHOARDER_NATIVE_READS_DLL "" CACHE FILEPATH "Reviewed active-filter native-read companion")
if(QOL_UNHOARDER_NATIVE_READS_DLL)
 add_test(NAME qol-unhoarder-native-reads-actual COMMAND qol-unhoarder-native-reads-admission-tests "${QOL_UNHOARDER_NATIVE_READS_DLL}")
endif()

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/client_loot.cpp" label_track_source)
string(FIND "${label_track_source}" "void Track(" label_track_start)
string(FIND "${label_track_source}" "int SlotFor(" label_track_end)
if(label_track_start LESS 0 OR label_track_end LESS label_track_start)
 message(FATAL_ERROR "Label tracking fixture boundaries changed")
endif()
math(EXPR label_track_length "${label_track_end} - ${label_track_start}")
string(SUBSTRING "${label_track_source}" ${label_track_start} ${label_track_length} label_track_body)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/label_track_runtime.inc" "${label_track_body}")
add_executable(qol-label-visibility-work-tests tests/label_visibility_work_tests.cpp)
target_include_directories(qol-label-visibility-work-tests PRIVATE src "${CMAKE_CURRENT_BINARY_DIR}" "${PLUGIN_SDK}/include")
target_compile_definitions(qol-label-visibility-work-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_test(NAME qol-label-visibility-work COMMAND qol-label-visibility-work-tests)

add_executable(qol-unhoarder-label-preflight-admission-tests tests/unhoarder_opacity_admission_tests.cpp)
target_include_directories(qol-unhoarder-label-preflight-admission-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(qol-unhoarder-label-preflight-admission-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX QOL_TEST_LABEL_PREFLIGHT_PROFILE)
target_link_libraries(qol-unhoarder-label-preflight-admission-tests PRIVATE bcrypt)
set(QOL_UNHOARDER_LABEL_PREFLIGHT_DLL "" CACHE FILEPATH "Reviewed shared-label preflight companion")
if(QOL_UNHOARDER_LABEL_PREFLIGHT_DLL)
 add_test(NAME qol-unhoarder-label-preflight-actual COMMAND qol-unhoarder-label-preflight-admission-tests "${QOL_UNHOARDER_LABEL_PREFLIGHT_DLL}")
endif()

add_executable(qol-unhoarder-label-work-admission-tests tests/unhoarder_opacity_admission_tests.cpp)
target_include_directories(qol-unhoarder-label-work-admission-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(qol-unhoarder-label-work-admission-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX QOL_TEST_LABEL_WORK_PROFILE)
target_link_libraries(qol-unhoarder-label-work-admission-tests PRIVATE bcrypt)
set(QOL_UNHOARDER_LABEL_WORK_DLL "" CACHE FILEPATH "Reviewed label-work companion")
if(QOL_UNHOARDER_LABEL_WORK_DLL)
 add_test(NAME qol-unhoarder-label-work-actual COMMAND qol-unhoarder-label-work-admission-tests "${QOL_UNHOARDER_LABEL_WORK_DLL}")
endif()

add_executable(qol-unhoarder-formatter-observation-admission-tests tests/unhoarder_opacity_admission_tests.cpp)
target_include_directories(qol-unhoarder-formatter-observation-admission-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(qol-unhoarder-formatter-observation-admission-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX QOL_TEST_FORMATTER_OBSERVATION_PROFILE)
target_link_libraries(qol-unhoarder-formatter-observation-admission-tests PRIVATE bcrypt)
set(QOL_UNHOARDER_FORMATTER_OBSERVATION_DLL "" CACHE FILEPATH "Reviewed formatter-observation companion")
if(QOL_UNHOARDER_FORMATTER_OBSERVATION_DLL)
 add_test(NAME qol-unhoarder-formatter-observation-actual COMMAND qol-unhoarder-formatter-observation-admission-tests "${QOL_UNHOARDER_FORMATTER_OBSERVATION_DLL}")
endif()

add_executable(qol-unhoarder-resident-pages-admission-tests tests/unhoarder_opacity_admission_tests.cpp)
target_include_directories(qol-unhoarder-resident-pages-admission-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(qol-unhoarder-resident-pages-admission-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX QOL_TEST_RESIDENT_PAGES_PROFILE)
target_link_libraries(qol-unhoarder-resident-pages-admission-tests PRIVATE bcrypt)
set(QOL_UNHOARDER_RESIDENT_PAGES_DLL "" CACHE FILEPATH "Reviewed resident-page companion")
if(QOL_UNHOARDER_RESIDENT_PAGES_DLL)
 add_test(NAME qol-unhoarder-resident-pages-actual COMMAND qol-unhoarder-resident-pages-admission-tests "${QOL_UNHOARDER_RESIDENT_PAGES_DLL}")
endif()

add_executable(qol-unhoarder-socket-batch-alt-admission-tests tests/unhoarder_opacity_admission_tests.cpp)
target_include_directories(qol-unhoarder-socket-batch-alt-admission-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(qol-unhoarder-socket-batch-alt-admission-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX QOL_TEST_SOCKET_BATCH_ALT_PROFILE)
target_link_libraries(qol-unhoarder-socket-batch-alt-admission-tests PRIVATE bcrypt)
add_executable(qol-unhoarder-dispatch-socket-batch-alt-tests tests/unhoarder_dispatch_tests_socket_batch_alt.cpp)
target_include_directories(qol-unhoarder-dispatch-socket-batch-alt-tests PRIVATE src)
add_test(NAME qol-unhoarder-dispatch-socket-batch-alt COMMAND qol-unhoarder-dispatch-socket-batch-alt-tests)
add_executable(qol-unhoarder-dispatch-socket-batch-alt-admission-tests tests/unhoarder_socket_batch_alt_admission_tests.cpp)
target_include_directories(qol-unhoarder-dispatch-socket-batch-alt-admission-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(qol-unhoarder-dispatch-socket-batch-alt-admission-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(qol-unhoarder-dispatch-socket-batch-alt-admission-tests PRIVATE bcrypt)
set(QOL_UNHOARDER_SOCKET_BATCH_ALT_DLL "" CACHE FILEPATH "Reviewed socket batching and ALT pickup companion")
if(QOL_UNHOARDER_SOCKET_BATCH_ALT_DLL)
 add_test(NAME qol-unhoarder-socket-batch-alt-actual COMMAND qol-unhoarder-socket-batch-alt-admission-tests "${QOL_UNHOARDER_SOCKET_BATCH_ALT_DLL}")
 add_test(NAME qol-unhoarder-dispatch-socket-batch-alt-actual COMMAND qol-unhoarder-dispatch-socket-batch-alt-admission-tests "${QOL_UNHOARDER_SOCKET_BATCH_ALT_DLL}" new)
endif()
