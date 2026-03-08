/*****************************************************************//**
 * \file   InputSystem.cpp
 *
 * \primary_author
 *      Ng Jordain Ying Zhe
 *
 * \brief
 *   Engine input subsystem that wraps GLFW keyboard/mouse state into a
 *   frame-based API (Down/Pressed/Released), with integer and enum
 *   query overloads, cursor locking, and mouse delta tracking.
 *********************************************************************/
#include "pch.h"
#include "InputSystem.h"
#include "WindowsSystem.h"
#include "GameManager.h"
#include "PhysicsSystem.h"
#include "CameraSystem.h"
#include "AudioSystem.h"
#include "ResourceManager.h"
#include "SceneManager.h"
#include "SpriteRenderSystem.h"
#include "EntityManager.h"

namespace Framework {

    void InputSystem::Initialize() {
        window_ = winSys_->window();
        if (!window_) return;

        glfwGetCursorPos(window_, &mouseX_, &mouseY_);
        prevMouseX_ = mouseX_;
        prevMouseY_ = mouseY_;
    }

    void InputSystem::Update() {
        if (!window_) return;

        glfwPollEvents();

        std::memcpy(keyPrev_, keyNow_, sizeof(keyNow_));
        std::memcpy(btnPrev_, btnNow_, sizeof(btnNow_));

        for (int i = 0; i < kMaxKeys; ++i) {
            int s = glfwGetKey(window_, i);
            keyNow_[i] = (s == GLFW_PRESS) || (s == GLFW_REPEAT);
        }

        for (int b = 0; b < kMaxButtons; ++b) {
            int s = glfwGetMouseButton(window_, b);
            btnNow_[b] = (s == GLFW_PRESS);
        }

        prevMouseX_ = mouseX_;
        prevMouseY_ = mouseY_;
        glfwGetCursorPos(window_, &mouseX_, &mouseY_);

        // =========================
        // --- GLOBAL SHORTCUT KEYS ---
        // =========================
        GameManager& gm = GameManager::GetInstance();
        EntityStorage* ecs = gm.GetSystems().ecs_;
        PhysicsSystem* physics = gm.GetSystems().physics_;
        CameraSystem* cameraSystem = gm.GetSystems().camera_;
        AudioSystem* audio = gm.GetSystems().audio_;
        ResourceManager* resources = gm.GetSystems().resourceManager_;
        SpriteRenderSystem* renderer = gm.GetSystems().spriteRenderer_;

        EntityManager entityMgr(*ecs);

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
        if (KeyPressed(Key::P)) {
            physics->SetPaused(!physics->IsPaused());
        }
        if (KeyPressed(Key::O)) {
            physics->ToggleStepMode();
        }
        if (KeyPressed(Key::I)) {
            physics->ExecuteNextStep();
        }

        // =========================
        // --- AUDIO REVERB TEST ---
        // =========================
        // 1: Reverb OFF (dry)
        if (KeyPressed(Key::Num1)) {
            if (audio) audio->EnableGroupReverb(AudioType::SFX, false);
        }

        // 2: Reverb ON (wet) - default preset
        if (KeyPressed(Key::Num2)) {
            if (audio) audio->EnableGroupReverb(AudioType::SFX, true);
        }

        // 3: Small room preset (audible but controlled)
        if (KeyPressed(Key::Num3)) {
            if (audio) {
                audio->EnableGroupReverb(AudioType::SFX, true);
                audio->SetGroupReverbWetDb(AudioType::SFX, -8.0f);
                audio->SetGroupReverbRoom(AudioType::SFX, 0.25f);
                audio->SetGroupReverbDecay(AudioType::SFX, 500.0f);
            }
        }

        // 4: Huge hall preset (very obvious)
        if (KeyPressed(Key::Num4)) {
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
        if (KeyPressed(Key::U)) {
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
        if (KeyPressed(Key::I)) {
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
        if (KeyPressed(Key::L)) {
            if (audio) audio->setMasterVolume(0.0f);
        }
        if (KeyPressed(Key::K)) {
            if (audio) audio->setMasterVolume(0.35f);
        }

        // --- RENDERER DEBUG TOGGLE ---
        if (KeyPressed(Key::C)) {
            if (renderer) renderer->ToggleColliders();
        }

        // --- CAMERA ZOOM CONTROLS ---
        if (cameraSystem) {
            Entity activeCam = cameraSystem->GetActiveCamera();
            if (activeCam != 0 && entityMgr.HasComponent(activeCam, CameraType)) {
                Camera& cam = entityMgr.GetComponent<Camera>(activeCam);

                if (KeyPressed(Key::Z)) cameraSystem->SetZoom(activeCam, cam.zoom * 0.8f);
                if (KeyPressed(Key::X)) cameraSystem->SetZoom(activeCam, cam.zoom * 1.2f);
                if (KeyPressed(Key::R)) cameraSystem->SetZoom(activeCam, 540.0f);
            }
        }

        // --- SCENE SWITCH ---
        if (KeyPressed(Key::N)) {
            SceneManager& sceneMgr = SceneManager::GetInstance();
            auto nextScene = sceneMgr.GetActiveScene().sceneIndex == 1 ?
                sceneMgr.GetSceneAt(2) :
                sceneMgr.GetSceneByName("Test Scene");
            sceneMgr.LoadScene(nextScene.name);
        }

        // FAKE PLAYER SWITCH ANIMATIONS
        if (KeyPressed(Key::F)) {
            for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, FakePlayerTagType)) {
                AnimationComponent& animationComponent = entityMgr.GetComponent<AnimationComponent>(entity);
                if (animationComponent.currentState != "RUNNING") {
                    animationComponent.currentState = "RUNNING";
                    Logger::Get().Info(DebugType::PlayerSystem) << "Fake Player Animation set to RUNNING" << std::endl;
                }
                else {
                    animationComponent.currentState = "IDLE";
                    Logger::Get().Info(DebugType::PlayerSystem) << "Fake Player Animation set to IDLE" << std::endl;
                }
            }
        }

#ifndef BUILD_EDITOR
        if (KeyPressed(Key::F11)) {
            winSys_->ToggleFullScreen();
        }
#endif
    }

    void InputSystem::ResetInputState() {
        std::memset(keyNow_, 0, sizeof(keyNow_));
        std::memset(keyPrev_, 0, sizeof(keyPrev_));
        std::memset(btnNow_, 0, sizeof(btnNow_));
        std::memset(btnPrev_, 0, sizeof(btnPrev_));
        SetCursorLocked(false);
    }

    bool InputSystem::KeyDown(int key) const {
        if (key < 0 || key >= kMaxKeys || keyboardLocked_) return false;
        return keyNow_[key];
    }
    bool InputSystem::KeyPressed(int key) const {
        if (key < 0 || key >= kMaxKeys || keyboardLocked_) return false;
        return keyNow_[key] && !keyPrev_[key];
    }
    bool InputSystem::KeyReleased(int key) const {
        if (key < 0 || key >= kMaxKeys || keyboardLocked_) return false;
        return !keyNow_[key] && keyPrev_[key];
    }

    bool InputSystem::MouseDown(int b) const {
        if (b < 0 || b >= kMaxButtons) return false;
        return btnNow_[b];
    }
    bool InputSystem::MousePressed(int b) const {
        if (b < 0 || b >= kMaxButtons) return false;
        return btnNow_[b] && !btnPrev_[b];
    }
    bool InputSystem::MouseReleased(int b) const {
        if (b < 0 || b >= kMaxButtons) return false;
        return !btnNow_[b] && btnPrev_[b];
    }

    bool InputSystem::KeyDown(Key k) const { return KeyDown(ToGlfwKey(k)); }
    bool InputSystem::KeyPressed(Key k) const { return KeyPressed(ToGlfwKey(k)); }
    bool InputSystem::KeyReleased(Key k) const { return KeyReleased(ToGlfwKey(k)); }

    bool InputSystem::MouseDown(MouseButton b) const { return MouseDown(ToGlfwMouse(b)); }
    bool InputSystem::MousePressed(MouseButton b) const { return MousePressed(ToGlfwMouse(b)); }
    bool InputSystem::MouseReleased(MouseButton b) const { return MouseReleased(ToGlfwMouse(b)); }

    bool InputSystem::KeyDown(char c) const { return KeyDown(ToGlfwKeyFromChar(c)); }
    bool InputSystem::KeyPressed(char c) const { return KeyPressed(ToGlfwKeyFromChar(c)); }
    bool InputSystem::KeyReleased(char c) const { return KeyReleased(ToGlfwKeyFromChar(c)); }

    void InputSystem::GetMousePos(double* x, double* y) const {
        if (x) *x = mouseX_;
        if (y) *y = mouseY_;
    }
    void InputSystem::GetMouseDelta(double* dx, double* dy) const {
        if (dx) *dx = mouseX_ - prevMouseX_;
        if (dy) *dy = mouseY_ - prevMouseY_;
    }
    void InputSystem::SetCursorLocked(bool locked) {
        cursorLocked_ = locked;
        if (!window_) return;
        glfwSetInputMode(window_, GLFW_CURSOR, locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }

    void InputSystem::SetKeyboardLocked(bool locked) {
        keyboardLocked_ = locked;
    }

    static int alphaToGlfw(int upper) { return GLFW_KEY_A + (upper - 'A'); }
    static int digitToGlfw(int digit) { return GLFW_KEY_0 + digit; }

    int InputSystem::ToGlfwKey(Key k) {
        switch (k) {
        case Key::A: return GLFW_KEY_A; case Key::B: return GLFW_KEY_B; case Key::C: return GLFW_KEY_C;
        case Key::D: return GLFW_KEY_D; case Key::E: return GLFW_KEY_E; case Key::F: return GLFW_KEY_F;
        case Key::G: return GLFW_KEY_G; case Key::H: return GLFW_KEY_H; case Key::I: return GLFW_KEY_I;
        case Key::J: return GLFW_KEY_J; case Key::K: return GLFW_KEY_K; case Key::L: return GLFW_KEY_L;
        case Key::M: return GLFW_KEY_M; case Key::N: return GLFW_KEY_N; case Key::O: return GLFW_KEY_O;
        case Key::P: return GLFW_KEY_P; case Key::Q: return GLFW_KEY_Q; case Key::R: return GLFW_KEY_R;
        case Key::S: return GLFW_KEY_S; case Key::T: return GLFW_KEY_T; case Key::U: return GLFW_KEY_U;
        case Key::V: return GLFW_KEY_V; case Key::W: return GLFW_KEY_W; case Key::X: return GLFW_KEY_X;
        case Key::Y: return GLFW_KEY_Y; case Key::Z: return GLFW_KEY_Z;
        case Key::Num0: return GLFW_KEY_0; case Key::Num1: return GLFW_KEY_1; case Key::Num2: return GLFW_KEY_2;
        case Key::Num3: return GLFW_KEY_3; case Key::Num4: return GLFW_KEY_4; case Key::Num5: return GLFW_KEY_5;
        case Key::Num6: return GLFW_KEY_6; case Key::Num7: return GLFW_KEY_7; case Key::Num8: return GLFW_KEY_8;
        case Key::Num9: return GLFW_KEY_9;
        case Key::Enter: return GLFW_KEY_ENTER; case Key::Escape: return GLFW_KEY_ESCAPE;
        case Key::Tab: return GLFW_KEY_TAB; case Key::Backspace: return GLFW_KEY_BACKSPACE;
        case Key::Space: return GLFW_KEY_SPACE;
        case Key::PageUp: return GLFW_KEY_PAGE_UP; case Key::PageDown: return GLFW_KEY_PAGE_DOWN;
        case Key::Home: return GLFW_KEY_HOME; case Key::End: return GLFW_KEY_END;
        case Key::Insert: return GLFW_KEY_INSERT; case Key::Delete: return GLFW_KEY_DELETE;
        case Key::Left: return GLFW_KEY_LEFT; case Key::Right: return GLFW_KEY_RIGHT;
        case Key::Up: return GLFW_KEY_UP; case Key::Down: return GLFW_KEY_DOWN;
        case Key::LShift: return GLFW_KEY_LEFT_SHIFT; case Key::RShift: return GLFW_KEY_RIGHT_SHIFT;
        case Key::LCtrl:  return GLFW_KEY_LEFT_CONTROL; case Key::RCtrl:  return GLFW_KEY_RIGHT_CONTROL;
        case Key::LAlt:   return GLFW_KEY_LEFT_ALT;     case Key::RAlt:   return GLFW_KEY_RIGHT_ALT;
        case Key::F1: return GLFW_KEY_F1; case Key::F2: return GLFW_KEY_F2; case Key::F3: return GLFW_KEY_F3;
        case Key::F4: return GLFW_KEY_F4; case Key::F5: return GLFW_KEY_F5; case Key::F6: return GLFW_KEY_F6;
        case Key::F7: return GLFW_KEY_F7; case Key::F8: return GLFW_KEY_F8; case Key::F9: return GLFW_KEY_F9;
        case Key::F10: return GLFW_KEY_F10; case Key::F11: return GLFW_KEY_F11; case Key::F12: return GLFW_KEY_F12;
        }
        return GLFW_KEY_UNKNOWN;
    }

    int InputSystem::ToGlfwKeyFromChar(char c) {
        if (c >= '0' && c <= '9') return digitToGlfw(c - '0');
        int u = std::toupper(static_cast<unsigned char>(c));
        if (u >= 'A' && u <= 'Z') return alphaToGlfw(u);
        return GLFW_KEY_UNKNOWN;
    }

    int InputSystem::ToGlfwMouse(MouseButton b) {
        switch (b) {
        case MouseButton::Left:   return GLFW_MOUSE_BUTTON_LEFT;
        case MouseButton::Right:  return GLFW_MOUSE_BUTTON_RIGHT;
        case MouseButton::Middle: return GLFW_MOUSE_BUTTON_MIDDLE;
        case MouseButton::Button4:return GLFW_MOUSE_BUTTON_4;
        case MouseButton::Button5:return GLFW_MOUSE_BUTTON_5;
        }
        return GLFW_MOUSE_BUTTON_LEFT;
    }

} // namespace Framework