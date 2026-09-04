cmake_minimum_required(VERSION 3.8)
project("Splatter")

set(CMAKE_CXX_STANDARD 20)

# Find Vulkan SDK
find_package(Vulkan REQUIRED)

include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# Include subdirectories
add_subdirectory(third_party)
add_subdirectory(src)

# =========================================================
# OUTPUT DIR (FOR EXE + SHADERS)
# =========================================================
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}")

# Force executables to go directly into /bin (no /Debug or /Release subfolders)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${OUTPUT_DIR})

# Fix for multi-configuration generators (like Visual Studio) to prevent subfolders
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} INTDIR)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${INTDIR} ${OUTPUT_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${INTDIR} ${OUTPUT_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${INTDIR} ${OUTPUT_DIR})
endforeach()

# =========================================================
# GLSL COMPILER
# =========================================================
find_program(GLSL_VALIDATOR glslangValidator
    HINTS
    $ENV{VULKAN_SDK}/Bin/
    $ENV{VULKAN_SDK}/Bin32/
)

# =========================================================
# SHADER OUTPUT DIR (NEXT TO EXE)
# =========================================================
set(SHADER_DIR "${OUTPUT_DIR}/shaders")
file(MAKE_DIRECTORY ${SHADER_DIR})

# =========================================================
# INPUT SHADERS
# =========================================================
file(GLOB GLSL_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp"
)

file(GLOB GLSL_INCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.glsl"
)

# =========================================================
# BUILD SHADERS
# =========================================================
set(SPIRV_OUTPUTS)

foreach(GLSL ${GLSL_FILES})

    get_filename_component(FILE_NAME ${GLSL} NAME)

    set(SPIRV_FILE "${SHADER_DIR}/${FILE_NAME}.spv")

    add_custom_command(
       OUTPUT ${SPIRV_FILE}
       COMMAND ${GLSL_VALIDATOR}
       -V --target-env vulkan1.3
       -g
       ${GLSL}
       -o ${SPIRV_FILE}
       DEPENDS ${GLSL}
       COMMENT "Compiling shader ${FILE_NAME}"
    )

    list(APPEND SPIRV_OUTPUTS ${SPIRV_FILE})

endforeach()

add_custom_target(Shaders ALL
    DEPENDS ${SPIRV_OUTPUTS}
)

add_custom_command(
    TARGET Shaders POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "${OUTPUT_DIR}/assets"
)

