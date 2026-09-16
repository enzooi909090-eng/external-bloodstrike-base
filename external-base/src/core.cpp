#define GLM_ENABLE_EXPERIMENTAL
#include <deps/glm/glm.hpp>
#include <deps/glm/gtc/type_ptr.hpp>
#include <deps/glm/gtx/string_cast.hpp>

#include "aimbot.hpp"
#include <DirectXMath.h>
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <deps/math/math.hpp>
#include <deps/memory/memory.h>
#include <deps/window/window.h>
#include <format>
#include <inttypes.h>
#include <iostream>
#include <string>
#include <vector>

using namespace DirectX;

bool show_menu = false;
bool esp_enabled = true;
bool esp_box = true;
bool esp_distance = true;
bool esp_bones = false;
bool esp_lines = true;

float esp_box_color[4] = {255, 0, 0, 255};
float esp_distance_color[4] = {255, 255, 0, 255};
float esp_bones_color[4] = {0, 255, 0, 255};
float esp_lines_color[4] = {255, 0, 255, 255};


namespace Global
{
    constexpr uint64_t MessiahEntityList = 0x8A6EA58;
    constexpr uint64_t MessiahClientEngine = 0x7FCD2E8;
}
namespace Steam // works but need to update the field offsets
{
    constexpr uint64_t Messiah__ClientEngine = 0x65F7AD0; // 49 8B D6 48 8B CF E8 ? ? ? ? 48 83 3D ? ? ? ? ? 0F 85 ? ? ? ? BA ? ? ? ? B9 -> on cmp qword ( cmp     cs:qword_, 0 )
    constexpr uint64_t Messiah__EntityList = 0x6E4D0D8; // 48 8B D9 48 8D 05 ?? ?? ?? ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 83 C1 ?? 48 89 5C 24 ?? 48 8B 51 (   mov     rcx, cs:qword_ )
}

namespace bloodstrike
{
  namespace offsets
  {
    constexpr uint64_t Messiah__ClientEngine = Global::MessiahClientEngine;
    constexpr uint64_t Messiah__EntityList = Global::MessiahEntityList;
  }

  namespace renderer
  {
    uint64_t camera = 0;
    uint64_t localActor = 0;
  }
}

void DrawLabel(const std::string &text, const glm::vec2 &pos, float color[4],
               bool outline = true)
{
  ImVec2 textPos(pos.x, pos.y);
  ImU32 textColor =
      IM_COL32((int)color[0], (int)color[1], (int)color[2], (int)color[3]);
  ImU32 outlineColor = IM_COL32(0, 0, 0, (int)color[3]);

  if (outline)
  {
    ImGui::GetBackgroundDrawList()->AddText(
        ImVec2(textPos.x - 1, textPos.y - 1), outlineColor, text.c_str());
    ImGui::GetBackgroundDrawList()->AddText(
        ImVec2(textPos.x + 1, textPos.y - 1), outlineColor, text.c_str());
    ImGui::GetBackgroundDrawList()->AddText(
        ImVec2(textPos.x - 1, textPos.y + 1), outlineColor, text.c_str());
    ImGui::GetBackgroundDrawList()->AddText(
        ImVec2(textPos.x + 1, textPos.y + 1), outlineColor, text.c_str());
    ImGui::GetBackgroundDrawList()->AddText(ImVec2(textPos.x, textPos.y - 1),
                                            outlineColor, text.c_str());
    ImGui::GetBackgroundDrawList()->AddText(ImVec2(textPos.x, textPos.y + 1),
                                            outlineColor, text.c_str());
    ImGui::GetBackgroundDrawList()->AddText(ImVec2(textPos.x - 1, textPos.y),
                                            outlineColor, text.c_str());
    ImGui::GetBackgroundDrawList()->AddText(ImVec2(textPos.x + 1, textPos.y),
                                            outlineColor, text.c_str());
  }

  ImGui::GetBackgroundDrawList()->AddText(textPos, textColor, text.c_str());
}

void draw_esp_overlay(ImDrawList *draw_list)
{
  if (!draw_list || !esp_enabled)
    return;

  update_camera_and_local();
  if (!bloodstrike::renderer::camera || !bloodstrike::renderer::localActor)
    return;

  glm::vec3 local_pos = get_local_position();

  uint64_t base = memory->get_module_address();
  uint64_t entityListStart =
      memory->read<uint64_t>(base + bloodstrike::offsets::Messiah__EntityList);
  if (!entityListStart)
    return;

  uint64_t head = memory->read<uint64_t>(entityListStart + 0x8);
  if (!head)
    return;

  uint64_t currentActor = memory->read<uint64_t>(head);
  int valid = 0;

  ImVec2 screenCenter = ImGui::GetBackgroundDrawList()->GetClipRectMax();
  screenCenter.x = (screenCenter.x - ImGui::GetBackgroundDrawList()->GetClipRectMin().x) / 2.0f;
  screenCenter.y = ImGui::GetBackgroundDrawList()->GetClipRectMax().y - ImGui::GetBackgroundDrawList()->GetClipRectMin().y;
  glm::vec2 bottomCenter(screenCenter.x, screenCenter.y - 50);

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
      bool isMisc = (entityMask != 2);

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

      DirectX::XMFLOAT3X4 transform =
          memory->read<DirectX::XMFLOAT3X4>(IEntity + 0x58);
      glm::vec3 entity_pos(transform._14, transform._24, transform._34);

      float dx = entity_pos.x - local_pos.x;
      float dy = entity_pos.y - local_pos.y;
      float dz = entity_pos.z - local_pos.z;
      float distance = sqrtf(dx * dx + dy * dy + dz * dz);
      int distance_m = (int)(distance / 5);

      if (distance_m < 1000 && !isMisc)
      {
        glm::vec2 screenPos;
        w2s(bloodstrike::renderer::camera, entity_pos, screenPos);

        glm::vec2 neck{}, spine1{}, spine2{}, spine3{}, pelvis{}, buttCheekL{},
            buttCheekR{}, kneeL{}, kneeR{}, footL{}, footR{}, sholL{}, elbowL{},
            wristL{}, sholR{}, elbowR{}, wristR{}, headPos{};
        glm::vec3 _neck, _spine1, _spine2, _spine3, _pelvis, _buttCheekL,
            _buttCheekR, _kneeL, _kneeR, _footL, _footR, _sholL, _elbowL,
            _wristL, _sholR, _elbowR, _wristR, _head;

        uint64_t boneStart = memory->read<uint64_t>(BipedPose + (7 * 0x8));
        if (boneStart)
        {
          DirectX::XMFLOAT3X4 boneMatrix =
              memory->read<DirectX::XMFLOAT3X4>(boneStart + 0x30);
          MessiahMatrixAdd(boneMatrix, transform, _neck);
          w2s(bloodstrike::renderer::camera, _neck, neck);
        }

        boneStart = memory->read<uint64_t>(BipedPose + (8 * 0x8));
        if (boneStart)
        {
          DirectX::XMFLOAT3X4 boneMatrix =
              memory->read<DirectX::XMFLOAT3X4>(boneStart + 0x30);
          MessiahMatrixAdd(boneMatrix, transform, _head);
          w2s(bloodstrike::renderer::camera, _head, headPos);
        }

        boneStart = memory->read<uint64_t>(BipedPose + (6 * 0x8));
        if (boneStart)
        {
          XMFLOAT3X4 boneMatrix = memory->read<XMFLOAT3X4>(boneStart + 0x30);
          MessiahMatrixAdd(boneMatrix, transform, _spine1);
          w2s(bloodstrike::renderer::camera, _spine1, spine1);
        }

        boneStart = memory->read<uint64_t>(BipedPose + (5 * 0x8));
        if (boneStart)
        {
          XMFLOAT3X4 boneMatrix = memory->read<XMFLOAT3X4>(boneStart + 0x30);
          MessiahMatrixAdd(boneMatrix, transform, _spine2);
          w2s(bloodstrike::renderer::camera, _spine2, spine2);
        }

        boneStart = memory->read<uint64_t>(BipedPose + (4 * 0x8));
        if (boneStart)
        {
          XMFLOAT3X4 boneMatrix = memory->read<XMFLOAT3X4>(boneStart + 0x30);
          MessiahMatrixAdd(boneMatrix, transform, _spine3);
          w2s(bloodstrike::renderer::camera, _spine3, spine3);
        }

        boneStart = memory->read<uint64_t>(BipedPose + (3 * 0x8));
        if (boneStart)
        {
          XMFLOAT3X4 boneMatrix = memory->read<XMFLOAT3X4>(boneStart + 0x30);
          MessiahMatrixAdd(boneMatrix, transform, _pelvis);
          w2s(bloodstrike::renderer::camera, _pelvis, pelvis);
        }

        boneStart = memory->read<uint64_t>(BipedPose + (22 * 0x8));
        if (boneStart)
        {
          XMFLOAT3X4 boneMatrix = memory->read<XMFLOAT3X4>(boneStart + 0x30);
          MessiahMatrixAdd(boneMatrix, transform, _buttCheekL);
          w2s(bloodstrike::renderer::camera, _buttCheekL, buttCheekL);
        }

        boneStart = memory->read<uint64_t>(BipedPose + (18 * 0x8));
        if (boneStart)
        {
          XMFLOAT3X4 boneMatrix = memory->read<XMFLOAT3X4>(boneStart + 0x30);
          MessiahMatrixAdd(boneMatrix, transform, _buttCheekR);
          w2s(bloodstrike::renderer::camera, _buttCheekR, buttCheekR);
        }

        boneStart = memory->read<uint64_t>(BipedPose + (23 * 0x8));
        if (boneStart)
        {
          XMFLOAT3X4 boneMatrix = memory->read<XMFLOAT3X4>(boneStart + 0x30);
          MessiahMatrixAdd(boneMatrix, transform, _kneeL);
          w2s(bloodstrike::renderer::camera, _kneeL, kneeL);
        }

        boneStart = memory->read<uint64_t>(BipedPose + (19 * 0x8));
        if (boneStart)
        {
          XMFLOAT3X4 boneMatrix = memory->read<XMFLOAT3X4>(boneStart + 0x30);
          MessiahMatrixAdd(boneMatrix, transform, _kneeR);
          w2s(bloodstrike::renderer::camera, _kneeR, kneeR);
        }

        boneStart = memory->read<uint64_t>(BipedPose + (24 * 0x8));
        if (boneStart)
        {
          XMFLOAT3X4 boneMatrix = memory->read<XMFLOAT3X4>(boneStart + 0x30);
          MessiahMatrixAdd(boneMatrix, transform, _footL);
          w2s(bloodstrike::renderer::camera, _footL, footL);
        }

        boneStart = memory->read<uint64_t>(BipedPose + (20 * 0x8));
        if (boneStart)
        {
          XMFLOAT3X4 boneMatrix = memory->read<XMFLOAT3X4>(boneStart + 0x30);
          MessiahMatrixAdd(boneMatrix, transform, _footR);
          w2s(bloodstrike::renderer::camera, _footR, footR);
        }

        boneStart = memory->read<uint64_t>(BipedPose + (14 * 0x8));
        if (boneStart)
        {
          XMFLOAT3X4 boneMatrix = memory->read<XMFLOAT3X4>(boneStart + 0x30);
          MessiahMatrixAdd(boneMatrix, transform, _sholL);
          w2s(bloodstrike::renderer::camera, _sholL, sholL);
        }

        boneStart = memory->read<uint64_t>(BipedPose + (9 * 0x8));
        if (boneStart)
        {
          XMFLOAT3X4 boneMatrix = memory->read<XMFLOAT3X4>(boneStart + 0x30);
          MessiahMatrixAdd(boneMatrix, transform, _sholR);
          w2s(bloodstrike::renderer::camera, _sholR, sholR);
        }

        boneStart = memory->read<uint64_t>(BipedPose + (15 * 0x8));
        if (boneStart)
        {
          XMFLOAT3X4 boneMatrix = memory->read<XMFLOAT3X4>(boneStart + 0x30);
          MessiahMatrixAdd(boneMatrix, transform, _elbowL);
          w2s(bloodstrike::renderer::camera, _elbowL, elbowL);
        }

        boneStart = memory->read<uint64_t>(BipedPose + (10 * 0x8));
        if (boneStart)
        {
          XMFLOAT3X4 boneMatrix = memory->read<XMFLOAT3X4>(boneStart + 0x30);
          MessiahMatrixAdd(boneMatrix, transform, _elbowR);
          w2s(bloodstrike::renderer::camera, _elbowR, elbowR);
        }

        boneStart = memory->read<uint64_t>(BipedPose + (16 * 0x8));
        if (boneStart)
        {
          XMFLOAT3X4 boneMatrix = memory->read<XMFLOAT3X4>(boneStart + 0x30);
          MessiahMatrixAdd(boneMatrix, transform, _wristL);
          w2s(bloodstrike::renderer::camera, _wristL, wristL);
        }

        boneStart = memory->read<uint64_t>(BipedPose + (11 * 0x8));
        if (boneStart)
        {
          XMFLOAT3X4 boneMatrix = memory->read<XMFLOAT3X4>(boneStart + 0x30);
          MessiahMatrixAdd(boneMatrix, transform, _wristR);
          w2s(bloodstrike::renderer::camera, _wristR, wristR);
        }

        glm::vec2 headScreen = headPos;
        glm::vec2 footScreen = footL;
        if (footScreen.x == 0 && footScreen.y == 0)
        {
          footScreen = footR;
        }
        if (footScreen.x == 0 && footScreen.y == 0)
        {
          footScreen = kneeL;
        }
        if (footScreen.x == 0 && footScreen.y == 0)
        {
          footScreen = pelvis;
        }

        if (headScreen.x != 0 && headScreen.y != 0 && footScreen.x != 0 && footScreen.y != 0)
        {
          valid++;

          float boxHeight = footScreen.y - headScreen.y;
          float boxWidth = boxHeight * 0.45f;
          float boxX = headScreen.x - (boxWidth / 2.0f);
          float boxY = headScreen.y;

          std::string dist_txt = std::format("{:d}m", distance_m);

          if (esp_lines)
          {
            ImColor linesCol(esp_lines_color[0], esp_lines_color[1],
                             esp_lines_color[2], esp_lines_color[3]);
            ImGui::GetBackgroundDrawList()->AddLine(
                ImVec2(bottomCenter.x, bottomCenter.y),
                ImVec2(headScreen.x, headScreen.y),
                linesCol, 1.5f);
          }

          if (esp_bones)
          {
            ImColor bonesCol(esp_bones_color[0], esp_bones_color[1],
                             esp_bones_color[2], esp_bones_color[3]);

            BoneConnection(neck, spine1, bonesCol);
            BoneConnection(spine1, spine2, bonesCol);
            BoneConnection(spine2, spine3, bonesCol);
            BoneConnection(spine3, pelvis, bonesCol);

            BoneConnection(spine1, sholL, bonesCol);
            BoneConnection(sholL, elbowL, bonesCol);
            BoneConnection(elbowL, wristL, bonesCol);

            BoneConnection(spine1, sholR, bonesCol);
            BoneConnection(sholR, elbowR, bonesCol);
            BoneConnection(elbowR, wristR, bonesCol);

            BoneConnection(pelvis, buttCheekL, bonesCol);
            BoneConnection(buttCheekL, kneeL, bonesCol);
            BoneConnection(kneeL, footL, bonesCol);

            BoneConnection(pelvis, buttCheekR, bonesCol);
            BoneConnection(buttCheekR, kneeR, bonesCol);
            BoneConnection(kneeR, footR, bonesCol);
          }

          if (esp_box)
          {
            ImGui::GetBackgroundDrawList()->AddRect(
                ImVec2(boxX, boxY),
                ImVec2(boxX + boxWidth, boxY + boxHeight),
                ImColor(esp_box_color[0], esp_box_color[1], esp_box_color[2],
                        esp_box_color[3]),
                0.0f, 0, 1.5f);
          }

          if (esp_distance)
          {
            DrawLabel(dist_txt,
                      glm::vec2(boxX + (boxWidth / 2.0f), boxY + boxHeight + 5.0f),
                      esp_distance_color, true);
          }
        }
      }

      currentActor = memory->read<uint64_t>(currentActor);
    } while (currentActor != head && currentActor != 0);
  }
}

int main()
{
  SetConsoleTitle(L"External Cheat Base");
  if (!memory->attach_to_process(L"BloodStrike.exe"))
  {
    std::cin.get();
    return false;
  }

  c_overlay overlay;
  if (!overlay.create())
  {
    std::cin.get();
    return false;
  }

  overlay.set_click_through(true);

  int scan_counter = 0;
  overlay.set_click_through(!show_menu);

  while (overlay.running)
  {
    if (GetAsyncKeyState(VK_INSERT) & 1)
    {
      show_menu = !show_menu;
      overlay.set_click_through(!show_menu);
    }

    overlay.start();

    draw_esp_overlay(ImGui::GetBackgroundDrawList());
    RenderAim();

    if (show_menu)
    {
      ImGui::SetNextWindowSize({500, 500});
      ImGui::Begin("External Cheat Base by Yazz");

      ImGui::Text("ESP Features:");
      ImGui::Checkbox("ESP Enabled", &esp_enabled);
      ImGui::Checkbox("Box ESP", &esp_box);
      ImGui::Checkbox("Distance ESP", &esp_distance);
      ImGui::Checkbox("Bones ESP", &esp_bones);
      ImGui::Checkbox("Lines ESP", &esp_lines);
      ImGui::Separator();

      ImGui::Text("Aimbot:");
      ImGui::Checkbox("Aimbot Enabled", &sdk::aimbotEnabled);
      ImGui::Checkbox("Draw FOV", &sdk::aimbotDrawFov);
      ImGui::SliderFloat("Aimbot FOV", &sdk::aimbotFov, 10.0f, 500.0f, "%.1f");
      ImGui::SliderFloat("Aimbot Smooth", &sdk::aimbotSmooth, 1.0f, 50.0f, "%.1f");
      ImGui::Text("Aimbot Key: Right Mouse Button");
      ImGui::Separator();

      ImGui::Text("ESP Colors:");
      ImGui::ColorEdit4("Box Color", esp_box_color);
      ImGui::ColorEdit4("Distance Color", esp_distance_color);
      ImGui::ColorEdit4("Bones Color", esp_bones_color);
      ImGui::ColorEdit4("Lines Color", esp_lines_color);

      ImGui::End();
    }

    overlay.end();
  }

  return 0;
}