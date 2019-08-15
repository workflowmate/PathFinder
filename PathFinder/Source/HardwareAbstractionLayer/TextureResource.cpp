#include "TextureResource.hpp"

#include "../Foundation/Assert.hpp"

namespace HAL
{

    TextureResource::TextureResource(
        const Device& device, 
        ResourceFormat::FormatVariant format,
        ResourceFormat::TextureKind kind, 
        const Geometry::Dimensions& dimensions, 
        const ClearValue& optimizedClearValue, 
        ResourceState initialStateMask, 
        ResourceState expectedStateMask,
        std::optional<CPUAccessibleHeapType> heapType)
        :
        Resource(device, ResourceFormat(format, kind, dimensions), initialStateMask, expectedStateMask, optimizedClearValue, heapType),
        mDimensions{ dimensions }, mKind{ kind }, mFormat{ format } {}

    TextureResource::TextureResource(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr) : Resource(existingResourcePtr)
    {
        mFormat = ResourceFormat::FormatFromD3DFormat(D3DDescription().Format);
        mDimensions = { D3DDescription().Width, D3DDescription().Height, D3DDescription().DepthOrArraySize };

        switch (D3DDescription().Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D: mKind = HAL::ResourceFormat::TextureKind::Texture1D; break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D: mKind = HAL::ResourceFormat::TextureKind::Texture2D; break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D: mKind = HAL::ResourceFormat::TextureKind::Texture3D; break;
        default: assert_format(false, "Existing resource is not a texture");
        }
    }

    bool TextureResource::IsArray() const
    {
        switch (mKind)
        {
        case ResourceFormat::TextureKind::Texture1D: 
        case ResourceFormat::TextureKind::Texture2D: 
            return mDimensions.Depth > 1;

        default:
            return false;
        }
    }

}
