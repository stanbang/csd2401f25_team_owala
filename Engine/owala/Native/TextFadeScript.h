/*****************************************************************//**
 * \file   TextFadeScript.h
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author
 *
 *
 * \brief	Header file to implement a fade in-out effect for world text
 *			depending on distance between player and text.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once

#include "ScriptableEntity.h"

using namespace Framework;

class TextFadeScript : public ScriptableEntity {
private:
	const float MAX_DISTANCE = 500.0f;
protected:
	void OnCreate() override;
	void OnUpdate() override;
	void OnDestroy() override;
};
