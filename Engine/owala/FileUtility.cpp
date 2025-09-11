/*****************************************************************//**
 * \file   FileUtility.cpp
 * \brief
 *
 *
 * \author Yar Xuan
 * \date   11 September 2025
 *********************************************************************/

#include <iostream>
#include <fstream>
#include <sstream>
#include "rapidjson/document.h"

#include "FileUtility.h"
#include "Vector2D.h"

using namespace std;
using namespace rapidjson;

namespace Framework {
	Document readFile(const string& fileName) {
		// read positions json file into json_str as a string
		ifstream file(fileName);
		ostringstream tmp;
		tmp << file.rdbuf();
		string json = tmp.str();
		const char* json_str = json.c_str();

		// create Document object to parse json string
		Document d;
		d.Parse(json_str);
		
		return d;
	}

	Vector2D FileIOSystem::loadWindowConfig() {
		Document d = readFile("../Data/config.json");
		const Value& config = d["windowConfig"];

		return Vector2D { 
			static_cast<float>(config["width"].GetInt()),
			static_cast<float>(config["height"].GetInt())
		};
	}
}
