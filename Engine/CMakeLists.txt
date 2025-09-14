# ======================= Project Configuration =========================
cmake_minimum_required(VERSION 2.8...3.5)

# Set the CMake policy version
cmake_policy(VERSION 3.5)

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
  PRIVATE stb_image  # Added stb_image to the libraries
)

# Include path for GLM (optional if not using FetchContent)
include_directories(${CMAKE_CURRENT_LIST_DIR}/lib/glm)

set(SRC_DIR ./owala)
file(GLOB_RECURSE APP_SRC 
   "${SRC_DIR}/*.cpp"
   "${SRC_DIR}/*.h"
   "${SRC_DIR}/*.hpp"
)
#GROUP_FILES_BY_FOLDER(${CMAKE_CURRENT_LIST_DIR}s)
GROUP_FILES_BY_FOLDER(${APP_SRC})


add_executable(${CMAKE_PROJECT_NAME}
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        ${APP_SRC})

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
            "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/Data"
)
add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Assets"
            "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/Assets"
)


# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries.
# Link libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    ${ALL_LIBS}
)

# Set C++ standard
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 20)

# Warnings based on compiler
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall ${DisableWarnings})
elseif (MSVC)
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /W3 /WX-)
endif()

# Include path for each project
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${APP_SRC}
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${SRC_DIR}/ECS
)