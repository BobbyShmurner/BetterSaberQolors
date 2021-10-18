#include <string>
#include <sstream>
#include <algorithm>

#include "UnityEngine/Color.hpp"
#include "UnityEngine/Color32.hpp"

class ColorUtility
{
	public:

	static UnityEngine::Color ParseHtmlString(std::string htmlStr) {
		if (static_cast<int>(htmlStr.length()) != 6) return {0, 0, 0, 0};

		unsigned int r, g, b;   

		std::stringstream ss;

		ss << std::hex << htmlStr.substr(0, 2);
		ss >> r;

		ss.clear();
		ss << std::hex << htmlStr.substr(2, 2);
		ss >> g;

		ss.clear();
		ss << std::hex << htmlStr.substr(4, 2);
		ss >> b;

		return {static_cast<float>(r / 255.0f), static_cast<float>(g / 255.0f), static_cast<float>(b / 255.0f), 1};
	}

	static std::string ToHtmlStringRGB(UnityEngine::Color color)
	{
		UnityEngine::Color32 col32 = UnityEngine::Color32(
			(uint8_t)std::clamp((int)(color.r * 255), 0, 255),
			(uint8_t)std::clamp((int)(color.g * 255), 0, 255),
			(uint8_t)std::clamp((int)(color.b * 255), 0, 255),
			1);

		char buf[7];
		sprintf(&buf[0],"%02x", col32.r);
		sprintf(&buf[2],"%02x", col32.g);
		sprintf(&buf[4],"%02x", col32.b);

		return std::string(buf);
	}
};