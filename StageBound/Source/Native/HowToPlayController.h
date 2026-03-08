/*****************************************************************//**
 * \file   HowToPlayController.h
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author
 *
 *
 * \brief	Header File for How To Play scene controller.
 *          Toggles entities active boolean to show/hide entities
 *          according to the current page.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once

#include "ScriptableEntity.h"

using namespace Framework;

class HowToPlayController : public ScriptableEntity {
protected:
	void OnCreate() override;
	void OnUpdate() override;
	void OnDestroy() override;

private:
	int currentPage = 0;
	int maxPages = 3;
	std::vector<std::vector<Entity>> pageCache_; // cache entities for each page
	Entity prevBtn, nextBtn; // cache button entities
};
