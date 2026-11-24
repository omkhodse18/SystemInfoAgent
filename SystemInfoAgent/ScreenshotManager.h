#pragma once
#include <string>

namespace SystemInfoAgent::Screenshot
{
	class ScreenshotManager
	{
	public:
		bool capture(const std::wstring& outputPath);
	};
}	// SystemInfoAgent::Screenshot