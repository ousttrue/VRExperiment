#pragma once
#include "Helper.h"
#include <d3dcompiler.h>

namespace d12u
{

class Shader : NonCopyable
{
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<ID3D12PipelineState> m_pipelineState;
    int m_generation = -1;

    // keep semantics string
    std::vector<std::string> m_semantics;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_layout;
    bool InputLayoutFromReflection(const ComPtr<ID3D12ShaderReflection> &reflection);

    std::string m_name;

public:
    Shader(const std::string &name)
        : m_name(name)
    {
    }

    const D3D12_INPUT_ELEMENT_DESC *inputLayout(int *count) const
    {
        *count = (int)m_layout.size();
        return m_layout.data();
    }
    bool Initialize(const ComPtr<ID3D12Device> &device,
                    const ComPtr<ID3D12RootSignature> &rootSignature,
                    const std::string &source, int generation);
    bool Set(const ComPtr<ID3D12GraphicsCommandList> &commandList);
};

} // namespace d12u
