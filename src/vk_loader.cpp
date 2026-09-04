#include "vk_loader.h"

#include "stb_image.h"
#include <iostream>

#include "vk_initializers.h"
#include "vk_types.h"
#include <glm/gtx/quaternion.hpp>

#include "vk_mem_alloc.h"
#include "vk_engine.h"

GPUMeshBuffers upload_mesh(const Context& context, std::span<uint32_t> indices, std::span<Vertex> vertices)
{
    const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    GPUMeshBuffers newSurface;

    //create vertex buffer
    newSurface.vertexBuffer = vkutil::create_buffer(context, vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    //find the adress of the vertex buffer
    VkBufferDeviceAddressInfo deviceAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = newSurface.vertexBuffer.buffer };
    newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(context.device, &deviceAdressInfo);

    //create index buffer
    newSurface.indexBuffer = vkutil::create_buffer(context, indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    AllocatedBuffer staging = vkutil::create_buffer(context, vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);


    VmaAllocationInfo allocInfo;
    vmaGetAllocationInfo(context.allocator, staging.allocation, &allocInfo);

    void* data = allocInfo.pMappedData;

    // copy vertex buffer
    memcpy(data, vertices.data(), vertexBufferSize);
    // copy index buffer
    memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

    context.immediate_submit([&](VkCommandBuffer cmd) {
        VkBufferCopy vertexCopy{ 0 };
        vertexCopy.dstOffset = 0;
        vertexCopy.srcOffset = 0;
        vertexCopy.size = vertexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

        VkBufferCopy indexCopy{ 0 };
        indexCopy.dstOffset = 0;
        indexCopy.srcOffset = vertexBufferSize;
        indexCopy.size = indexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
        });

    vkutil::destroy_buffer(context, staging);

    return newSurface;
}

VertexMeshBuffer upload_mesh(const Context& context, std::span<Vertex> vertices)
{
    const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);

    VertexMeshBuffer newSurface;

    //create vertex buffer
    newSurface.vertexBuffer = vkutil::create_buffer(context, vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    //find the address of the vertex buffer
    VkBufferDeviceAddressInfo deviceAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = newSurface.vertexBuffer.buffer };
    newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(context.device, &deviceAdressInfo);


    AllocatedBuffer staging = vkutil::create_buffer(context, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);


    VmaAllocationInfo allocInfo;
    vmaGetAllocationInfo(context.allocator, staging.allocation, &allocInfo);

    void* data = allocInfo.pMappedData;

    // copy vertex buffer
    memcpy(data, vertices.data(), vertexBufferSize);

    context.immediate_submit([&](VkCommandBuffer cmd) {
        VkBufferCopy vertexCopy{ 0 };
        vertexCopy.dstOffset = 0;
        vertexCopy.srcOffset = 0;
        vertexCopy.size = vertexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);
        });

    vkutil::destroy_buffer(context, staging);

    return newSurface;
}


/*std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(VulkanEngine* engine, std::filesystem::path filePath) {
    std::cout << "Loading GLTF: " << filePath << std::endl;

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(filePath);

    constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers
        | fastgltf::Options::LoadExternalBuffers;

    fastgltf::Asset gltf;
    fastgltf::Parser parser{};

    auto load = parser.loadBinaryGLTF(&data, filePath.parent_path(), gltfOptions);
    if (load) {
        gltf = std::move(load.get());
    }
    else {
        fmt::print("Failed to load glTF: {} \n", fastgltf::to_underlying(load.error()));
        return {};
    }

    std::vector<std::shared_ptr<MeshAsset>> meshes;

    // use the same vectors for all meshes so that the memory doesnt reallocate as
    // often
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    for (fastgltf::Mesh& mesh : gltf.meshes) {
        MeshAsset newmesh;

        newmesh.name = mesh.name;

        // clear the mesh arrays each mesh, we dont want to merge them by error
        indices.clear();
        vertices.clear();

        for (auto&& p : mesh.primitives) {
            GeoSurface newSurface;
            newSurface.startIndex = (uint32_t)indices.size();
            newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

            size_t initial_vtx = vertices.size();

            // load indexes
            {
                fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + indexaccessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor,
                    [&](std::uint32_t idx) {
                        indices.push_back(idx + initial_vtx);
                    });
            }

            // load vertex positions
            {
                fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
                vertices.resize(vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                    [&](glm::vec3 v, size_t index) {
                        Vertex newvtx;
                        newvtx.position = v;
                        newvtx.normal = { 1, 0, 0 };
                        newvtx.color = glm::vec4{ 1.f };
                        newvtx.uv_x = 0;
                        newvtx.uv_y = 0;
                        vertices[initial_vtx + index] = newvtx;
                    });
            }

            // load vertex normals
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second],
                    [&](glm::vec3 v, size_t index) {
                        vertices[initial_vtx + index].normal = v;
                    });
            }

            // load UVs
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second],
                    [&](glm::vec2 v, size_t index) {
                        vertices[initial_vtx + index].uv_x = v.x;
                        vertices[initial_vtx + index].uv_y = v.y;
                    });
            }

            // load vertex colors
            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second],
                    [&](glm::vec4 v, size_t index) {
                        vertices[initial_vtx + index].color = v;
                    });
            }

            calculate_tangents(indices, vertices);

            newmesh.surfaces.push_back(newSurface);
        }

        // display the vertex normals
        constexpr bool OverrideColors = true;
        if (OverrideColors) {
            for (Vertex& vtx : vertices) {
                vtx.color = glm::vec4(vtx.normal, 1.f);
            }
        }
        newmesh.meshBuffers = upload_mesh(engine->_context, indices, vertices);

        meshes.emplace_back(std::make_shared<MeshAsset>(std::move(newmesh)));
    }

    return meshes;
}*/

AllocatedImage load_image_from_gltf(const Context& context, fastgltf::Asset& asset, fastgltf::Image& image) {
    AllocatedImage new_image{};

    auto load_from_bytes = [&](const std::byte* bytes_ptr, size_t total_length, size_t offset) {
        int width, height, channels;
        const stbi_uc* data_ptr = reinterpret_cast<const stbi_uc*>(bytes_ptr) + offset;

        unsigned char* data = stbi_load_from_memory(
            data_ptr,
            static_cast<int>(total_length),
            &width, &height, &channels,
            STBI_rgb_alpha
        );

        if (data) {
            VkExtent3D extent{ (uint32_t)width, (uint32_t)height, 1 };
            new_image = vkutil::create_image(context, data, extent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true, VK_SAMPLE_COUNT_1_BIT);
            stbi_image_free(data);
        }
        };

    std::visit(fastgltf::visitor{
        [](auto& arg) {},

        [&](fastgltf::sources::BufferView& view) {
            auto& buffer_view = asset.bufferViews[view.bufferViewIndex];
            auto& buffer = asset.buffers[buffer_view.bufferIndex];

            std::visit(fastgltf::visitor{
                [](auto& arg) {},
                [&](fastgltf::sources::ByteView& arr) {
                    load_from_bytes(arr.bytes.data(), buffer_view.byteLength, buffer_view.byteOffset);
                },
                [&](fastgltf::sources::Vector& vec) {
                    // This is the path your asset uses!
                    load_from_bytes(reinterpret_cast<const std::byte*>(vec.bytes.data()), buffer_view.byteLength, buffer_view.byteOffset);
                }
            }, buffer.data);
        },

        [&](fastgltf::sources::ByteView& arr) {
            load_from_bytes(arr.bytes.data(), arr.bytes.size(), 0);
        },

        [&](fastgltf::sources::Vector& vec) {
            load_from_bytes(reinterpret_cast<const std::byte*>(vec.bytes.data()), vec.bytes.size(), 0);
        }
        }, image.data);

    return new_image;
}


AllocatedImage uploadTexture(const Context& context,std::filesystem::path filename) {
    fmt::println("Loading texture {}", filename.string());
    int img_width = 0;
    int img_height = 0;
    int img_channels = 0;

    stbi_uc* pixels = stbi_load(filename.string().c_str(), &img_width, &img_height, &img_channels, STBI_rgb_alpha);

    if (pixels == nullptr) {
        throw std::runtime_error("Failed to load texture");
    }

    VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM; // maybe change
    VkExtent3D tex_extent;
    tex_extent.width = img_width;
    tex_extent.height = img_height;
    tex_extent.depth = 1;

    VkImageUsageFlags tex_flags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;// add more for mip

    AllocatedImage result = vkutil::create_image(context, pixels, tex_extent, tex_format, tex_flags, false, VK_SAMPLE_COUNT_1_BIT);
    stbi_image_free(pixels);
    return result;
}

/*void calculate_tangents(std::vector<uint32_t>& indices, std::vector<Vertex>& vertices) {
    uint32_t idx1;
    uint32_t idx2;
    uint32_t idx3;

    glm::vec3 p1;
    glm::vec3 p2;
    glm::vec3 p3;

    glm::vec2 uv1;
    glm::vec2 uv2;
    glm::vec2 uv3;

    glm::vec3 d_p1;
    glm::vec3 d_p2;

    glm::vec2 d_uv1;
    glm::vec2 d_uv2;

    glm::vec3 tangent;
    glm::vec3 bitangent;

    float coeff;

    float h1;
    float h2;
    float h3;
    // STEP 1: You MUST zero-initialize the arrays first!
    for (auto& v : vertices) {
        v.tangent = glm::vec4(0.0f);
    }

    // STEP 2: The Face Loop (Your math, cleaned up)
    for (size_t i = 0; i < indices.size(); i += 3) {
        idx1 = indices[i];
        idx2 = indices[i + 1];
        idx3 = indices[i + 2];

        p1 = vertices[idx1].position;
        p2 = vertices[idx2].position;
        p3 = vertices[idx3].position;

        uv1 = glm::vec2(vertices[idx1].uv_x, vertices[idx1].uv_y);
        uv2 = glm::vec2(vertices[idx2].uv_x, vertices[idx2].uv_y);
        uv3 = glm::vec2(vertices[idx3].uv_x, vertices[idx3].uv_y);

        d_p1 = p2 - p1;
        d_p2 = p3 - p1;

        d_uv1 = uv2 - uv1;
        d_uv2 = uv3 - uv1;

        float det = (d_uv1.x * d_uv2.y - d_uv1.y * d_uv2.x);
        // Protect against division by zero on degenerate UV mappings
        float coeff = (std::abs(det) > 1e-6f) ? (1.0f / det) : 0.0f;

        tangent = coeff * (d_uv2.y * d_p1 - d_uv1.y * d_p2);

        // ONLY accumulate the 3D directional vector components here
        vertices[idx1].tangent += glm::vec4(tangent, 0.0f);
        vertices[idx2].tangent += glm::vec4(tangent, 0.0f);
        vertices[idx3].tangent += glm::vec4(tangent, 0.0f);
    }

    // STEP 3: The Final Normalization & Handedness Pass
    for (auto& v : vertices) {
        glm::vec3 t = glm::vec3(v.tangent);
        glm::vec3 n = v.normal; // assuming your naming layout

        // Protect against zero-length vectors before normalizing
        if (glm::length2(t) > 1e-6f) {
            // Gram-Schmidt orthogonalization
            glm::vec3 ortho_t = glm::normalize(t - n * glm::dot(n, t));

            // Re-calculate local bitangent to evaluate true handedness sign cleanly
            // We use a safe arbitrary reference direction if bitangent breaks down
            glm::vec3 bitangent = glm::cross(n, ortho_t);

            // Use standard right-handed vs left-handed validation
            float w = (glm::dot(glm::cross(n, ortho_t), bitangent) < 0.0f) ? -1.0f : 1.0f;

            v.tangent = glm::vec4(ortho_t, w);
        }
        else {
            // Fallback for vertices with degenerate mappings
            v.tangent = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        }
    }

    for (auto& v : vertices) {
        fmt::print("tangent x: {}\n", v.tangent.x);
        fmt::print("tangent y: {}\n", v.tangent.y);
        fmt::print("tangent z: {}\n", v.tangent.z);
        fmt::print("tangent w: {}\n", v.tangent.w);
    }

}*/