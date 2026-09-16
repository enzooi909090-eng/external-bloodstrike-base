#pragma once
#include <Windows.h>
#include <cstdint>
#include <DirectXMath.h>
#include <deps/glm/glm.hpp>

namespace sdk
{
    inline bool aimbotEnabled = true;
    inline bool aimbotDrawFov = true;
    inline float aimbotFov = 120.0f;
    inline float aimbotSmooth = 5.0f;
    inline int aimbotKey = VK_RBUTTON; // Right mouse button
}

void RenderAim();
bool w2s(uint64_t cam, const glm::vec3 &world, glm::vec2 &out, bool returnAnyway = false);
bool MessiahMatrixAdd(const DirectX::XMFLOAT3X4 &bonemat, const DirectX::XMFLOAT3X4 &pos, glm::vec3 &out);