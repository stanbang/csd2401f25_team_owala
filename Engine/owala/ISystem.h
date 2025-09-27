/*****************************************************************//**
 * \file   ISystem.h
 *
 * \primary_author
 *      Ng Jordain Ying Zhe
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
namespace Framework {

	struct ISystem {
		virtual ~ISystem() = default;
		virtual void Initialize() = 0;   // no Core& param
		virtual void Update(float dt) = 0;
		virtual void Shutdown() = 0;
	};

} // namespace Framework
