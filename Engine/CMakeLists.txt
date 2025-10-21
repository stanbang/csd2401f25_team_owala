# ======================= Project Configuration =========================
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

# List of external libraries
set(ALL_LIBS
  PRIVATE glfw
  PRIVATE libglew_static
  PRIVATE imgui
  PRIVATE rapid_json
  PRIVATE stb_image
  PRIVATE Backward::Interface
  PRIVATE freetype
)

# Include path for GLM
include_directories(${CMAKE_CURRENT_LIST_DIR}/lib/glm)

# ======================= Source Files =========================
set(SRC_DIR ./owala)

# Get ALL source files EXCEPT main.cpp and EditorMain.cpp
file(GLOB_RECURSE ENGINE_SRC
   "${SRC_DIR}/*.cpp"
   "${SRC_DIR}/*.h"
   "${SRC_DIR}/*.hpp"
)

# Remove main entry points from engine sources
list(FILTER ENGINE_SRC EXCLUDE REGEX ".*main\\.cpp$")
list(FILTER ENGINE_SRC EXCLUDE REGEX ".*EditorMain\\.cpp$")

# Group by folder tree instead of "Header/Source Files"
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${ENGINE_SRC})

# -------------------- Group files by folder --------------------
# New macro to organize filters by directory
macro(GROUP_FILES_BY_FOLDER all_files)
    foreach(_file IN LISTS all_files)
        get_filename_component(_abs_file "${_file}" ABSOLUTE)
        get_filename_component(_dir "${_abs_file}" DIRECTORY)
        file(RELATIVE_PATH _rel_dir "${CMAKE_CURRENT_SOURCE_DIR}" "${_dir}")

        if (_rel_dir STREQUAL "")
            set(_rel_dir "\\") # root folder in VS filter
        endif()

        # Replace / with \ for Visual Studio filters
        string(REPLACE "/" "\\" _group_path "${_rel_dir}")

        source_group("${_group_path}" FILES "${_abs_file}")
    endforeach()
endmacro()

# Apply grouping
GROUP_FILES_BY_FOLDER("${ENGINE_SRC}")

# ======================= Executables =========================

# Main Game Executable (uses main.cpp)
add_executable(${CMAKE_PROJECT_NAME} 
    ${ENGINE_SRC}
    ${SRC_DIR}/main.cpp
)

# Editor Executable (uses EditorMain.cpp)
add_executable(${CMAKE_PROJECT_NAME}_Editor
    ${ENGINE_SRC}
    ${SRC_DIR}/Editor/EditorMain.cpp
)

# ======================= Set working directory to Debug/Release folders =========================
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
    "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>"
)
set_property(TARGET ${CMAKE_PROJECT_NAME}_Editor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
    "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}_Editor>"
)

# ======================================================
# Copy Data and Assets folders at configure time
# ======================================================
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/Logs)

# ======================================================
# Also copy Data and Assets after each build (keeps sync)
# ======================================================

# Function to setup post-build commands for both executables
function(setup_post_build_commands TARGET_NAME)
    add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                  "${CMAKE_SOURCE_DIR}/Data"
                  "$<TARGET_FILE_DIR:${TARGET_NAME}>/Data"
        COMMENT "Copying Data folder to ${TARGET_NAME} output directory."
    )
    add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                  "${CMAKE_SOURCE_DIR}/Assets"
                  "$<TARGET_FILE_DIR:${TARGET_NAME}>/Assets"
        COMMENT "Copying Assets folder to ${TARGET_NAME} output directory."
    )
    add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                  "$<TARGET_FILE_DIR:${TARGET_NAME}>/Logs"
        COMMENT "Creating Logs directory for ${TARGET_NAME}."
    )
    
    # Copy FMOD DLL
    if(WIN32)
        add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    "${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmod.dll"
                    "$<TARGET_FILE_DIR:${TARGET_NAME}>"
            COMMENT "Copying FMOD DLL to ${TARGET_NAME} output directory"
        )
        
        if(EXISTS "${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmodL.dll")
            add_custom_command(
                TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                        "${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmodL.dll"
                        "$<TARGET_FILE_DIR:${TARGET_NAME}>"
            )
        endif()
    endif()
endfunction()

# Setup post-build for both executables
setup_post_build_commands(${CMAKE_PROJECT_NAME})
setup_post_build_commands(${CMAKE_PROJECT_NAME}_Editor)

# ======================= Linking =========================
# ======================================================
# Link libraries and set properties
# ======================================================
target_link_libraries(${CMAKE_PROJECT_NAME}
    ${ALL_LIBS}
    ${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmod_vc.lib
)

target_link_libraries(${CMAKE_PROJECT_NAME}_Editor
    ${ALL_LIBS}
    ${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmod_vc.lib
)

# ======================= Compiler Settings =========================
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${CMAKE_PROJECT_NAME}_Editor PROPERTY CXX_STANDARD 20)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall ${DisableWarnings})
    target_compile_options(${CMAKE_PROJECT_NAME}_Editor PRIVATE -Wall ${DisableWarnings})
elseif (MSVC)
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /W4 /WX-)
    target_compile_options(${CMAKE_PROJECT_NAME}_Editor PRIVATE /W4 /WX-)
endif()

# ======================= Include Paths =========================
# Add owala and all of its subfolders so we can do #include "xxx.h"
file(GLOB_RECURSE ALL_DIRS LIST_DIRECTORIES true "${SRC_DIR}/*")

set(INCLUDE_DIRS ${SRC_DIR}) # start with the root
foreach(dir ${ALL_DIRS})
    if (IS_DIRECTORY ${dir})
        list(APPEND INCLUDE_DIRS ${dir})
    endif()
endforeach()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})
target_include_directories(${CMAKE_PROJECT_NAME}_Editor PRIVATE ${INCLUDE_DIRS})

# Include path for each project
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/fmod/core/inc  # Add FMOD include directory
)

target_include_directories(${CMAKE_PROJECT_NAME}_Editor PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/fmod/core/inc  # Add FMOD include directory
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${SRC_DIR}/ECS
)

target_include_directories(${CMAKE_PROJECT_NAME}_Editor PRIVATE
    ${SRC_DIR}/ECS
)

# ======================================================
# Install rules for distribution (optional)
# ======================================================
if(WIN32)
    install(FILES "${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmod.dll"
        DESTINATION bin
        CONFIGURATIONS Release RelWithDebInfo MinSizeRel
    )
endif()