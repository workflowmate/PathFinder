#include "Resource.hpp"
#include "Utils.h"

#include "../Foundation/STDHelpers.hpp"

namespace HAL
{

    Resource::Resource(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr)
        : mResource(existingResourcePtr), mDescription(mResource->GetDesc()) {}

    Resource::Resource(
        const Device& device,
        const ResourceFormat& format, 
        ResourceState initialStateMask,
        ResourceState expectedStateMask,
        std::optional<CPUAccessibleHeapType> heapType)
        :
        mDescription{ format.D3DResourceDescription() },
        mInitialStates{ initialStateMask },
        mExpectedStates{ expectedStateMask }
    {
        D3D12_HEAP_PROPERTIES heapProperties{};
        D3D12_RESOURCE_STATES initialStates = D3DResourceState(initialStateMask);

        if (heapType)
        {
            switch (*heapType)
            {
            case CPUAccessibleHeapType::Upload:
                heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
                initialStates |= D3D12_RESOURCE_STATE_GENERIC_READ;
                break;

            case CPUAccessibleHeapType::Readback:
                heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
                break;
            }
        }
        else {
            heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        }

        SetExpectedUsageFlags(expectedStateMask);

        bool isSubjectForClearing = EnumMaskBitSet(expectedStateMask, ResourceState::RenderTarget) || EnumMaskBitSet(expectedStateMask, ResourceState::DepthWrite);

        ThrowIfFailed(device.D3DDevice()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &mDescription,
            initialStates,
            isSubjectForClearing ? format.D3DOptimizedClearValue() : nullptr,
            IID_PPV_ARGS(&mResource)
        ));

        UINT GPUMask = 0;
        D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device.D3DDevice()->GetResourceAllocationInfo(GPUMask, 1, &mDescription);

        mMemoryAlignment = allocInfo.Alignment;
        mMemoryFootprint = allocInfo.SizeInBytes;
    }

    Resource::~Resource() {}

    D3D12_GPU_VIRTUAL_ADDRESS Resource::GPUVirtualAddress() const
    {
        return mResource->GetGPUVirtualAddress();
    }

    bool Resource::CanImplicitlyPromoteFromCommonStateToState(ResourceState state) const
    {
        return false;
    }

    bool Resource::CanImplicitlyDecayToCommonStateFromState(ResourceState state) const
    {
        return false;
    }

    void Resource::SetExpectedUsageFlags(ResourceState stateMask)
    {
        if (EnumMaskBitSet(stateMask, ResourceState::RenderTarget))
        {
            mDescription.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }

        if (EnumMaskBitSet(stateMask, ResourceState::DepthRead) ||
            EnumMaskBitSet(stateMask, ResourceState::DepthWrite))
        {
            mDescription.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        }

        if (EnumMaskBitSet(stateMask, ResourceState::UnorderedAccess))
        {
            mDescription.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        if (!EnumMaskBitSet(stateMask, ResourceState::PixelShaderAccess) &&
            !EnumMaskBitSet(stateMask, ResourceState::NonPixelShaderAccess) &&
            !EnumMaskBitSet(stateMask, ResourceState::RenderTarget) &&
            !EnumMaskBitSet(stateMask, ResourceState::GenericRead) &&
            !EnumMaskBitSet(stateMask, ResourceState::IndirectArgument) &&
            !EnumMaskBitSet(stateMask, ResourceState::CopySource) &&
            !EnumMaskBitSet(stateMask, ResourceState::DepthRead) &&
            !EnumMaskBitSet(stateMask, ResourceState::ConstantBuffer) &&
            !EnumMaskBitSet(stateMask, ResourceState::UnorderedAccess))
        {
            mDescription.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }
    }

}
