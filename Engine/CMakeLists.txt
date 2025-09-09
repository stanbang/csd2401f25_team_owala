# Minimum version of CMake required
cmake_minimum_required(VERSION 3.5)


# Project Setup
project(owala)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Add Definitions
add_definitions(-DGLEW_STATIC)

# external
add_subdirectory(lib/glfw EXCLUDE_FROM_ALL)
add_subdirectory(lib/glew EXCLUDE_FROM_ALL) 
add_subdirectory(lib/glm EXCLUDE_FROM_ALL) 

# Gather src and hdr file
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE ENGINE_HEADERS CONFIGURE_DEPENDS src/*.h)


# Gather headers from include folder
file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS include/*.h include/*.hpp)

add_executable(${PROJECT_NAME} ${ENGINE_SOURCES} ${ENGINE_HEADERS} ${PROJECT_HEADERS})

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



# Organize files in Visual Studio Solution Explorer

# Engine sources -> mirror folder tree under "Engine"
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" 
  PREFIX "Engine" 
  FILES ${ENGINE_SOURCES} ${ENGINE_HEADERS}
)


source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/include"
             PREFIX "Headers"
             FILES ${PROJECT_HEADERS})
