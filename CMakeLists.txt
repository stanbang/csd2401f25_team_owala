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
set(GAME_DIR   ${CMAKE_CURRENT_LIST_DIR}/Game)

# Shared assets source (single source of truth for Data/ and Assets/)
set(ASSETS_DIR ${CMAKE_CURRENT_LIST_DIR})

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


# ======================= owala_Engine Executable (Engine + Editor + Game) =========================
# This is the developer-facing build. BUILD_EDITOR is defined so all editor
# code compiles in via #ifdef BUILD_EDITOR guards.
add_executable(${CMAKE_PROJECT_NAME}_Engine
    ${ENGINE_SRC}
    ${EDITOR_SRC}
    ${GAME_SRC}
)
target_compile_definitions(${CMAKE_PROJECT_NAME}_Engine PRIVATE BUILD_EDITOR)
set_target_properties(${CMAKE_PROJECT_NAME}_Engine PROPERTIES
    FOLDER ""
    OUTPUT_NAME "StageBound_Engine"
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


# ======================= Asset Directory ==========================
# Game assets live under Game/Assets and Game/Data
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/Game")

# ======================= Post-Build: Copy Assets & Create Logs =========================
macro(COPY_ASSETS_AND_DATA target_name)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${ASSETS_DIR}/Data"
                "$<TARGET_FILE_DIR:${target_name}>/Data"
        COMMENT "Copying Data folder to output directory for ${target_name}."
    )
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${ASSETS_DIR}/Assets"
                "$<TARGET_FILE_DIR:${target_name}>/Assets"
        COMMENT "Copying Assets folder to output directory for ${target_name}."
    )
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${target_name}>/Logs"
        COMMENT "Creating Logs directory for ${target_name}."
    )
endmacro()

COPY_ASSETS_AND_DATA(${CMAKE_PROJECT_NAME}_Engine)
COPY_ASSETS_AND_DATA(StageBound)


# ======================= Post-Build: Copy FMOD DLLs =========================
if(WIN32)
    macro(COPY_FMOD_DLL target_name)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    "${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmod.dll"
                    "$<TARGET_FILE_DIR:${target_name}>"
            COMMENT "Copying FMOD DLL to output directory for ${target_name}"
        )
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    "${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmodL.dll"
                    "$<TARGET_FILE_DIR:${target_name}>"
            COMMENT "Copying FMOD logging DLL to output directory for ${target_name}"
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

# owala_Engine: needs engine + editor + game includes
target_include_directories(${CMAKE_PROJECT_NAME}_Engine PRIVATE
    ${INCLUDE_DIRS}
    ${EDITOR_INCLUDE_DIRS}
    ${GAME_INCLUDE_DIRS}
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