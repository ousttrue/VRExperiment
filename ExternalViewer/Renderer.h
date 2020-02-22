#pragma once
#include <memory>

namespace screenstate
{
    struct ScreenState;
}

class Renderer
{
    class Impl *m_impl = nullptr;

public:
    Renderer(int maxModelCount);
    ~Renderer();
    void OnFrame(void *hwnd, const screenstate::ScreenState &state,
                 const std::shared_ptr<class Model> *models, int count);
    void AddModel(int index,
                  const uint8_t *vertices, int verticesByteLength, int vertexStride,
                  const uint8_t *indices, int indicesByteLength, int indexStride);
};
