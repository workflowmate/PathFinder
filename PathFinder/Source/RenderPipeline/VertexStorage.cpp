#include "VertexStorage.hpp"

namespace PathFinder
{

    VertexStorage::VertexStorage(HAL::Device* device)
        : mDevice{ device },
        mCommandAllocator{ *device },
        mCommandList{ *device, mCommandAllocator },
        mCommandQueue{ *device }, 
        mFence{ *device } {}


    VertexStorageLocation VertexStorage::AddVertices(const Vertex1P1N1UV1T1BT* vertices, uint64_t vertexCount, const uint32_t* indices, uint64_t indexCount)
    {
        return WriteToUploadBuffers(vertices, vertexCount, indices, indexCount);
    }

    VertexStorageLocation VertexStorage::AddVertices(const Vertex1P1N1UV* vertices, uint64_t vertexCount, const uint32_t* indices, uint64_t indexCount)
    {
        return WriteToUploadBuffers(vertices, vertexCount, indices, indexCount);
    }

    VertexStorageLocation VertexStorage::AddVertices(const Vertex1P3* vertices, uint64_t vertexCount, const uint32_t* indices, uint64_t indexCount)
    {
        return WriteToUploadBuffers(vertices, vertexCount, indices, indexCount);
    }

    template <class Vertex>
    void VertexStorage::AllocateUploadBuffersIfNeeded()
    {
        auto& package = std::get<UploadBufferPackage<Vertex>>(mUploadBuffers);

        if (!package.VertexBuffer)
        {
            package.VertexBuffer = std::make_unique<HAL::BufferResource<Vertex>>(
                *mDevice, mUploadBufferCapacity, 1, HAL::ResourceState::Common, HAL::ResourceState::CopySource, HAL::HeapType::Upload);
        }

        if (!package.IndexBuffer)
        {
            package.IndexBuffer = std::make_unique<HAL::BufferResource<uint32_t>>(
                *mDevice, mUploadBufferCapacity, 1, HAL::ResourceState::Common, HAL::ResourceState::CopySource, HAL::HeapType::Upload);
        }
    }

    template <class Vertex>
    void VertexStorage::AllocateFinalBuffersIfNeeded()
    {
        auto& uploadBuffers = std::get<UploadBufferPackage<Vertex>>(mUploadBuffers);
        auto& finalBuffers = std::get<FinalBufferPackage<Vertex>>(mFinalBuffers);

        if (uploadBuffers.VertexBuffer)
        {
            finalBuffers.VertexBuffer = std::make_unique<HAL::BufferResource<Vertex>>(
                *mDevice, uploadBuffers.CurrentVertexOffset, 1, HAL::ResourceState::CopyDestination, HAL::ResourceState::VertexBuffer);
        }
        
        if (uploadBuffers.IndexBuffer)
        {
            finalBuffers.IndexBuffer = std::make_unique<HAL::BufferResource<uint32_t>>(
                *mDevice, uploadBuffers.CurrentIndexOffset, 1, HAL::ResourceState::CopyDestination, HAL::ResourceState::IndexBuffer);
        }
    }

    template <class Vertex>
    VertexStorageLocation PathFinder::VertexStorage::WriteToUploadBuffers(const Vertex* vertices, uint64_t vertexCount, const uint32_t* indices, uint64_t indexCount)
    {
        AllocateUploadBuffersIfNeeded<Vertex>();

        auto& package = std::get<UploadBufferPackage<Vertex>>(mUploadBuffers);

        VertexStorageLocation location{ package.CurrentVertexOffset, vertexCount, package.CurrentIndexOffset, indexCount };

        package.VertexBuffer->Write(location.VertexBufferOffset, vertices, location.VertexCount);
        package.IndexBuffer->Write(location.IndexBufferOffset, indices, location.IndexCount);
        
        package.CurrentVertexOffset += location.VertexCount;
        package.CurrentIndexOffset += location.IndexCount;

        return location;
    }

    template <class Vertex>
    void VertexStorage::CopyBuffersToDefaultHeap()
    {
        AllocateFinalBuffersIfNeeded<Vertex>();

        auto& uploadBuffers = std::get<UploadBufferPackage<Vertex>>(mUploadBuffers);
        auto& finalBuffers = std::get<FinalBufferPackage<Vertex>>(mFinalBuffers);

        if (uploadBuffers.VertexBuffer && finalBuffers.VertexBuffer)
        {
            mCommandList.CopyBufferRegion(*uploadBuffers.VertexBuffer, *finalBuffers.VertexBuffer, 0, uploadBuffers.CurrentVertexOffset, 0);
            mCommandList.TransitionResourceState({ HAL::ResourceState::CopyDestination, HAL::ResourceState::VertexBuffer, finalBuffers.VertexBuffer.get() });

            finalBuffers.VertexBufferDescriptor = std::make_unique<HAL::VertexBufferDescriptor>(*finalBuffers.VertexBuffer);
        }
        
        if (uploadBuffers.IndexBuffer && finalBuffers.IndexBuffer)
        {
            mCommandList.CopyBufferRegion(*uploadBuffers.IndexBuffer, *finalBuffers.IndexBuffer, 0, uploadBuffers.CurrentIndexOffset, 0);
            mCommandList.TransitionResourceState({ HAL::ResourceState::CopyDestination, HAL::ResourceState::IndexBuffer, finalBuffers.IndexBuffer.get() });

            finalBuffers.IndexBufferDescriptor = std::make_unique<HAL::IndexBufferDescriptor>(*finalBuffers.IndexBuffer, HAL::ResourceFormat::Color::R32_Unsigned);
        }
        
        mCommandList.Close();
        mCommandQueue.ExecuteCommandList(mCommandList);
        mCommandQueue.StallCPUUntilDone(mFence);
        mCommandAllocator.Reset();
        mCommandList.Reset(mCommandAllocator);

        uploadBuffers.VertexBuffer = nullptr;
        uploadBuffers.IndexBuffer = nullptr;
    }

    void VertexStorage::TransferDataToGPU()
    {
        CopyBuffersToDefaultHeap<Vertex1P1N1UV1T1BT>();
        CopyBuffersToDefaultHeap<Vertex1P1N1UV>();
        CopyBuffersToDefaultHeap<Vertex1P3>();
    }

}
