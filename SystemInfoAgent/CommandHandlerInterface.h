#pragma once

#include "Command.h"

#include "ScreenshotManager.h"

namespace SystemInfoAgent::cli
{
	using SystemInfoAgent::Screenshot::ScreenshotManager;

	class ICommandHandler
	{
	public:
		~ICommandHandler() = default;
		virtual int run(const Command& command) = 0;
	};

	class ScreenshotHandler : public ICommandHandler
	{
	public:
		
		//bool ScreenshotManager::capture(const std::wstring& outputPath)
		int run(const Command& command) override
		{
			ScreenshotManager objScreenshotManager;
			bool isCaptured = objScreenshotManager.capture(L"C:\\SystemInfoAgent\\");
			return isCaptured;
		}
	};

}