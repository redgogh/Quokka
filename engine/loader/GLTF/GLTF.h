#pragma once

#include <tiny_gltf.h>

namespace GLTF {

    inline static void _LoadTinyGLTFModel(const char* path, tinygltf::Model* pGLTFModel)
    {
        tinygltf::TinyGLTF loader;
        std::string tinyGlTFErr;
        std::string tinyGLTFWarn;
        loader.LoadBinaryFromFile(pGLTFModel, &tinyGLTFWarn, &tinyGlTFErr, path);

        if (!tinyGlTFErr.empty())
            throw std::runtime_error(tinyGlTFErr);

        if (!tinyGLTFWarn.empty())
            printf("[GLTF] load GLTF warn: %s\n", tinyGLTFWarn.c_str());
    }

    inline static Mesh LoadMeshFromGLTF(const char* glTF_File) {
        tinygltf::Model model;
        _LoadTinyGLTFModel(glTF_File, &model);

        Mesh mesh;

        const auto& gltfMesh = model.meshes[0];
        const auto& primitive = gltfMesh.primitives[0];

        /* ----------------------------------- */
        /*              POSITION               */
        /* ----------------------------------- */
        {
            const auto& accessor = model.accessors[primitive.attributes.at("POSITION")];
            const auto& view = model.bufferViews[accessor.bufferView];
            const auto& buffer = model.buffers[view.buffer];

            const float* data = reinterpret_cast<const float*>(&buffer.data[view.byteOffset + accessor.byteOffset]);

            mesh.vertices.resize(accessor.count);
            for (size_t i = 0; i < accessor.count; i++)
                mesh.vertices[i].pos = glm::make_vec3(data + i * 3);
        }

        /* ----------------------------------- */
        /*               NORMAL                */
        /* ----------------------------------- */
        {
            if (primitive.attributes.count("NORMAL")) {
                const auto &accessor = model.accessors[primitive.attributes.at("NORMAL")];
                const auto &view = model.bufferViews[accessor.bufferView];
                const auto &buffer = model.buffers[view.buffer];

                const float *data = reinterpret_cast<const float *>(&buffer.data[view.byteOffset + accessor.byteOffset]);

                for (size_t i = 0; i < accessor.count; i++)
                    mesh.vertices[i].normal = glm::make_vec3(data + i * 3);
            }
        }

        /* ----------------------------------- */
        /*              TEXCOORD               */
        /* ----------------------------------- */
        {
            if (primitive.attributes.count("TEXCOORD_0")) {
                const auto &accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                const auto &view = model.bufferViews[accessor.bufferView];
                const auto &buffer = model.buffers[view.buffer];

                const float *data = reinterpret_cast<const float *>(&buffer.data[view.byteOffset + accessor.byteOffset]);

                for (size_t i = 0; i < accessor.count; i++)
                    mesh.vertices[i].uv = glm::make_vec2(data + i * 2);
            }
        }

        /* ----------------------------------- */
        /*               INDICES               */
        /* ----------------------------------- */
        {
            if (primitive.indices >= 0) {
                const auto &accessor = model.accessors[primitive.indices];
                const auto &view = model.bufferViews[accessor.bufferView];
                const auto &buffer = model.buffers[view.buffer];

                const float *data = reinterpret_cast<const float *>(&buffer.data[view.byteOffset + accessor.byteOffset]);

                mesh.indices.resize(accessor.count);

                switch (accessor.componentType) {
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                        const uint8_t* buf = reinterpret_cast<const uint8_t*>(data);
                        for (size_t i = 0; i < accessor.count; i++)
                            mesh.indices[i] = buf[i];
                        break;
                    }
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                        const uint16_t* buf = reinterpret_cast<const uint16_t*>(data);
                        for (size_t i = 0; i < accessor.count; i++)
                            mesh.indices[i] = buf[i];
                        break;
                    }
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                        const uint32_t* buf = reinterpret_cast<const uint32_t*>(data);
                        for (size_t i = 0; i < accessor.count; i++)
                            mesh.indices[i] = buf[i];
                        break;
                    }
                    default:
                        throw std::runtime_error("[GLTF] unsupported index component type");
                }
            }
        }

        return mesh;
    }

}