cmake_minimum_required(VERSION 3.15)

project(owala)

# Set the directory where custom CMake modules are stored
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Include project-wide utility and dependency modules
include(ImportDependencies)
include(Utilities)
importDependencies()

# Global compile flags and macros
add_definitions(-DGLEW_STATIC)
add_definitions(-DUSE_CSD3151_AUTOMATION=0) # Used for instructor's automation
add_definitions(-DGLEW_NO_GLU) # Prevent GLU-related issues

# List of external libraries (common for both builds)
set(COMMON_LIBS
  glfw
  libglew_static
  rapid_json
  stb_image
  Backward::Interface
  freetype
)

# Editor-specific libraries
set(EDITOR_LIBS
  imgui
)

# Include path for GLM
include_directories(${CMAKE_CURRENT_LIST_DIR}/lib/glm)

# ======================= Source Directories =========================
set(ENGINE_DIR ${CMAKE_CURRENT_LIST_DIR}/owala)
set(EDITOR_DIR ${CMAKE_CURRENT_LIST_DIR}/Editor)
set(GAME_DIR   ${CMAKE_CURRENT_LIST_DIR}/StageBound)


# Get all ENGINE source files (from owala/ folder)
file(GLOB_RECURSE ENGINE_SRC
    "${ENGINE_DIR}/*.cpp"
    "${ENGINE_DIR}/*.h"
    "${ENGINE_DIR}/*.hpp"
)

# Get all EDITOR source files (from Editor/ folder)
file(GLOB_RECURSE EDITOR_SRC
    "${EDITOR_DIR}/*.cpp"
    "${EDITOR_DIR}/*.h"
    "${EDITOR_DIR}/*.hpp"
)

# Get all GAME source files (from Game/ folder)
file(GLOB_RECURSE GAME_SRC
    "${GAME_DIR}/*.cpp"
    "${GAME_DIR}/*.h"
    "${GAME_DIR}/*.hpp"
)


# ======================= owala_Engine Executable (Engine + Editor only) =========================
# Universal blank editor. No game code compiled in — the editor edits game data files.
# BUILD_EDITOR is defined so all editor code compiles in via #ifdef BUILD_EDITOR guards.
add_executable(${CMAKE_PROJECT_NAME}_Engine
    ${ENGINE_SRC}
    ${EDITOR_SRC}
)
target_compile_definitions(${CMAKE_PROJECT_NAME}_Engine PRIVATE BUILD_EDITOR)
set_target_properties(${CMAKE_PROJECT_NAME}_Engine PROPERTIES
    FOLDER ""
    OUTPUT_NAME "owala_Engine"
)

# Keep console window visible for the engine/editor build
if(WIN32)
    set_target_properties(${CMAKE_PROJECT_NAME}_Engine PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()


# ======================= StageBound Executable (Engine + Game only) =========================
# This is the shipping/game-only build. No editor code is compiled in.
add_executable(StageBound
    ${ENGINE_SRC}
    ${GAME_SRC}
)
set_target_properties(StageBound PROPERTIES
    FOLDER ""
    OUTPUT_NAME "StageBound"
)

# Hide console window for the game build
if(WIN32)
    set_target_properties(StageBound PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    target_link_options(StageBound PRIVATE
        "/SUBSYSTEM:WINDOWS"
        "/ENTRY:mainCRTStartup"
    )
endif()


# ======================= Precompiled Headers =========================
target_precompile_headers(${CMAKE_PROJECT_NAME}_Engine PRIVATE "${ENGINE_DIR}/pch.h")
target_precompile_headers(StageBound                  PRIVATE "${ENGINE_DIR}/pch.h")


# ======================= File Organization for Visual Studio =========================
foreach(source IN LISTS ENGINE_SRC)
    file(RELATIVE_PATH source_rel "${ENGINE_DIR}" "${source}")
    get_filename_component(source_path "${source_rel}" PATH)
    if(source_path)
        string(REPLACE "/" "\\" group_path "${source_path}")
        source_group("Engine\\${group_path}" FILES "${source}")
    else()
        source_group("Engine" FILES "${source}")
    endif()
endforeach()

foreach(source IN LISTS EDITOR_SRC)
    file(RELATIVE_PATH source_rel "${EDITOR_DIR}" "${source}")
    get_filename_component(source_path "${source_rel}" PATH)
    if(source_path)
        string(REPLACE "/" "\\" group_path "${source_path}")
        source_group("Editor\\${group_path}" FILES "${source}")
    else()
        source_group("Editor" FILES "${source}")
    endif()
endforeach()

foreach(source IN LISTS GAME_SRC)
    file(RELATIVE_PATH source_rel "${GAME_DIR}" "${source}")
    get_filename_component(source_path "${source_rel}" PATH)
    if(source_path)
        string(REPLACE "/" "\\" group_path "${source_path}")
        source_group("Game\\${group_path}" FILES "${source}")
    else()
        source_group("Game" FILES "${source}")
    endif()
endforeach()


# ======================= Working Directory (VS Debugger) =========================
set_property(TARGET ${CMAKE_PROJECT_NAME}_Engine PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
    "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}_Engine>"
)
set_property(TARGET StageBound PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
    "$<TARGET_FILE_DIR:StageBound>"
)


# ======================= Install Rules =========================
macro(COPY_ASSETS_AND_DATA target_name assets_source_dir)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${assets_source_dir}/Data"
                "$<TARGET_FILE_DIR:${target_name}>/Data"
        COMMENT "Copying Data folder to output directory for ${target_name}."
    )
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${assets_source_dir}/Assets"
                "$<TARGET_FILE_DIR:${target_name}>/Assets"
        COMMENT "Copying Assets folder to output directory for ${target_name}."
    )
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${target_name}>/Logs"
        COMMENT "Creating Logs directory for ${target_name}."
    )
endmacro()

# Copy Game/ data to owala_Engine output only when no active game overrides it.
# When an active game is configured, the active game section does a clean + copy instead.
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/active_game.cmake")
    COPY_ASSETS_AND_DATA(${CMAKE_PROJECT_NAME}_Engine "${GAME_DIR}")
else()
    # Peek at active_game.cmake to see if it points to a valid directory.
    include("${CMAKE_CURRENT_LIST_DIR}/active_game.cmake")
    if(NOT ACTIVE_GAME_NAME OR NOT EXISTS "${ACTIVE_GAME_DIR}")
        # Stale/invalid cmake — fall back to Game/ data for the editor.
        COPY_ASSETS_AND_DATA(${CMAKE_PROJECT_NAME}_Engine "${GAME_DIR}")
    endif()
    # If valid, the active game section below handles owala_Engine's data copy.
    unset(ACTIVE_GAME_DIR)
    unset(ACTIVE_GAME_NAME)
endif()
COPY_ASSETS_AND_DATA(StageBound "${GAME_DIR}")


# ======================= Post-Build: Copy FMOD DLLs =========================
if(WIN32)
    macro(COPY_FMOD_DLL target_name)
        # Always copy the release DLL
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    "${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmod.dll"
                    "$<TARGET_FILE_DIR:${target_name}>"
            COMMENT "Copying FMOD DLL to output directory for ${target_name}"
        )
        # Only copy the debug DLL in Debug builds
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND $<$<CONFIG:Debug>:${CMAKE_COMMAND}> $<$<CONFIG:Debug>:-E> $<$<CONFIG:Debug>:copy>
                    $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmodL.dll>
                    $<$<CONFIG:Debug>:$<TARGET_FILE_DIR:${target_name}>>
            COMMENT "Copying FMOD debug DLL (Debug only) for ${target_name}"
        )
    endmacro()

    COPY_FMOD_DLL(${CMAKE_PROJECT_NAME}_Engine)
    COPY_FMOD_DLL(StageBound)
endif()


# ======================= Compiler Settings =========================
set_property(TARGET ${CMAKE_PROJECT_NAME}_Engine PROPERTY CXX_STANDARD 20)
set_property(TARGET StageBound                  PROPERTY CXX_STANDARD 20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${CMAKE_PROJECT_NAME}_Engine PRIVATE -Wall ${DisableWarnings})
    target_compile_options(StageBound                  PRIVATE -Wall ${DisableWarnings})
elseif(MSVC)
    target_compile_options(${CMAKE_PROJECT_NAME}_Engine PRIVATE /W4 /WX-)
    target_compile_options(StageBound                  PRIVATE /W4 /WX-)
endif()


# ======================= Include Paths =========================
# Collect all subdirectories for each source tree
file(GLOB_RECURSE ENGINE_DIRS LIST_DIRECTORIES true "${ENGINE_DIR}/*")
set(INCLUDE_DIRS ${ENGINE_DIR})
foreach(dir ${ENGINE_DIRS})
    if(IS_DIRECTORY ${dir})
        list(APPEND INCLUDE_DIRS ${dir})
    endif()
endforeach()

file(GLOB_RECURSE EDITOR_DIRS LIST_DIRECTORIES true "${EDITOR_DIR}/*")
set(EDITOR_INCLUDE_DIRS ${EDITOR_DIR})
foreach(dir ${EDITOR_DIRS})
    if(IS_DIRECTORY ${dir})
        list(APPEND EDITOR_INCLUDE_DIRS ${dir})
    endif()
endforeach()

file(GLOB_RECURSE GAME_DIRS LIST_DIRECTORIES true "${GAME_DIR}/*")
set(GAME_INCLUDE_DIRS ${GAME_DIR})
foreach(dir ${GAME_DIRS})
    if(IS_DIRECTORY ${dir})
        list(APPEND GAME_INCLUDE_DIRS ${dir})
    endif()
endforeach()

# owala_Engine: engine + editor only (no game source)
target_include_directories(${CMAKE_PROJECT_NAME}_Engine PRIVATE
    ${INCLUDE_DIRS}
    ${EDITOR_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/lib/fmod/core/inc
)

# StageBound: needs engine + game includes only
target_include_directories(StageBound PRIVATE
    ${INCLUDE_DIRS}
    ${GAME_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/lib/fmod/core/inc
)


# ======================= Linking =========================
target_link_libraries(${CMAKE_PROJECT_NAME}_Engine
    ${COMMON_LIBS}
    ${EDITOR_LIBS}
    ${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmod_vc.lib
)

target_link_libraries(StageBound
    ${COMMON_LIBS}
    ${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmod_vc.lib
)


# ======================= Install Rules =========================
if(WIN32)
    install(FILES "${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmod.dll"
        DESTINATION bin
        CONFIGURATIONS Release RelWithDebInfo MinSizeRel
    )
endif()


# ======================= Active Game Project (optional) =========================
# Written by the editor when the user creates or imports a game project.
# Stagebound targets above are never affected.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/active_game.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/active_game.cmake")
    # active_game.cmake must define ACTIVE_GAME_DIR and ACTIVE_GAME_NAME
    if(NOT ACTIVE_GAME_NAME OR NOT EXISTS "${ACTIVE_GAME_DIR}")
        message(WARNING "active_game.cmake references a missing or unnamed project ('${ACTIVE_GAME_NAME}' at '${ACTIVE_GAME_DIR}'). Skipping active game build.")
    else()
    message(STATUS "Active game project: ${ACTIVE_GAME_NAME} at ${ACTIVE_GAME_DIR}")

    file(GLOB_RECURSE ACTIVE_GAME_SRC
        "${ACTIVE_GAME_DIR}/Source/*.cpp"
        "${ACTIVE_GAME_DIR}/Source/*.h"
        "${ACTIVE_GAME_DIR}/Source/*.hpp"
    )

    # Compute include dirs up-front — shared by both the editor and shipping builds.
    file(GLOB_RECURSE ACTIVE_GAME_DIRS LIST_DIRECTORIES true "${ACTIVE_GAME_DIR}/*")
    set(ACTIVE_GAME_INCLUDE_DIRS ${ACTIVE_GAME_DIR})
    foreach(dir ${ACTIVE_GAME_DIRS})
        if(IS_DIRECTORY ${dir})
            list(APPEND ACTIVE_GAME_INCLUDE_DIRS ${dir})
        endif()
    endforeach()

    # --- Inject active game into owala_Engine (editor + game scripts in one build) ---
    # This lets the editor see and register all game scripts after a rebuild.
    target_sources(owala_Engine PRIVATE ${ACTIVE_GAME_SRC})
    target_include_directories(owala_Engine PRIVATE ${ACTIVE_GAME_INCLUDE_DIRS})
    # ACTIVE_GAME_INCLUDED suppresses the NullApplication stub in main.cpp.
    target_compile_definitions(owala_Engine PRIVATE ACTIVE_GAME_INCLUDED)

    # Clean the editor's Data/ and Assets/ output before copying the active game's
    # data so no stale files from a previously active game remain.
    add_custom_command(TARGET owala_Engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:owala_Engine>/Data"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:owala_Engine>/Assets"
        COMMENT "Cleaning stale data from owala_Engine output before copying ${ACTIVE_GAME_NAME} data."
    )
    COPY_ASSETS_AND_DATA(owala_Engine "${ACTIVE_GAME_DIR}")

    # --- Game-only shipping build (skip if same folder as StageBound to avoid a duplicate target) ---
    if("${ACTIVE_GAME_DIR}" STREQUAL "${GAME_DIR}")
        message(STATUS "Active game is StageBound — skipping duplicate shipping target.")
    else()
    add_executable(${ACTIVE_GAME_NAME}
        ${ENGINE_SRC}
        ${ACTIVE_GAME_SRC}
    )
    set_target_properties(${ACTIVE_GAME_NAME} PROPERTIES
        FOLDER ""
        OUTPUT_NAME "${ACTIVE_GAME_NAME}"
    )
    if(WIN32)
        set_target_properties(${ACTIVE_GAME_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
        target_link_options(${ACTIVE_GAME_NAME} PRIVATE
            "/SUBSYSTEM:WINDOWS"
            "/ENTRY:mainCRTStartup"
        )
    endif()

    target_precompile_headers(${ACTIVE_GAME_NAME} PRIVATE "${ENGINE_DIR}/pch.h")

    foreach(source IN LISTS ACTIVE_GAME_SRC)
        file(RELATIVE_PATH source_rel "${ACTIVE_GAME_DIR}" "${source}")
        get_filename_component(source_path "${source_rel}" PATH)
        if(source_path)
            string(REPLACE "/" "\\" group_path "${source_path}")
            source_group("Game\\${group_path}" FILES "${source}")
        else()
            source_group("Game" FILES "${source}")
        endif()
    endforeach()

    set_property(TARGET ${ACTIVE_GAME_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
        "$<TARGET_FILE_DIR:${ACTIVE_GAME_NAME}>"
    )

    COPY_ASSETS_AND_DATA(${ACTIVE_GAME_NAME} "${ACTIVE_GAME_DIR}")

    if(WIN32)
        COPY_FMOD_DLL(${ACTIVE_GAME_NAME})
    endif()

    set_property(TARGET ${ACTIVE_GAME_NAME} PROPERTY CXX_STANDARD 20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${ACTIVE_GAME_NAME} PRIVATE -Wall ${DisableWarnings})
    elseif(MSVC)
        target_compile_options(${ACTIVE_GAME_NAME} PRIVATE /W4 /WX-)
    endif()

    target_include_directories(${ACTIVE_GAME_NAME} PRIVATE
        ${INCLUDE_DIRS}
        ${ACTIVE_GAME_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/lib/fmod/core/inc
    )

    target_link_libraries(${ACTIVE_GAME_NAME}
        ${COMMON_LIBS}
        ${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmod_vc.lib
    )
    endif() # else: active game differs from StageBound — create shipping target
    endif() # else: ACTIVE_GAME_NAME and ACTIVE_GAME_DIR are valid
endif() # EXISTS active_game.cmake