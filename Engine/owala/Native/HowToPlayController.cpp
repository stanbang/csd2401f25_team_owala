/*****************************************************************//**
 * \file   HowToPlayController.cpp
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author
 *
 *
 * \brief	Source File for How To Play scene controller.
 *          Toggles entities active boolean to show/hide entities 
 *          according to the current page.
 * 
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "HowToPlayController.h"
#include "ScriptRegistry.h"
#include "GameManager.h"

REGISTER_SCRIPT(HowToPlayController, "HowToPlayController")

void HowToPlayController::OnCreate() {
    EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);

    // cache page entities
    pageCache_.resize(maxPages); 
    for (size_t pg = 0; pg < maxPages; ++pg) {
        std::string pageString = "Page" + std::to_string(pg+1) + '_';
        pageCache_[pg] = entityMgr.GetEntitiesBySubstring(pageString);
    }

    // cache button entities
    prevBtn = entityMgr.GetEntityByName("PrevButton");
    nextBtn = entityMgr.GetEntityByName("NextButton");

    // subscribe to prev/next button clicks
    MessageBus::Instance().Subscribe<ButtonClickedEvent>([this](const ButtonClickedEvent& event) {
        if (event.action == ButtonAction::PREV_PAGE && currentPage > 0) {
            currentPage--;
        }

        if (event.action == ButtonAction::NEXT_PAGE && currentPage < (maxPages - 1)) {
            currentPage++;
        }
    });
}

void HowToPlayController::OnUpdate() {
    EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);

    // toggle active property according to page number
    for (size_t pg = 0; pg < maxPages; ++pg) {
        bool isCurrentPg = (pg == currentPage);

        for (Entity en : pageCache_[pg]) {
            // set current page entities to active
            if (isCurrentPg && !entityMgr.IsActive(en)) {
                entityMgr.SetActive(en, true);
            }
            // set active entities that are NOT current page to inactive
            if (!isCurrentPg && entityMgr.IsActive(en)) {
                entityMgr.SetActive(en, false);
            }

        }
    }

    // handle prev/next buttons
    if (currentPage == 0) {
        entityMgr.SetActive(prevBtn, false);
        entityMgr.SetActive(nextBtn, true);
    }
    else if (currentPage > 0 && currentPage < (maxPages - 1)) {
        entityMgr.SetActive(prevBtn, true);
        entityMgr.SetActive(nextBtn, true);
    }
    else if (currentPage == (maxPages-1)) {
        entityMgr.SetActive(prevBtn, true);
        entityMgr.SetActive(nextBtn, false);
    }
}

void HowToPlayController::OnDestroy() {
	// Logic to be called before Entity is deleted
}
