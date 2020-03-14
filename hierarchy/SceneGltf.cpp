#include "SceneGltf.h"
#include "ParseGltf.h"
#include <vector>
#include <fstream>
#include <string>
#include <gltfformat/glb.h>
#include <gltfformat/bin.h>

template <class T>
static std::vector<uint8_t> read_allbytes(T path)
{
    std::vector<uint8_t> buffer;

    // open the file for binary reading
    std::ifstream file;
    file.open(path, std::ios_base::binary);
    if (file.is_open())
    {
        // get the length of the file
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        // read the file
        buffer.resize(fileSize);
        file.read(reinterpret_cast<char *>(buffer.data()), fileSize);
    }

    return buffer;
}

namespace hierarchy
{

SceneNodePtr SceneGltf::LoadFromPath(const std::string &path)
{
    auto bytes = read_allbytes(path);
    if (bytes.empty())
    {
        return nullptr;
    }

    return LoadGlbBytes(bytes.data(), (int)bytes.size());
}

SceneNodePtr SceneGltf::LoadFromPath(const std::wstring &path)
{
    auto bytes = read_allbytes(path);
    if (bytes.empty())
    {
        return nullptr;
    }

    return LoadGlbBytes(bytes.data(), (int)bytes.size());
}

SceneNodePtr SceneGltf::LoadGlbBytes(const uint8_t *bytes, int byteLength)
{
    gltfformat::glb glb;
    if (!glb.load(bytes, byteLength))
    {
        return nullptr;
    }

    auto gltf = ::ParseGltf(glb.json.p, glb.json.size);

    gltfformat::bin bin(gltf, glb.bin.p, glb.bin.size);

    // build scene
    std::vector<SceneImagePtr> images;
    images.reserve(gltf.images.size());
    for (auto &gltfImage : gltf.images)
    {
        auto &bufferView = gltf.bufferViews[gltfImage.bufferView.value()];
        auto bytes = bin.get_bytes(bufferView);

        // TO_PNG
        auto image = SceneImage::Load(bytes.p, bytes.size);

        images.push_back(image);
    }

    std::vector<SceneMaterialPtr> materials;
    materials.reserve(gltf.materials.size());
    for (auto &gltfMaterial : gltf.materials)
    {
        auto material = SceneMaterial::Create();
        if (gltfMaterial.pbrMetallicRoughness.has_value())
        {
            auto &pbr = gltfMaterial.pbrMetallicRoughness.value();
            if (pbr.baseColorTexture.has_value())
            {
                auto &gltfTexture = gltf.textures[pbr.baseColorTexture.value().index.value()];
                auto image = images[gltfTexture.source.value()];
                material->colorImage = image;
            }

            //material->SetColor(pbr.baseColorFactor.value());
        }
        material->name = gltfMaterial.name;

        materials.push_back(material);
    }

    // for (auto &gltfNode : gltf.nodes)
    // {
    //     auto node = SceneNode::Create();

    // }

    auto node = SceneNode::Create("gltf");
    node->EnableGizmo(true);
    for (auto &gltfMesh : gltf.meshes)
    {
        for (auto &gltfPrimitive : gltfMesh.primitives)
        {
            auto mesh = SceneMesh::Create();

            node->AddMesh(mesh);

            for (auto [k, v] : gltfPrimitive.attributes)
            {
                if (k == "POSITION")
                {
                    auto accessor = gltf.accessors[v];
                    auto [p, size] = bin.get_bytes(accessor);
                    mesh->SetVertices(Semantics::Position, 12, p, size);
                }
                else if (k == "NORMAL")
                {
                    auto accessor = gltf.accessors[v];
                    auto [p, size] = bin.get_bytes(accessor);
                    mesh->SetVertices(Semantics::Normal, 12, p, size);
                }
                else if (k == "TEXCOORD_0")
                {
                    auto accessor = gltf.accessors[v];
                    auto [p, size] = bin.get_bytes(accessor);
                    mesh->SetVertices(Semantics::TexCoord, 8, p, size);
                }
                else
                {
                    auto a = 0;
                }
            }

            if (gltfPrimitive.material.has_value())
            {
                auto index = gltfPrimitive.indices.value();
                auto accessor = gltf.accessors[index];
                auto [p, size] = bin.get_bytes(accessor);
                int stride = 0;
                switch (accessor.componentType.value())
                {
                case gltfformat::AccessorComponentType::UNSIGNED_SHORT:
                case gltfformat::AccessorComponentType::SHORT:
                    stride = 2;
                    break;

                case gltfformat::AccessorComponentType::UNSIGNED_INT:
                    stride = 4;
                    break;

                default:
                    throw;
                }
                mesh->SetIndices(stride, p, size);

                auto material = materials[gltfPrimitive.material.value()];
                mesh->submeshes.push_back({
                    // .draw_offset = 0,
                    .draw_count = (uint32_t)accessor.count.value(),
                    .material = material,
                });
            }
            else
            {
                throw "not indices";
            }
        }
    }
    return node;
}

} // namespace hierarchy