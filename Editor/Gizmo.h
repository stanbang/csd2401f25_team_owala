/*****************************************************************//**
* \file   Gizmo.h
*
* \primary_author
*       Yar Xuan
* \secondary_author
*      
*
* \brief 
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include <imgui.h>
#include <string>
#include "Vector2D.h"
#include "Entity.h"

// forward declarations
namespace Framework {
    class EntityStorage;
    class CameraSystem;
}

namespace Editor {
    class Editor; // forward declaration

    enum class GizmoMode { None, Translate, Rotate, Scale };
    enum class GizmoAxis { None, X, Y, Center, RotateRing };

    class GizmoSystem {
    public:
        GizmoMode currentMode_ = GizmoMode::None;

        explicit GizmoSystem(Editor& editor, Framework::EntityStorage& ecs, Framework::CameraSystem& camera);

        // --- Invisible Entities stuff ---
        // Draw a small green dot placeholder for invisible entities to make them selectable.
        void DrawInvisPlaceholders(ImDrawList* drawList);
        // Helper function to check if an invisible entity was selected
        Entity IsInvisEntityClicked();

        // --- Gizmos Drawing ---
        void DrawGizmos(ImDrawList* drawList, int selectedEntityIndex);

        void DrawTranslateGizmo(ImDrawList* drawList, ImVec2 origin);
        void DrawRotateGizmo(ImDrawList* drawList, ImVec2 origin, Entity entity);
        void DrawScaleGizmo(ImDrawList* drawList, ImVec2 origin);

        // --- Gizmos Dragging (Actual transformation logic) ---
        void Update(int selectedEntityIndex, Entity activeCamera);
        void HandleStopDrag();
        void HandleStartDrag();
        void ApplyTransformation(const ImVec2 mouseDelta, const Entity activeCamera);

        
        // --- Helper functions ---
        // Helper function to check if gizmos is toggled
        bool IsGizmosToggled() const {
            return currentMode_ != GizmoMode::None;
        }
        
        // Helper function to check if user mouse is hovering over any gizmo axis
        bool IsGizmosHover() const { return hoveredAxis_ != GizmoAxis::None; }

        // Setter for gizmo mode
        void SetGizmoMode(GizmoMode mode) {
            currentMode_ = mode;
        }

    private:
        Editor* editor_ = nullptr;
        Framework::EntityStorage& ecs_;
        Framework::CameraSystem* cameraSystem_ = nullptr;

        // Circle radius for placeholder for invisible entities
        const float invisCircleRad_ = 8.0f;

        // Gizmo Dimensions
        const float axisLength_ = 80.0f; // The arrows are always 80px long
        const float axisThickness_ = 8.0f;
        const float handleSize_ = 10.0f; // Size of the square/triangle at tip
        const float centerCubeHalfsize_ = 8.0f;

        // Stores which axis is currently being hovered
        GizmoAxis hoveredAxis_ = GizmoAxis::None;
        // Stores which axis is currently active (being clicked on)
        GizmoAxis activeAxis_ = GizmoAxis::None; 
        // True when the mouse is down and we are actively modifying an entity
        bool isDragging_ = false;
        // Stores the mouse position from the *previous* frame for delta calculation
        ImVec2 lastMousePos_; 
        // Stores the selected entity ID for easy access during the drag
        Entity draggedEntity_ = 0;

        // For Undo/Redo system (HistoryManager)
        std::string beforeGizmosJSON_{}, afterGizmosJSON_{};
    };

}