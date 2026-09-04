
# Add source to this project's executable.
add_executable(Splatter
    main.cpp
    vk_engine.cpp
    vk_engine.h
    vk_types.h
    vk_initializers.cpp
    vk_initializers.h
    vk_images.h
    vk_images.cpp
    vk_descriptors.h
    vk_descriptors.cpp
    vk_pipelines.h
    vk_pipelines.cpp
    vk_loader.h
    vk_loader.cpp
    vk_buffers.h
    vk_buffers.cpp
    vk_swapchain.h
    vk_swapchain.cpp
    splat.hpp
    ply_loader.hpp
)


set_property(TARGET Splatter PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Splatter>")

target_include_directories(Splatter PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(Splatter vkbootstrap vma glm stb_image tinyobjloader imgui fmt::fmt fastgltf nlohmann_json::nlohmann_json)
target_link_libraries(Splatter Vulkan::Vulkan SDL2::SDL2)
target_compile_definitions(Splatter PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE)

add_dependencies(Splatter Shaders)

add_custom_command(TARGET Splatter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:Splatter>/assets"
)