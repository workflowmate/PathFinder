#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "GraphicAPIObject.hpp"
#include "Device.hpp"
#include "CommandList.hpp"
#include "Fence.hpp"

namespace HAL
{

    class CommandQueue : public GraphicAPIObject
    {
    public:
        CommandQueue(const Device& device, D3D12_COMMAND_LIST_TYPE commandListType);
        virtual ~CommandQueue() = 0;

        void SignalFence(const Fence& fence);
        void WaitFence(const Fence& fence);
        void SetDebugName(const std::string& name) override;

    protected:
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> mQueue;

    public:
        inline const auto D3DPtr() const { return mQueue.Get(); }
    };

    class GraphicsCommandQueue : public CommandQueue {
    public:
        GraphicsCommandQueue(const Device& device);
        ~GraphicsCommandQueue() = default;

        void ExecuteCommandList(const GraphicsCommandList& list);
    };

    class ComputeCommandQueue : public CommandQueue {
    public:
        ComputeCommandQueue(const Device& device);
        ~ComputeCommandQueue() = default;

        void ExecuteCommandList(const ComputeCommandList& list);
    };

    class CopyCommandQueue : public CommandQueue {
    public:
        CopyCommandQueue(const Device& device);
        ~CopyCommandQueue() = default;

        void ExecuteCommandList(const CopyCommandList& list);
    };

}