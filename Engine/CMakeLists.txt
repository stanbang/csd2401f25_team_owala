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

# ======================= Source Files =========================
set(ENGINE_DIR ${CMAKE_CURRENT_LIST_DIR}/owala)
set(EDITOR_DIR ${CMAKE_CURRENT_LIST_DIR}/../Editor)


# Get all ENGINE source files (from Engine/owala/ folder)
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


# Add this section right after your executable creation sections
# ======================= Engine Executable =========================
add_executable(${CMAKE_PROJECT_NAME}_Engine ${ENGINE_SRC})
target_compile_definitions(${CMAKE_PROJECT_NAME}_Engine PRIVATE -DBUILD_ENGINE)
set_target_properties(${CMAKE_PROJECT_NAME}_Engine PROPERTIES 
    FOLDER ""
    OUTPUT_NAME "StageBound"
)

# Hide console window for Engine builds (Windows only)
#if(WIN32)
    set_target_properties(${CMAKE_PROJECT_NAME}_Engine PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    target_link_options(${CMAKE_PROJECT_NAME}_Engine PRIVATE 
        "/SUBSYSTEM:WINDOWS"
        "/ENTRY:mainCRTStartup"
    )
#endif()

# ======================= Editor Executable =========================
add_executable(${CMAKE_PROJECT_NAME}_Editor ${ENGINE_SRC} ${EDITOR_SRC})
target_compile_definitions(${CMAKE_PROJECT_NAME}_Editor PRIVATE -DBUILD_EDITOR)
set_target_properties(${CMAKE_PROJECT_NAME}_Editor PROPERTIES 
    FOLDER ""
    OUTPUT_NAME "StageBound_Editor"
)

# Keep console window for Editor builds (explicitly set to FALSE for clarity)
if(WIN32)
    set_target_properties(${CMAKE_PROJECT_NAME}_Editor PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()

#Precompiled Headers
target_precompile_headers(${CMAKE_PROJECT_NAME}_Engine PRIVATE "${ENGINE_DIR}/pch.h")
target_precompile_headers(${CMAKE_PROJECT_NAME}_Editor PRIVATE "${ENGINE_DIR}/pch.h")

# ======================= File Organization for Visual Studio =========================
foreach(source IN LISTS ENGINE_SRC)
    # Get relative path from ENGINE_DIR
    file(RELATIVE_PATH source_rel "${ENGINE_DIR}" "${source}")
    
    # Get just the directory part
    get_filename_component(source_path "${source_rel}" PATH)
    
    if(source_path)
        string(REPLACE "/" "\\" group_path "${source_path}")
        source_group("Engine\\${group_path}" FILES "${source}")
    else()
        # File is in root owala directory
        source_group("Engine" FILES "${source}")
    endif()
endforeach()

foreach(source IN LISTS EDITOR_SRC)
    # Get relative path from EDITOR_DIR
    file(RELATIVE_PATH source_rel "${EDITOR_DIR}" "${source}")
    
    # Get just the directory part
    get_filename_component(source_path "${source_rel}" PATH)
    
    if(source_path)
        # File is in a subdirectory
        string(REPLACE "/" "\\" group_path "${source_path}")
        source_group("Editor\\${group_path}" FILES "${source}")
    else()
        # File is in root Editor directory
        source_group("Editor" FILES "${source}")
    endif()
endforeach()

# ======================= Set working directory to Debug/Release folders =========================
set_property(TARGET ${CMAKE_PROJECT_NAME}_Engine PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
    "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}_Engine>"
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
macro(COPY_ASSETS_AND_DATA target_name)
    add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                  "${CMAKE_SOURCE_DIR}/Data"
                  "$<TARGET_FILE_DIR:${target_name}>/Data"
        COMMENT "Copying Data folder to output directory for ${target_name}."
    )
    add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                  "${CMAKE_SOURCE_DIR}/Assets"
                  "$<TARGET_FILE_DIR:${target_name}>/Assets"
        COMMENT "Copying Assets folder to output directory for ${target_name}."
    )
    
    # Also create the Logs folder in the final output directory
    add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                  "$<TARGET_FILE_DIR:${target_name}>/Logs"
        COMMENT "Creating Logs directory for ${target_name}."
    )
endmacro()

COPY_ASSETS_AND_DATA(${CMAKE_PROJECT_NAME}_Engine)
COPY_ASSETS_AND_DATA(${CMAKE_PROJECT_NAME}_Editor)

# ======================= Linking =========================
# ======================================================
# Copy FMOD DLL to output directory (CRITICAL FOR RUNTIME)
# ======================================================
if(WIN32)
    macro(COPY_FMOD_DLL target_name)
        add_custom_command(
            TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    "${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmod.dll"
                    "$<TARGET_FILE_DIR:${target_name}>"
            COMMENT "Copying FMOD DLL to output directory for ${target_name}"
        )
        
        # Optional: Copy debug version if needed
        if(EXISTS "${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmodL.dll")
            add_custom_command(
                TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                        "${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmodL.dll"
                        "$<TARGET_FILE_DIR:${target_name}>"
            )
        endif()
    endmacro()
    
    COPY_FMOD_DLL(${CMAKE_PROJECT_NAME}_Engine)
    COPY_FMOD_DLL(${CMAKE_PROJECT_NAME}_Editor)
endif()

# ======================= Compiler Settings =========================
set_property(TARGET ${CMAKE_PROJECT_NAME}_Engine PROPERTY CXX_STANDARD 20)
set_property(TARGET ${CMAKE_PROJECT_NAME}_Editor PROPERTY CXX_STANDARD 20)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${CMAKE_PROJECT_NAME}_Engine PRIVATE -Wall ${DisableWarnings})
    target_compile_options(${CMAKE_PROJECT_NAME}_Editor PRIVATE -Wall ${DisableWarnings})
elseif (MSVC)
    target_compile_options(${CMAKE_PROJECT_NAME}_Engine PRIVATE /W4 /WX-)
    target_compile_options(${CMAKE_PROJECT_NAME}_Editor PRIVATE /W4 /WX-)
endif()

# ======================= Include Paths =========================
# Add owala and all of its subfolders
file(GLOB_RECURSE ENGINE_DIRS LIST_DIRECTORIES true "${ENGINE_DIR}/*")

set(INCLUDE_DIRS ${ENGINE_DIR}) # start with the root owala folder
foreach(dir ${ENGINE_DIRS})
    if (IS_DIRECTORY ${dir})
        list(APPEND INCLUDE_DIRS ${dir})
    endif()
endforeach()

# Add Editor folder and its subdirectories
file(GLOB_RECURSE EDITOR_DIRS LIST_DIRECTORIES true "${EDITOR_DIR}/*")
set(EDITOR_INCLUDE_DIRS ${EDITOR_DIR})
foreach(dir ${EDITOR_DIRS})
    if (IS_DIRECTORY ${dir})
        list(APPEND EDITOR_INCLUDE_DIRS ${dir})
    endif()
endforeach()

# Engine includes (only owala folder)
target_include_directories(${CMAKE_PROJECT_NAME}_Engine PRIVATE 
    ${INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/lib/fmod/core/inc
)

# Editor includes (both owala and Editor folders)
target_include_directories(${CMAKE_PROJECT_NAME}_Editor PRIVATE 
    ${INCLUDE_DIRS}
    ${EDITOR_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/lib/fmod/core/inc
)

# ======================= Final Linking =========================
target_link_libraries(${CMAKE_PROJECT_NAME}_Engine
    ${COMMON_LIBS}
    ${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmod_vc.lib
)

target_link_libraries(${CMAKE_PROJECT_NAME}_Editor
    ${COMMON_LIBS}
    ${EDITOR_LIBS}
    ${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmod_vc.lib
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