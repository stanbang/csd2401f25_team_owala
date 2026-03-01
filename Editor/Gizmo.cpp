/*****************************************************************//**
* \file   Gizmo.cpp
*
* \primary_author
*       Yar Xuan (80%)
* \secondary_author
*       Jason Lou (20%)
*
* \brief  Implementation file for Gizmo system in the Editor module.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "pch.h"
#include "Gizmo.h"
#include "Logger.h"
#include "Editor.h"
#include "EntityManager.h"
#include "EntitySerializer.h"
#include "GameManager.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace Editor;
using namespace Framework;

GizmoSystem::GizmoSystem(Editor& editor) : editor_(&editor) {}

// --- Invisible Entities stuff ---
void GizmoSystem::DrawInvisPlaceholders(ImDrawList* drawList) {
    // Draw placeholder green dot for SpawnPoint (invisible entities)
    EntityManager entityMgr(GameManager::GetInstance().GetECS());
    entityMgr.ForEachEntityWithComponents(
        [&](Entity en) {
            Transform& transform = entityMgr.GetComponent<Transform>(en);
            ImVec2 gizmoScreenPos = editor_->WorldToImGuiScreen(transform.position);
            drawList->AddCircleFilled(gizmoScreenPos, invisCircleRad_, IM_COL32(0, 255, 0, 255), 12);
        }, true, SpawnPointTagType, LoadedTagType, TransformType
    );
}

Entity GizmoSystem::IsInvisEntityClicked() {
    ImVec2 mouseScreenPos = ImGui::GetMousePos();

    EntityManager entityMgr(GameManager::GetInstance().GetECS());

    Entity clickedEntity = 0;
    bool found = false;
    entityMgr.ForEachEntityWithComponents(
        [&](Entity en) {
            if (found) return;

            Transform& transform = entityMgr.GetComponent<Transform>(en);
            ImVec2 gizmoScreenPos = editor_->WorldToImGuiScreen(transform.position);

            float dx = mouseScreenPos.x - gizmoScreenPos.x;
            float dy = mouseScreenPos.y - gizmoScreenPos.y;
            float distSq = dx * dx + dy * dy;

            if (distSq <= (invisCircleRad_ * invisCircleRad_))
            {
                found = true;
                clickedEntity = en;
            }
        }, true, SpawnPointTagType, LoadedTagType, TransformType
    );

    return clickedEntity;
}

// --- Gizmos Drawing ---
void GizmoSystem::DrawGizmos(ImDrawList* drawList, int selectedEntityIndex) {
    // no entity selected. no need to draw transform gizmos
    if (selectedEntityIndex <= 1) return;

    // minus 1 to get actual entity ID
    Entity entity = selectedEntityIndex - 1;

    // get position in IMGUI screen coords
    EntityManager entityMgr(GameManager::GetInstance().GetECS());
    Vector2D pos{};

    if (entityMgr.HasComponent(entity, TransformType)) {
        pos = entityMgr.GetComponent<Transform>(entity).position;
    }

    ImVec2 screenOrigin = ImVec2(pos.x, pos.y);
    // GUI elements use screen coords
    if (entityMgr.HasComponent(entity, GUITagType)) {
        screenOrigin = editor_->NormalizedToImGuiScreen(pos);
    }
    else {
        screenOrigin = editor_->WorldToImGuiScreen(pos);
    }

    // draw gizmos according to mode
    switch (currentMode_) {
    case GizmoMode::Translate:
        DrawOutline(drawList, entity);
        DrawTranslateGizmo(drawList, screenOrigin);
        break;
    case GizmoMode::Rotate:
        DrawOutline(drawList, entity);
        DrawRotateGizmo(drawList, screenOrigin, entity);
        break;
    case GizmoMode::Scale:
        DrawOutline(drawList, entity);
        DrawScaleGizmo(drawList, screenOrigin);
        break;
    }
}

void GizmoSystem::DrawOutline(ImDrawList* drawList, Entity entity) {
    EntityManager entityMgr(GameManager::GetInstance().GetECS());

    if (!entityMgr.HasComponent(entity, TransformType)) return;

    auto& transform = entityMgr.GetComponent<Transform>(entity);
    Vector2D worldPos = transform.position;
    Vector2D scale = transform.scale;

    // Get Rotation
    float rotationRads = -transform.rotation.textureAngle;
    float c = std::cos(rotationRads);
    float s = std::sin(rotationRads);

    float hw = 10.f, hh = 10.f;

    if (entityMgr.HasComponent(entity, TextureComponentType)) {
        TextureComponent texture = entityMgr.GetComponent<TextureComponent>(entity);
        hw = texture.HalfSize.x * scale.x;
        hh = texture.HalfSize.y * scale.y;
    }
    else if (entityMgr.HasComponent(entity, TextComponentType)) {
        TextComponent text = entityMgr.GetComponent<TextComponent>(entity);
        hw = text.halfSize.x * scale.x;
        hh = text.halfSize.y * scale.y;
    }

    bool isGui = entityMgr.HasComponent(entity, GUITagType);

    ImVec2 screenCenter = isGui ? editor_->NormalizedToImGuiScreen(worldPos)
        : editor_->WorldToImGuiScreen(worldPos);

    ImVec2 screenEdge = isGui ? editor_->NormalizedToImGuiScreen({ worldPos.x + hw, worldPos.y + hh })
        : editor_->WorldToImGuiScreen({ worldPos.x + hw, worldPos.y + hh });

    // Use absolute difference to get exact screen-pixel dimensions regardless of axis directions
    float screenHw = std::abs(screenEdge.x - screenCenter.x);
    float screenHh = std::abs(screenEdge.y - screenCenter.y);

    // Screen-space local corners (TopLeft, TopRight, BotRight, BotLeft)
    ImVec2 localScreenCorners[4] = {
        { -screenHw, -screenHh },
        {  screenHw, -screenHh },
        {  screenHw,  screenHh },
        { -screenHw,  screenHh }
    };

    ImVec2 screenCorners[4];

    for (int i = 0; i < 4; i++) {
        // Rotate in screen space
        float rX = localScreenCorners[i].x * c - localScreenCorners[i].y * s;
        float rY = localScreenCorners[i].x * s + localScreenCorners[i].y * c;

        // Translate to final screen position
        screenCorners[i] = { screenCenter.x + rX, screenCenter.y + rY };
    }

    // Draw the Polyline (Closed loop)
    drawList->AddPolyline(screenCorners, 4, IM_COL32(255, 0, 0, 255), true, 2.0f);
}

void GizmoSystem::DrawTranslateGizmo(ImDrawList* drawList, ImVec2 origin) {
    ImVec2 mousePos = ImGui::GetMousePos();

    // End points for axes
    ImVec2 xTip = ImVec2(origin.x + axisLength_, origin.y);
    ImVec2 yTip = ImVec2(origin.x, origin.y - axisLength_); // Up is Negative in ImGui

    // Default Colors (R, G, White)
    ImU32 colX = IM_COL32(200, 50, 50, 255);
    ImU32 colY = IM_COL32(50, 200, 50, 255);
    ImU32 colCenter = IM_COL32(255, 255, 255, 255);

    // --- HOVER LOGIC (Simple AABB/Distance check) ---
    // Check Center Square
    bool hoverCenter = (abs(mousePos.x - origin.x) < 10.0f && abs(mousePos.y - origin.y) < 10.0f);

    // Check X Axis (Horizontal Box)
    bool hoverX = (mousePos.x >= origin.x && mousePos.x <= xTip.x && abs(mousePos.y - origin.y) < 8.0f);

    // Check Y Axis (Vertical Box) - Y is inverted
    bool hoverY = (mousePos.y <= origin.y && mousePos.y >= yTip.y && abs(mousePos.x - origin.x) < 8.0f);

    // Highlight on Hover & update hoveredAxis_
    hoveredAxis_ = GizmoAxis::None;
    if (hoverCenter) {
        colCenter = IM_COL32(255, 255, 0, 255); // Yellow
        hoveredAxis_ = GizmoAxis::Center;
    }
    else if (hoverX) {
        colX = IM_COL32(255, 100, 100, 255);    // Bright Red
        hoveredAxis_ = GizmoAxis::X;
    }
    else if (hoverY) {
        colY = IM_COL32(100, 255, 100, 255);    // Bright Green
        hoveredAxis_ = GizmoAxis::Y;
    }

    // --- DRAWING ---

    // X-Axis (Line + Triangle Tip)
    drawList->AddLine(origin, xTip, colX, axisThickness_);
    drawList->AddTriangleFilled(
        ImVec2(xTip.x, xTip.y - handleSize_),
        ImVec2(xTip.x, xTip.y + handleSize_),
        ImVec2(xTip.x + handleSize_ * 1.5f, xTip.y),
        colX
    );

    // Y-Axis (Line + Triangle Tip)
    drawList->AddLine(origin, yTip, colY, axisThickness_);
    drawList->AddTriangleFilled(
        ImVec2(yTip.x - handleSize_, yTip.y),
        ImVec2(yTip.x + handleSize_, yTip.y),
        ImVec2(yTip.x, yTip.y - handleSize_ * 1.5f),
        colY
    );

    // Center Quad
    drawList->AddRectFilled(
        ImVec2(origin.x - centerCubeHalfsize_, origin.y - centerCubeHalfsize_),
        ImVec2(origin.x + centerCubeHalfsize_, origin.y + centerCubeHalfsize_),
        colCenter
    );
}

void GizmoSystem::DrawScaleGizmo(ImDrawList* drawList, ImVec2 origin) {
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 xTip = ImVec2(origin.x + axisLength_, origin.y);
    ImVec2 yTip = ImVec2(origin.x, origin.y - axisLength_);

    ImU32 colX = IM_COL32(200, 50, 50, 255);
    ImU32 colY = IM_COL32(50, 200, 50, 255);
    ImU32 colCenter = IM_COL32(200, 200, 200, 255); // Gray for uniform scale

    // Hover Logic
    hoveredAxis_ = GizmoAxis::None;
    bool hoverCenter = (abs(mousePos.x - origin.x) < 10.0f && abs(mousePos.y - origin.y) < 10.0f);
    bool hoverX = (mousePos.x >= origin.x && mousePos.x <= xTip.x && abs(mousePos.y - origin.y) < 8.0f);
    bool hoverY = (mousePos.y <= origin.y && mousePos.y >= yTip.y && abs(mousePos.x - origin.x) < 8.0f);

    if (hoverCenter) {
        colCenter = IM_COL32(255, 255, 0, 255);
        hoveredAxis_ = GizmoAxis::Center;
    }
    else if (hoverX) {
        colX = IM_COL32(255, 100, 100, 255);
        hoveredAxis_ = GizmoAxis::X;
    }
    else if (hoverY) {
        colY = IM_COL32(100, 255, 100, 255);
        hoveredAxis_ = GizmoAxis::Y;
    }

    // Drawing Lines
    drawList->AddLine(origin, xTip, colX, axisThickness_);
    drawList->AddLine(origin, yTip, colY, axisThickness_);

    // Drawing Box Tips (Instead of Triangles)
    float box = handleSize_;
    drawList->AddRectFilled(ImVec2(xTip.x - box / 2, xTip.y - box / 2), ImVec2(xTip.x + box, xTip.y + box), colX);
    drawList->AddRectFilled(ImVec2(yTip.x - box / 2, yTip.y - box / 2), ImVec2(yTip.x + box / 2, yTip.y - box), colY); // Shift up

    // Center Box
    drawList->AddRectFilled(
        ImVec2(origin.x - centerCubeHalfsize_, origin.y - centerCubeHalfsize_),
        ImVec2(origin.x + centerCubeHalfsize_, origin.y + centerCubeHalfsize_), colCenter);
}

void GizmoSystem::DrawRotateGizmo(ImDrawList* drawList, ImVec2 origin, Entity entity) {
    ImVec2 mousePos = ImGui::GetMousePos();
    float radius = axisLength_ * 0.8f;

    ImU32 color = IM_COL32(50, 100, 200, 255); // Blue ring

    // Calculate distance from center
    float dx = mousePos.x - origin.x;
    float dy = mousePos.y - origin.y;
    float dist = sqrt(dx * dx + dy * dy);

    // Hover if mouse is near the ring's edge
    bool hoverRing = (dist > radius - 10.0f && dist < radius + 10.0f);
    hoveredAxis_ = GizmoAxis::None;

    if (hoverRing) {
        color = IM_COL32(100, 150, 255, 255); // Bright Blue
        hoveredAxis_ = GizmoAxis::RotateRing;
    }

    // Draw Ring
    drawList->AddCircle(origin, radius, color, 64, axisThickness_);

     // Draw a line indicating current rotation
    EntityManager entityMgr(GameManager::GetInstance().GetECS());
    if (entityMgr.HasComponent(entity, TransformType)) {
        //float currentRot = entityMgr.GetComponent<Rotation>(entity).angle;
        float currentRot = entityMgr.GetComponent<Transform>(entity).rotation.textureAngle;
        float adjustedRot = currentRot + (float)(M_PI/2.0f);

        // Calculate coordinates and invert the Y component sign for ImGui screen space.
        ImVec2 rotTip = ImVec2(
            origin.x + cos(adjustedRot) * radius,  // X component
            origin.y - sin(adjustedRot) * radius   // Y component (Inverted for ImGui: +Y is Down)
        );

        drawList->AddLine(origin, rotTip, IM_COL32(255, 255, 255, 150), 2.0f);
    }
}

// --- Gizmos Dragging & Transformation --- 
void GizmoSystem::Update(int selectedEntityIndex, Entity activeCamera) {
    if (selectedEntityIndex <= 1 || !IsGizmosToggled()) {
        isDragging_ = false;
        return;
    }

    // Convert index to Entity ID
    Entity entity = selectedEntityIndex - 1;
    ImVec2 currentMousePos = ImGui::GetMousePos();

    // --- State: Stop Drag ---
    if (isDragging_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        isDragging_ = false;
        activeAxis_ = GizmoAxis::None;
        HandleStopDrag();
    }

    // --- State: Start Drag ---
    // If NOT dragging, check if the mouse is hovering over an axis AND clicked.
    if (!isDragging_ && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && hoveredAxis_ != GizmoAxis::None) {
        isDragging_ = true;
        activeAxis_ = hoveredAxis_;
        draggedEntity_ = entity;
        lastMousePos_ = currentMousePos;
        HandleStartDrag();
    }

    // --- State: Dragging ---
    if (isDragging_) {
        // Calculate how much the mouse has moved since the last frame (in screen pixels)
        ImVec2 mouseDelta = ImVec2(
            currentMousePos.x - lastMousePos_.x,
            currentMousePos.y - lastMousePos_.y
        );

        // Apply the transformation based on the active mode and axis
        ApplyTransformation(mouseDelta, activeCamera);

        // IMPORTANT: Update the last position for the next frame
        lastMousePos_ = currentMousePos;
    }
}

void GizmoSystem::HandleStopDrag() {
    // save the current state of the relevant components AFTER transformation
    EntityStorage& ecs_ = GameManager::GetInstance().GetECS();
    EntityManager entityMgr(ecs_);
    HistoryManager* historyMgr = editor_->GetHistoryMgr();

    if (gizmoAffectedComponents_.empty()) return;
    
    afterGizmosJSON_.clear();
    afterGizmosJSON_.reserve(gizmoAffectedComponents_.size());

    for (ComponentType& compType : gizmoAffectedComponents_) {
        if (entityMgr.HasComponent(draggedEntity_, compType)) {
            afterGizmosJSON_.push_back(EntitySerializer::SerializeComponentToString(draggedEntity_, compType, ecs_));
        }
        else {
            afterGizmosJSON_.push_back("");
        }
    }

    std::size_t changedCount = 0;
    for (std::size_t i = 0; i < gizmoAffectedComponents_.size(); ++i) {
        if (beforeGizmosJSON_[i] != afterGizmosJSON_[i]) ++changedCount;
    }
    if (changedCount == 0) return; // No changes detected

    if (changedCount > 1) {
        historyMgr->BeginGroup();

        for (std::size_t i = 0; i < gizmoAffectedComponents_.size(); ++i) {
            if (beforeGizmosJSON_[i] == afterGizmosJSON_[i]) continue;

            ComponentType compType = gizmoAffectedComponents_[i];

            std::stringstream ss;
            ss << "Modified " << ComponentToString(compType) << " in " << entityMgr.GetProperties(draggedEntity_).name << std::endl;
            historyMgr->RecordModifyComponent(draggedEntity_, ss.str(), beforeGizmosJSON_[i], afterGizmosJSON_[i], compType);
        }
        
        historyMgr->EndGroup();
    }
    else {
        for (std::size_t i = 0; i < gizmoAffectedComponents_.size(); ++i) {
            if (beforeGizmosJSON_[i] == afterGizmosJSON_[i]) continue;

            ComponentType compType = gizmoAffectedComponents_[i];

            std::stringstream ss;
            ss << "Modified " << ComponentToString(compType) << " in " << entityMgr.GetProperties(draggedEntity_).name << std::endl;
            historyMgr->RecordModifyComponent(draggedEntity_, ss.str(), beforeGizmosJSON_[i], afterGizmosJSON_[i], compType);
            break;
        }
    }
}

void GizmoSystem::HandleStartDrag() {
    EntityStorage& ecs_ = GameManager::GetInstance().GetECS();
    EntityManager entityMgr(ecs_);

    gizmoAffectedComponents_.clear();
    beforeGizmosJSON_.clear();
    afterGizmosJSON_.clear();

    auto record = [&](ComponentType compType, bool allowEmpty) {
        if (!entityMgr.HasComponent(draggedEntity_, compType)) {
            if (!allowEmpty) return;

            gizmoAffectedComponents_.push_back(compType);
            beforeGizmosJSON_.push_back("");
            return;
        }

        gizmoAffectedComponents_.push_back(compType);
        beforeGizmosJSON_.push_back(EntitySerializer::SerializeComponentToString(draggedEntity_, compType, ecs_));
    };

    // save the current state of the relevant component before applying any transformations
    if (currentMode_ == GizmoMode::Translate) {
        record(TransformType, true);
        record(TrapPlatformComponentType, false);
    }
    else if (currentMode_ == GizmoMode::Scale) {
        record(TransformType, true);
    }
    else if (currentMode_ == GizmoMode::Rotate) {
        record(TransformType, true);
    }
}

void GizmoSystem::ApplyTransformation(const ImVec2 mouseDelta, const Entity activeCamera) {
    // Safety check: isDragging_ must be true, and draggedEntity_ must be valid
    if (!isDragging_ || draggedEntity_ == 0) return;

    EntityManager entityMgr(GameManager::GetInstance().GetECS());

    switch (currentMode_) {
    case GizmoMode::Translate:
    {
        bool isGUI = entityMgr.HasComponent(draggedEntity_, GUITagType);
        Vector2D currentMousePos = editor_->GetMouseWorldPosition();

        // If draggedEntity does not have Transform component, add an empty one.
        if (!entityMgr.HasComponent(draggedEntity_, TransformType)) {
            entityMgr.AddComponent(draggedEntity_, TransformType);
        }

        // --- Determine new position based on active axis ---
        if (isGUI) {
            currentMousePos = editor_->GetNormalizedMousePos();
        }

        Vector2D snappedPos = editor_->SnapToGrid(currentMousePos, isGUI);

        // If dragging only on one axis (X or Y), we need to preserve the other axis's current position.
        // We use the *snapped* position for the dragged axis, and the *current* position for the fixed axis.
        if (activeAxis_ == GizmoAxis::X || activeAxis_ == GizmoAxis::Y) {
            // Get the entity's CURRENT position to preserve the fixed axis
            Vector2D currentPos;
            if (entityMgr.HasComponent(draggedEntity_, TransformType)) {
                currentPos = entityMgr.GetComponent<Transform>(draggedEntity_).position;
            }
            else return;

            if (activeAxis_ == GizmoAxis::X) {
                snappedPos.y = currentPos.y; // Preserve current Y
            }
            else {
                snappedPos.x = currentPos.x; // Preserve current X
            }
        }

        // --- Apply the Transformation to all relevant components ---
        if (entityMgr.HasComponent(draggedEntity_, TransformType)) {
            entityMgr.GetComponent<Transform>(draggedEntity_).position = snappedPos;
        }
        if (entityMgr.HasComponent(draggedEntity_, TrapPlatformComponentType)) {
            entityMgr.GetComponent<TrapPlatformComponent>(draggedEntity_).originalPosition = snappedPos;
        }
        break;
    }

    case GizmoMode::Rotate:
    {
        if (activeAxis_ != GizmoAxis::RotateRing) return;

        // If draggedEntity does not have Transform component, add an empty one.
        if (!entityMgr.HasComponent(draggedEntity_, TransformType)) {
            entityMgr.AddComponent(draggedEntity_, TransformType);
        }

        if (entityMgr.HasComponent(draggedEntity_, TransformType)) {
            Transform::Rotation& rotation = entityMgr.GetComponent<Transform>(draggedEntity_).rotation;
            
            // Get position
            Vector2D entityWorldPos;
            if (entityMgr.HasComponent(draggedEntity_, TransformType)) {
                entityWorldPos = entityMgr.GetComponent<Transform>(draggedEntity_).position;
            }
            else return;

            // 1. Get Screen Center of the Entity
            ImVec2 screenCenter = editor_->WorldToImGuiScreen(entityWorldPos, activeCamera);

            // 2. Vector from Center to Last Mouse Pos (v1)
            // NOTE: The Y component is inverted for screen-to-angle calculation
            ImVec2 v1 = ImVec2(lastMousePos_.x - screenCenter.x, screenCenter.y - lastMousePos_.y);

            // 3. Vector from Center to Current Mouse Pos (v2)
            ImVec2 currentMousePos = ImGui::GetMousePos();
            ImVec2 v2 = ImVec2(currentMousePos.x - screenCenter.x, screenCenter.y - currentMousePos.y);

            // 4. Calculate the angle between the two vectors (atan2(y, x) is preferred for full circle)
            float angle1 = atan2(v1.y, v1.x);
            float angle2 = atan2(v2.y, v2.x);

            float deltaAngle = angle2 - angle1;

            // 5. Apply the rotation
            rotation.angle += deltaAngle;
            rotation.textureAngle += deltaAngle;
            rotation.colliderAngle += deltaAngle;
        }
        break;
    }

    case GizmoMode::Scale:
    {
        // If draggedEntity does not have Transform component, add an empty one.
        if (!entityMgr.HasComponent(draggedEntity_, TransformType)) {
            entityMgr.AddComponent(draggedEntity_, TransformType);
        }

        if (entityMgr.HasComponent(draggedEntity_, TransformType)) {
            Vector2D& scale = entityMgr.GetComponent<Transform>(draggedEntity_).scale;

            // Get camera zoom for screen-to-world conversion
            float cameraZoom = GameManager::GetInstance().GetSystems().camera_->GetCamera(activeCamera).size.x / editor_->GetGameViewportSize().x;

            // Convert screen delta to world scale change
            float deltaWorldX = mouseDelta.x * cameraZoom * 0.01f; // Multiply by a small factor (e.g., 0.01) to slow down scale
            float deltaWorldY = -mouseDelta.y * cameraZoom * 0.01f; // Y inverted

            if (activeAxis_ == GizmoAxis::X) {
                scale.x += deltaWorldX;
            }
            else if (activeAxis_ == GizmoAxis::Y) {
                scale.y += deltaWorldY;
            }
            else if (activeAxis_ == GizmoAxis::Center) {
                // Apply uniform scaling based on the larger delta (for proportional scaling)
                float deltaUniform = (abs(deltaWorldX) > abs(deltaWorldY)) ? deltaWorldX : deltaWorldY;

                scale.x += deltaUniform;
                scale.y += deltaUniform;
            }

            // Safety: Clamp scale factors to prevent negative sizes
            scale.x = std::max(0.1f, scale.x);
            scale.y = std::max(0.1f, scale.y);
        }

        break;
    }

    case GizmoMode::None:
    default:
        break;
    }
}