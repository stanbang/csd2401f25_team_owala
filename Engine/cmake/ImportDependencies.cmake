include(FetchContent)

# Macro to import stb_image
macro(import_stb_image)
    if(NOT TARGET stb_image)  # Guard to prevent multiple inclusion
        FetchContent_Declare(
            stb_image
            GIT_REPOSITORY https://github.com/nothings/stb.git
            GIT_TAG master
        )
        if(NOT stb_image_POPULATED)
            FetchContent_Populate(stb_image)
        endif()

        # Create a simple interface library for stb_image
        add_library(stb_image INTERFACE)
        target_include_directories(stb_image INTERFACE ${stb_image_SOURCE_DIR})
        
        # Optionally, you can also define STB_IMAGE_IMPLEMENTATION if you want to compile it
        # Alternatively, you can just include the header and define STB_IMAGE_IMPLEMENTATION in your source
    endif()
endmacro()

# Macro to import GLFW
macro(import_glfw)
    if(NOT TARGET glfw)  # Guard to prevent multiple inclusion
        FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.3.8
        )
        if(NOT glfw_POPULATED)
            set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
            FetchContent_Populate(glfw)
        endif()

        add_subdirectory(${glfw_SOURCE_DIR})
        include_directories(${GLFW_SOURCE_DIR}/include)
    endif()
endmacro()

# Macro to import glm
macro(import_glm)
    if(NOT TARGET glm)  # Guard to prevent multiple inclusion
        FetchContent_Declare(
            glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG master
        )
        FetchContent_MakeAvailable(glm)

        include_directories(${glm_SOURCE_DIR})
    endif()
endmacro()

# Macro to import glew
macro(import_glew)
    if(NOT TARGET glew_s)  # Guard to prevent multiple inclusion
        include(FetchContent)

        FetchContent_Declare(
            glew
            GIT_REPOSITORY https://github.com/omniavinco/glew-cmake.git
            GIT_TAG master  # Or use a specific commit or tag if preferred
        )

        # Disable shared builds if needed
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libs" FORCE)
        FetchContent_MakeAvailable(glew)

        # Optional: include directories if needed manually (though target_link_libraries handles it)
        include_directories(${glew_SOURCE_DIR}/include)
    endif()
endmacro()

# Macro to import ImGui
macro(import_imgui)
    if(NOT TARGET imgui)  # Guard to prevent multiple inclusion
        FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.90.4
        )
        if(NOT imgui_POPULATED)
            FetchContent_Populate(imgui)
        endif()

        # Create ImGui library manually since it doesn't have CMakeLists.txt
        set(IMGUI_SOURCES
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        )

        add_library(imgui STATIC ${IMGUI_SOURCES})
        target_include_directories(imgui PUBLIC 
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        target_link_libraries(imgui PUBLIC glfw libglew_static)
        
        # Set C++ standard for ImGui (required for constexpr support)
        set_property(TARGET imgui PROPERTY CXX_STANDARD 11)
        set_property(TARGET imgui PROPERTY CXX_STANDARD_REQUIRED ON)
    endif()
endmacro()

# Macro to import rapid_json
macro(import_rapid_json)
    if(NOT TARGET rapid_json)  # Guard to prevent multiple inclusion
        include(FetchContent)

        FetchContent_Declare(
            rapid_json
            GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
            GIT_TAG master  # Or use a specific commit or tag if preferred
        )

        # Disable building tests and examples
        set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE INTERNAL "")
        set(RAPIDJSON_BUILD_TESTS OFF CACHE INTERNAL "")
        set(RAPIDJSON_BUILD_DOC OFF CACHE INTERNAL "")
        set(RAPIDJSON_BUILD_THIRDPARTY_GTEST OFF CACHE INTERNAL "")

        # Disable shared builds if needed
        FetchContent_MakeAvailable(rapid_json)

        add_library(rapid_json INTERFACE)
        target_include_directories(rapid_json INTERFACE ${rapid_json_SOURCE_DIR}/include)

    endif()
endmacro()

# Macro to import all dependencies
macro(importDependencies)
    message(STATUS "Starting to import dependencies...")

    message(STATUS "Importing stb_image...")
    import_stb_image()
    message(STATUS "stb_image imported successfully.")

    message(STATUS "Importing GLFW...")
    import_glfw()
    message(STATUS "GLFW imported successfully.")

    message(STATUS "Importing GLM...")
    import_glm()
    message(STATUS "GLM imported successfully.")

    message(STATUS "Importing GLEW...")
    import_glew()
    message(STATUS "GLEW imported successfully.")

    message(STATUS "Importing ImGui...")
    import_imgui()
    message(STATUS "ImGui imported successfully.")

    message(STATUS "Importing rapid_json...")
    import_rapid_json()
    message(STATUS "rapid_json imported successfully.")

<<<<<<< Updated upstream
    link_libraries(${CMAKE_SOURCE_DIR}/lib/fmod/api/core/lib/x64/fmod_vc.lib)
=======
    link_libraries(${CMAKE_SOURCE_DIR}/lib/fmod/core/lib/x64/fmod_vc.lib) 
    include_directories(${CMAKE_SOURCE_DIR}/lib/fmod/core/inc) 
>>>>>>> Stashed changes

    message(STATUS "All dependencies have been imported successfully.")
endmacro()