# Define a macro to group source and header files by directory


# Groups files by their relative subfolder in the source tree
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




# Define function to copy assets with conditional copying
function(COPY_FOLDER_IF_DIFFERENT PROJECT_NAME source_dir dest_dir)
    file(GLOB_RECURSE ASSET_FILES "${source_dir}/*")

    foreach(asset ${ASSET_FILES})
        file(RELATIVE_PATH relative_path "${source_dir}" "${asset}")
        get_filename_component(dest_subdir "${relative_path}" PATH)
        set(dest_file "${dest_dir}/${relative_path}")

        add_custom_command(
            TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${dest_dir}/${dest_subdir}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${asset}" "${dest_file}"
        )
    endforeach()
endfunction()


function(BUILD_SHADERS TARGET_NAME)
    find_program(GLSL_VALIDATOR glslangValidator HINTS 
    ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} 
      /usr/bin 
      /usr/local/bin 
      ${VULKAN_SDK_PATH}/Bin
      ${VULKAN_SDK_PATH}/Bin32
      $ENV{VULKAN_SDK}/Bin/ 
      $ENV{VULKAN_SDK}/Bin32/
    )

    file(GLOB_RECURSE SHADER_SOURCES
    "${PROJECT_SOURCE_DIR}/Engine/Assets/Shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/Engine/Assets/Shaders/*.vert"
    )

    foreach(GLSL ${SHADER_SOURCES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_SOURCE_DIR}/Engine/Assets/Shaders/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
    endforeach(GLSL)
    add_custom_target(
        ${TARGET_NAME}
        SOURCES ${SHADER_SOURCES}
        DEPENDS ${SPIRV_BINARY_FILES}
    )

    # Define your shaders directory
    set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Engine/Assets/Shaders")

    # Define where your executables are built
    set(SHADER_DEST_DIR "${CMAKE_BINARY_DIR}/Assets/Shaders")
    
    # Call the function to copy shaders
    COPY_FOLDER_IF_DIFFERENT(${TARGET_NAME} ${SHADER_SOURCE_DIR} ${SHADER_DEST_DIR})
endfunction()