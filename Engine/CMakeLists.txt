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
  PRIVATE freetype
)

# Include path for GLM
include_directories(${CMAKE_CURRENT_LIST_DIR}/lib/glm)

# ======================= Source Files =========================
set(SRC_DIR ./owala)
file(GLOB_RECURSE APP_SRC
   "${SRC_DIR}/*.cpp"
   "${SRC_DIR}/*.h"
   "${SRC_DIR}/*.hpp"
)

# Group by folder tree instead of "Header/Source Files"
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${APP_SRC})

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
GROUP_FILES_BY_FOLDER("${APP_SRC}")

# ======================= Executable =========================
add_executable(${CMAKE_PROJECT_NAME} ${APP_SRC})

# ======================================================
# Copy Data and Assets folders at configure time
# ======================================================
file(COPY "${CMAKE_SOURCE_DIR}/Data" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "${CMAKE_SOURCE_DIR}/Assets" DESTINATION "${CMAKE_BINARY_DIR}")

# ======================================================
# Also copy Data and Assets after each build (keeps sync)
# ======================================================
add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Data"
            "$/Data"
)
add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Assets"
            "$/Assets"
)

# ======================= Linking =========================
# ======================================================
# Copy FMOD DLL to output directory (CRITICAL FOR RUNTIME)
# ======================================================
if(WIN32)
    add_custom_command(
        TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmod.dll"
                "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>"
        COMMENT "Copying FMOD DLL to output directory"
    )
    
    # Optional: Copy debug version if needed
    if(EXISTS "${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmodL.dll")
        add_custom_command(
            TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    "${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmodL.dll"
                    "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>"
        )
    endif()
endif()

# ======================================================
# Link libraries and set properties
# ======================================================
target_link_libraries(${CMAKE_PROJECT_NAME}
    ${ALL_LIBS}
    ${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmod_vc.lib
)

# ======================= Compiler Settings =========================
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 20)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall ${DisableWarnings})
elseif (MSVC)
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /W3 /WX-)
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

# Include path for each project
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${APP_SRC}
    ${CMAKE_SOURCE_DIR}/lib/fmod/core/inc  # Add FMOD include directory
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
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
