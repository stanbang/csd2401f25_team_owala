# Minimum version of CMake required
cmake_minimum_required(VERSION 3.5)


# Project Setup
project(owala)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



# Add executables
add_executable(${PROJECT_NAME}
  src/main.cpp
)


# Add Definitions
add_definitions(-DGLEW_STATIC)

# external
add_subdirectory(lib/glfw EXCLUDE_FROM_ALL)
add_subdirectory(lib/glew EXCLUDE_FROM_ALL) 
add_subdirectory(lib/glm EXCLUDE_FROM_ALL) 



# Link Libraries
target_link_libraries(${PROJECT_NAME}
  PRIVATE
    glfw
    glm
    libglew_static
)

# Includes
target_include_directories(${PROJECT_NAME}
  PRIVATE
    include
    lib/glfw/include
    lib/glm
)