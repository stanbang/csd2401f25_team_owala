/*****************************************************************//**
 * \file   RestartButtonScript.h
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author
 *
 *
 * \brief	Header file for restart button logic to switch to previous scene.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once

#include "ScriptableEntity.h"

using namespace Framework;

class RestartButtonScript : public ScriptableEntity {
protected:
	void OnCreate() override;
	void OnUpdate() override;
	void OnDestroy() override;
private:
	Entity self_;
};
