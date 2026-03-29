/*********************************************************************//**
 * \file   SB_InputSystem.cpp
 *
 * \primary_author
 *      Ng Jordain Ying Zhe (80%)
 * \secondary_author
 *      Yar Xuan (20%)
 *
 * \brief   StageBound specific input system. Contains all testing and debugging keyboard shortcuts.
 *
 * \copyright
 *   Copyright (C) 2025 DigiPen Institute of Technology.
 *   Reproduction or disclosure of this file or its contents without
 *   the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "pch.h"
#include "SB_InputSystem.h"
#include "InputSystem.h"
#include "GameManager.h"

using EngineInput = Framework::InputSystem;

namespace Framework {

    void SB_InputSystem::Update() {

        GameManager& gm = GameManager::GetInstance();
        EngineInput* input = gm.GetSystems().input_;

        EntityStorage* ecs = gm.GetSystems().ecs_;
        EntityManager entityMgr(*ecs);

        PhysicsSystem* physics = gm.GetSystems().physics_;
        CameraSystem* cameraSystem = gm.GetSystems().camera_;
        AudioSystem* audio = gm.GetSystems().audio_;
        ResourceManager* resources = gm.GetSystems().resourceManager_;
        SpriteRenderSystem* renderer = gm.GetSystems().spriteRenderer_;

        // ---- helper: player position (for 3D audio test) ----
        auto GetPlayerPos = [&]() -> Vector2D {
            Vector2D p{ 0.0f, 0.0f };
            auto players = entityMgr.GetAllEntitiesWithComponent(true, PlayerTagType);
            if (!players.empty() && entityMgr.HasComponent(players[0], TransformType)) {
                p = entityMgr.GetComponent<Transform>(players[0]).position;
            }
            return p;
        };

        // --- PHYSICS TOGGLES ---
        if (input->KeyPressed(EngineInput::Key::P)) {
            physics->SetPaused(!physics->IsPaused());
        }
        if (input->KeyPressed(EngineInput::Key::O)) {
            physics->ToggleStepMode();
        }
        if (input->KeyPressed(EngineInput::Key::I)) {
            physics->ExecuteNextStep();
        }

        // =========================
        // --- AUDIO REVERB TEST ---
        // =========================
        // 1: Reverb OFF (dry)
        if (input->KeyPressed(EngineInput::Key::Num1)) {
            if (audio) audio->EnableGroupReverb(AudioType::SFX, false);
        }

        // 2: Reverb ON (wet) - default preset
        if (input->KeyPressed(EngineInput::Key::Num2)) {
            if (audio) audio->EnableGroupReverb(AudioType::SFX, true);
        }

        // 3: Small room preset (audible but controlled)
        if (input->KeyPressed(EngineInput::Key::Num3)) {
            if (audio) {
                audio->EnableGroupReverb(AudioType::SFX, true);
                audio->SetGroupReverbWetDb(AudioType::SFX, -8.0f);
                audio->SetGroupReverbRoom(AudioType::SFX, 0.25f);
                audio->SetGroupReverbDecay(AudioType::SFX, 500.0f);
            }
        }

        // 4: Huge hall preset (very obvious)
        if (input->KeyPressed(EngineInput::Key::Num4)) {
            if (audio) {
                audio->EnableGroupReverb(AudioType::SFX, true);
                audio->SetGroupReverbWetDb(AudioType::SFX, 0.0f);
                audio->SetGroupReverbRoom(AudioType::SFX, 1.0f);
                audio->SetGroupReverbDecay(AudioType::SFX, 2000.0f);
            }
        }

        // =========================
        // --- AUDIO SPATIAL TEST ---
        // =========================
        // U: play 3D SFX left of player
        if (input->KeyPressed(EngineInput::Key::U)) {
            if (audio && resources) {
                try {
                    Vector2D p = GetPlayerPos();
                    FMOD::Sound& sfx = resources->get<FMOD::Sound>("jingle.ogg");
                    audio->play3D(sfx, AudioType::SFX, 1.0f, { p.x - 250.0f, p.y }, 50.0f, 700.0f, false);
                }
                catch (...) {}
            }
        }

        // I: play 3D SFX right of player
        if (input->KeyPressed(EngineInput::Key::I)) {
            if (audio && resources) {
                try {
                    Vector2D p = GetPlayerPos();
                    FMOD::Sound& sfx = resources->get<FMOD::Sound>("jingle.ogg");
                    audio->play3D(sfx, AudioType::SFX, 1.0f, { p.x + 250.0f, p.y }, 50.0f, 700.0f, false);
                }
                catch (...) {}
            }
        }

        // --- MASTER VOLUME QUICK TEST (optional keep) ---
        if (input->KeyPressed(EngineInput::Key::L)) {
            if (audio) audio->setMasterVolume(0.0f);
        }
        if (input->KeyPressed(EngineInput::Key::K)) {
            if (audio) audio->setMasterVolume(0.35f);
        }

        // --- RENDERER DEBUG TOGGLE ---
        if (input->KeyPressed(EngineInput::Key::C)) {
            if (renderer) renderer->ToggleColliders();
        }

        // --- CAMERA ZOOM CONTROLS ---
        if (cameraSystem) {
            Entity activeCam = cameraSystem->GetActiveCamera();
            if (activeCam != 0 && entityMgr.HasComponent(activeCam, CameraType)) {
                Camera& cam = entityMgr.GetComponent<Camera>(activeCam);

                if (input->KeyPressed(EngineInput::Key::Z)) cameraSystem->SetZoom(activeCam, cam.zoom * 0.8f);
                if (input->KeyPressed(EngineInput::Key::X)) cameraSystem->SetZoom(activeCam, cam.zoom * 1.2f);
                if (input->KeyPressed(EngineInput::Key::R)) cameraSystem->SetZoom(activeCam, 540.0f);
            }
        }
    }

} // namespace Framework