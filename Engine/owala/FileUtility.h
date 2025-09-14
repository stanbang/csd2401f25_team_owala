/*****************************************************************//**
 * \file   FileUtility.h
 * \brief
 *
 *
 * \author Yar Xuan
 * \date   11 September 2025
 *********************************************************************/
#pragma once
#include "rapidjson/document.h"
#include "Vector2D.h"
#include "Components.h"
#include "ISystem.h"

namespace Framework {
	using namespace std;
	using namespace rapidjson;

	Document readFile(const string& fileName);
	
	class FileIOSystem {
	public:
		Vector2D loadWindowConfig();
		PlayerStats loadPlayerStats();
		bool writePlayerStats();

	private:
	};
} // namespace Framework
