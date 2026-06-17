# Dual-mode CMake:
#   - Root build (default, how all other projects build): the third-party libs
#     already exist as targets, added by the repo-root CMakeLists.txt.
#   - Standalone (cmake -B build -S projects/finalproject): we add the libs here.
#
# CRITICAL: detect the mode by whether the glfw target already exists. Do NOT
# check a THIRD_PARTY_PATH variable -- the root CMakeLists.txt never defines one,
# so any "defined" check is always-true and we'd add_subdirectory the libs twice
# (CMake error: target already exists).
if(NOT TARGET glfw)
    # ---- STANDALONE MODE ----
    cmake_minimum_required(VERSION 3.15)
    project(FinalProject LANGUAGES C CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    set(FINAL_ROOT ${CMAKE_SOURCE_DIR})              # projects/finalproject
    set(TP ${FINAL_ROOT}/external)

    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    add_subdirectory(${TP}/glfw  ${CMAKE_BINARY_DIR}/_lib/glfw)
    add_subdirectory(${TP}/glad  ${CMAKE_BINARY_DIR}/_lib/glad)
    add_subdirectory(${TP}/glm   ${CMAKE_BINARY_DIR}/_lib/glm)
    add_subdirectory(${TP}/imgui ${CMAKE_BINARY_DIR}/_lib/imgui)
    add_subdirectory(${TP}/stb   ${CMAKE_BINARY_DIR}/_lib/stb)
    set(FINAL_NAME FinalProject)
else()
    # ---- ROOT-INCLUDED MODE (libs already added by root CMakeLists.txt) ----
    set(FINAL_ROOT ${CMAKE_CURRENT_SOURCE_DIR})      # projects/finalproject
    set(FINAL_NAME finalproject)
    # Do NOT add_subdirectory the libs here (root already did).
    # Do NOT link tinyobjloader (TA requires a self-written OBJ loader).
endif()

# ---- Sources (base + game + source root; model.{h,cpp} added in Task 1) ----
file(GLOB BASE_HDR ${FINAL_ROOT}/source/base/*.h)
file(GLOB BASE_SRC ${FINAL_ROOT}/source/base/*.cpp)
# Exclude throwaway self-test sources from the main target (they have their own
# main() and live in their own executable below).
list(FILTER BASE_SRC EXCLUDE REGEX ".*selftest.*")
list(FILTER BASE_HDR EXCLUDE REGEX ".*selftest.*")
file(GLOB GAME_HDR ${FINAL_ROOT}/source/game/*.h)
file(GLOB GAME_SRC ${FINAL_ROOT}/source/game/*.cpp)
# Top-level source modules (primitives, nurbs, collision, ...).
file(GLOB ROOT_HDR ${FINAL_ROOT}/source/*.h)
file(GLOB ROOT_SRC ${FINAL_ROOT}/source/*.cpp)
set(MAIN_SRC ${FINAL_ROOT}/source/main.cpp)
list(FILTER ROOT_SRC EXCLUDE REGEX ".*main\\.cpp$")
list(FILTER ROOT_SRC EXCLUDE REGEX ".*selftest.*")
list(FILTER ROOT_HDR EXCLUDE REGEX ".*selftest.*")

add_executable(${FINAL_NAME} ${BASE_HDR} ${BASE_SRC} ${GAME_HDR} ${GAME_SRC} ${ROOT_HDR} ${ROOT_SRC} ${MAIN_SRC})

target_include_directories(${FINAL_NAME} PRIVATE
    ${FINAL_ROOT}/source
    ${FINAL_ROOT}/source/base
    ${FINAL_ROOT}/source/game
    ${FINAL_ROOT}/external/miniaudio)
    # imgui impl headers (imgui_impl_glfw.h, imgui_impl_opengl3.h) live flat in
    # external/imgui/ -- NOT in a backends/ subdir -- and are already compiled
    # into the imgui library target. Just link imgui; no extra include path.

# Copy finalproject's own media next to the exe, so assets resolve via a RELATIVE
# path ("media/"). This is robust across machines / when packaged: the source
# tree's absolute path is never baked into the binary. (Re-copied on every
# configure so edits to media/ are picked up.)
add_custom_command(TARGET ${FINAL_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${FINAL_ROOT}/media
            $<TARGET_FILE_DIR:${FINAL_NAME}>/media)

target_link_libraries(${FINAL_NAME} PRIVATE glfw glad glm imgui stb)

# miniaudio: single header compiled in exactly one TU (audio.cpp, Task 8).
# On non-MSVC Windows toolchains, link the system libs miniaudio relies on.
if(WIN32 AND NOT MSVC)
    target_link_libraries(${FINAL_NAME} PRIVATE ole32 winmm ksuser)
endif()
