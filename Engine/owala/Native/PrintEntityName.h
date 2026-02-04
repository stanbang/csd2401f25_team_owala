/*****************************************************************//**
 * \file	PrintEntityName.h
 *
 * \primary_author
 *      Jason Lou
 * \secondary_author

 *
 * \brief	Contains definitions for the PrintEntityName script
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once

#include "ScriptableEntity.h"

using namespace Framework;

class PrintEntityName : public ScriptableEntity {
protected:
	void OnCreate() override;
	void OnUpdate() override;
	void OnDestroy() override;
};
