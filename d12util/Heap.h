#pragma once
#include "Helper.h"
#include <vector>

namespace d12u
{

class Heap : NonCopyable
{
    ComPtr<ID3D12DescriptorHeap> m_cbvHeap;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_cpuHandles;
    std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> m_gpuHandles;

public:
    ID3D12DescriptorHeap *Get() { return m_cbvHeap.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle(int index) const { return m_cpuHandles[index]; }
    D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle(int index) const { return m_gpuHandles[index]; }
    void Initialize(const ComPtr<ID3D12Device> &device,
                    class ConstantBufferBase *const*resources, UINT resourceCount,
                    UINT groupCount);
};

} // namespace d12u