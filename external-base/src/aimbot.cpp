#define GLM_ENABLE_EXPERIMENTAL
#include "aimbot.hpp"
#include "../deps/imgui/imgui.h"
#include "../deps/imgui/imgui_impl_dx11.h"
#include "../deps/imgui/imgui_impl_win32.h"
#include "../deps/imgui/imgui_internal.h"
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>
#include <deps/glm/glm.hpp>
#include <deps/glm/gtc/type_ptr.hpp>
#include <deps/glm/gtx/string_cast.hpp>
#include <deps/math/math.h>
#include <deps/memory/memory.h>
#include <iostream>
#include <string>

using namespace DirectX;

using namespace sdk;

namespace bloodstrike
{
  namespace renderer
  {
    extern uint64_t camera;
    extern uint64_t localActor;
  } // namespace renderer
  namespace offsets
  {
    constexpr uint64_t Messiah__ClientEngine = 0x65F7AD0;
    constexpr uint64_t Messiah__EntityList = 0x6E4D0D8;
  } // namespace offsets
} // namespace bloodstrike

extern std::unique_ptr<c_memory> memory;

void RenderAim()
{
  ImVec2 screenCenter(ImGui::GetIO().DisplaySize.x / 2.0f,
                      ImGui::GetIO().DisplaySize.y / 2.0f);

  if (aimbotDrawFov)
  {
    ImGui::GetBackgroundDrawList()->AddCircle(
        screenCenter, aimbotFov, ImColor(255, 255, 255, 120), 64, 1.0f);
  }

  if (!aimbotEnabled || !(GetAsyncKeyState(aimbotKey) & 0x8000))
    return;

  uint64_t base = memory->get_module_address();
  uint64_t ClientEngine = memory->read<uint64_t>(
      base + bloodstrike::offsets::Messiah__ClientEngine);
  if (!ClientEngine)
    return;
  uint64_t IGameplay = memory->read<uint64_t>(ClientEngine + 0x58);
  if (!IGameplay)
    return;
  uint64_t ClientPlayer = memory->read<uint64_t>(IGameplay + 0x58);
  if (!ClientPlayer)
    return;
  bloodstrike::renderer::camera = memory->read<uint64_t>(ClientPlayer + 0x238);
  bloodstrike::renderer::localActor =
      memory->read<uint64_t>(ClientPlayer + 0x288);
  if (!bloodstrike::renderer::camera || !bloodstrike::renderer::localActor)
    return;
  uint64_t entityListStart =
      memory->read<uint64_t>(base + bloodstrike::offsets::Messiah__EntityList);
  if (!entityListStart)
    return;
  uint64_t head = memory->read<uint64_t>(entityListStart + 0x8);
  if (!head)
    return;
  uint64_t currentActor = memory->read<uint64_t>(head);

  float closestDistance = aimbotFov;
  glm::vec2 bestTarget(0.0f, 0.0f);
  bool foundTarget = false;

  if (currentActor)
  {
    do
    {
      uint64_t actorInstance = memory->read<uint64_t>(currentActor + 0x18);
      if (!actorInstance)
      {
        currentActor = memory->read<uint64_t>(currentActor);
        continue;
      }
      uint64_t actorProps = memory->read<uint64_t>(actorInstance + 0x278);
      if (!actorProps)
      {
        currentActor = memory->read<uint64_t>(currentActor);
        continue;
      }
      uint64_t actorComponent = memory->read<uint64_t>(actorProps + 0x18);
      if (!actorComponent)
      {
        currentActor = memory->read<uint64_t>(currentActor);
        continue;
      }
      uint64_t IEntity = memory->read<uint64_t>(actorComponent + 0x40);
      if (!IEntity)
      {
        currentActor = memory->read<uint64_t>(currentActor);
        continue;
      }
      uint64_t entityMask = memory->read<uint64_t>(IEntity + 0x2e0);
      if (entityMask != 2)
      {
        currentActor = memory->read<uint64_t>(currentActor);
        continue;
      }
      if (IEntity == bloodstrike::renderer::localActor)
      {
        currentActor = memory->read<uint64_t>(currentActor);
        continue;
      }
      uint64_t IArea = memory->read<uint64_t>(IEntity + 0x88);
      if (IArea == 0x0)
      {
        currentActor = memory->read<uint64_t>(currentActor);
        continue;
      }
      uint64_t pose = memory->read<uint64_t>(actorInstance + 0x18);
      if (!pose)
      {
        currentActor = memory->read<uint64_t>(currentActor);
        continue;
      }
      uint64_t BipedPose = memory->read<uint64_t>(pose + 0x90);
      if (!BipedPose)
      {
        currentActor = memory->read<uint64_t>(currentActor);
        continue;
      }
      BipedPose += 0x8;
      uint64_t neckBoneStart = memory->read<uint64_t>(BipedPose + (7 * 0x8));
      if (!neckBoneStart)
      {
        currentActor = memory->read<uint64_t>(currentActor);
        continue;
      }
      XMFLOAT3X4 dxTrans = memory->read<XMFLOAT3X4>(IEntity + 0x58);
      glm::vec3 _neck;
      MessiahMatrixAdd(memory->read<XMFLOAT3X4>(neckBoneStart + 0x30), dxTrans,
                       _neck);
      glm::vec2 neck2D;
      if (w2s(bloodstrike::renderer::camera, _neck, neck2D, false))
      {
        float distToCrosshair =
            std::sqrt(std::pow(neck2D.x - screenCenter.x, 2) +
                      std::pow(neck2D.y - screenCenter.y, 2));
        if (distToCrosshair < closestDistance)
        {
          closestDistance = distToCrosshair;
          bestTarget = neck2D;
          foundTarget = true;
        }
      }
      currentActor = memory->read<uint64_t>(currentActor);
    } while (currentActor != head && currentActor != 0);
  }

  if (foundTarget)
  {
    float deltaX = bestTarget.x - screenCenter.x;
    float deltaY = bestTarget.y - screenCenter.y;
    if (aimbotSmooth > 0.0f)
    {
      deltaX /= aimbotSmooth;
      deltaY /= aimbotSmooth;
    }
    mouse_event(MOUSEEVENTF_MOVE, (DWORD)deltaX, (DWORD)deltaY, 0, 0);
  }
}
