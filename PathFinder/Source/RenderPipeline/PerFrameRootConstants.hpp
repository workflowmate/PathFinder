#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "SceneGPUStorage.hpp"

namespace PathFinder
{

    struct PerFrameRootConstants
    {
        GPUCamera CurrentFrameCamera;
        GPUCamera PreviousFrameCamera;
    };

}
