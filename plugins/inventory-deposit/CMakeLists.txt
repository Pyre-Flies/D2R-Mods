cmake_minimum_required(VERSION 3.29)
# Embedded debug information avoids depending on an external PDB server for
# compiler probes and Debug builds; Release artifacts retain normal flags.
cmake_policy(SET CMP0141 NEW)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
project(inventory-deposit VERSION 1.3.1.13 LANGUAGES CXX RC)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(D2RLPLUGIN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(PLUGIN_SDK "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/D2RLoader-PluginSDK" CACHE PATH "D2RLoader PluginSDK")
add_subdirectory("${PLUGIN_SDK}" "${CMAKE_CURRENT_BINARY_DIR}/sdk" EXCLUDE_FROM_ALL)
include("${PLUGIN_SDK}/cmake/D2RLPluginEmbedConfig.cmake")
set(QOL "${CMAKE_CURRENT_SOURCE_DIR}/../controller-qol")

# Original source art is converted during the build, then embedded as RCDATA.
# Only the DLL is needed at runtime; generated sprites stay in the build tree.
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
set(BUTTON_ART_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/art")
set(BUTTON_ART_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/button-art")
set(BUTTON_ART_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tools/build_button_sprites.py")
add_custom_command(
    OUTPUT "${BUTTON_ART_GENERATED}/button_art.rc"
        "${BUTTON_ART_GENERATED}/deposit-button.sprite"
        "${BUTTON_ART_GENERATED}/deposit-button.lowend.sprite"
        "${BUTTON_ART_GENERATED}/button-sprites-manifest.json"
    COMMAND "${Python3_EXECUTABLE}" "${BUTTON_ART_SCRIPT}"
        --input-dir "${BUTTON_ART_SOURCE}" --output-dir "${BUTTON_ART_GENERATED}"
    DEPENDS "${BUTTON_ART_SCRIPT}"
        "${BUTTON_ART_SOURCE}/normal.png" "${BUTTON_ART_SOURCE}/hover.png"
        "${BUTTON_ART_SOURCE}/pressed.png" "${BUTTON_ART_SOURCE}/disabled.png"
    COMMENT "Convert original button art into embedded native sprites"
    VERBATIM)
set_source_files_properties("${BUTTON_ART_GENERATED}/button_art.rc" PROPERTIES
    OBJECT_DEPENDS "${BUTTON_ART_GENERATED}/deposit-button.sprite;${BUTTON_ART_GENERATED}/deposit-button.lowend.sprite")

# Compile the proven batch implementation directly. There is no runtime dependency
# on Controller QOL, and no controller hooks or input remapping are linked here.
add_library(inventory-deposit SHARED src/plugin_main.cpp src/deposit_button.cpp
    src/deposit_native.cpp src/gold_anchor.cpp src/button_state.cpp src/button_art.cpp src/stash_feedback.cpp src/version.rc
    "${BUTTON_ART_GENERATED}/button_art.rc"
    "${QOL}/src/bulk_stash.cpp" "${QOL}/src/belt_compatibility.cpp")
target_include_directories(inventory-deposit PRIVATE src "${QOL}/src" "${QOL}/include")
target_compile_definitions(inventory-deposit PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
target_compile_options(inventory-deposit PRIVATE /W4)
target_link_libraries(inventory-deposit PRIVATE D2RLPlugin::D2RLPlugin bcrypt)
set_target_properties(inventory-deposit PROPERTIES OUTPUT_NAME "Inventory Deposit" PREFIX "")
d2rlplugin_embed_config(TARGET inventory-deposit FILE "${CMAKE_CURRENT_SOURCE_DIR}/inventory-deposit.toml")

enable_testing()
add_test(NAME inventory-deposit-sprite-codec COMMAND "${Python3_EXECUTABLE}" "${BUTTON_ART_SCRIPT}" --self-test)
add_executable(inventory-deposit-ui-tests tests/deposit_button_tests.cpp src/deposit_button.cpp)
target_include_directories(inventory-deposit-ui-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(inventory-deposit-ui-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_test(NAME inventory-deposit-ui COMMAND inventory-deposit-ui-tests)
add_executable(inventory-deposit-policy-tests "${QOL}/tests/bulk_stash_tests.cpp")
target_include_directories(inventory-deposit-policy-tests PRIVATE "${QOL}/src" "${PLUGIN_SDK}/include")
add_test(NAME inventory-deposit-batch-policy COMMAND inventory-deposit-policy-tests)
add_executable(inventory-deposit-transfer-tests "${QOL}/tests/client_transfer_tests.cpp")
target_include_directories(inventory-deposit-transfer-tests PRIVATE "${QOL}/src" "${PLUGIN_SDK}/include")
target_compile_definitions(inventory-deposit-transfer-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_test(NAME inventory-deposit-transfer-policy COMMAND inventory-deposit-transfer-tests)
add_executable(inventory-deposit-artifact-tests tests/artifact_tests.cpp)
target_include_directories(inventory-deposit-artifact-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(inventory-deposit-artifact-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_dependencies(inventory-deposit-artifact-tests inventory-deposit)
add_test(NAME inventory-deposit-artifact COMMAND inventory-deposit-artifact-tests "$<TARGET_FILE:inventory-deposit>")
add_executable(inventory-deposit-settings-tests tests/settings_tests.cpp)
target_include_directories(inventory-deposit-settings-tests PRIVATE src)
add_test(NAME inventory-deposit-settings COMMAND inventory-deposit-settings-tests)
add_executable(inventory-deposit-gold-anchor-tests tests/gold_anchor_policy_tests.cpp)
target_include_directories(inventory-deposit-gold-anchor-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(inventory-deposit-gold-anchor-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_test(NAME inventory-deposit-gold-anchor COMMAND inventory-deposit-gold-anchor-tests)
add_executable(inventory-deposit-feedback-policy-tests tests/stash_feedback_policy_tests.cpp)
target_include_directories(inventory-deposit-feedback-policy-tests PRIVATE src)
add_test(NAME inventory-deposit-feedback-policy COMMAND inventory-deposit-feedback-policy-tests)
add_executable(inventory-deposit-feedback-tests tests/stash_feedback_tests.cpp src/stash_feedback.cpp)
target_include_directories(inventory-deposit-feedback-tests PRIVATE src "${PLUGIN_SDK}/include")
target_compile_definitions(inventory-deposit-feedback-tests PRIVATE INVENTORY_DEPOSIT_FEEDBACK_TESTS WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(inventory-deposit-feedback-tests PRIVATE /W4)
add_test(NAME inventory-deposit-feedback COMMAND inventory-deposit-feedback-tests)
add_executable(inventory-deposit-wiring-tests tests/plugin_wiring_tests.cpp src/plugin_main.cpp)
target_include_directories(inventory-deposit-wiring-tests PRIVATE src "${QOL}/include" "${PLUGIN_SDK}/include")
target_compile_definitions(inventory-deposit-wiring-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(inventory-deposit-wiring-tests PRIVATE /W4)
add_test(NAME inventory-deposit-wiring COMMAND inventory-deposit-wiring-tests)
add_executable(inventory-deposit-eligibility-tests tests/deposit_eligibility_tests.cpp)
target_include_directories(inventory-deposit-eligibility-tests PRIVATE src "${QOL}/src" "${PLUGIN_SDK}/include")
target_compile_definitions(inventory-deposit-eligibility-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(inventory-deposit-eligibility-tests PRIVATE /W4)
add_test(NAME inventory-deposit-eligibility COMMAND inventory-deposit-eligibility-tests)
add_executable(inventory-deposit-button-state-tests tests/button_state_tests.cpp)
target_include_directories(inventory-deposit-button-state-tests PRIVATE src)
target_compile_definitions(inventory-deposit-button-state-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(inventory-deposit-button-state-tests PRIVATE /W4)
add_test(NAME inventory-deposit-button-state COMMAND inventory-deposit-button-state-tests)
