#include "AppDifferenceTracker.h"

namespace SystemInfoAgent::ApplicationReport
{
	std::vector<AppDiffEntry> AppDiffTracker::diff(const std::unordered_map<std::wstring, AppInfo>& previous,
												   const std::unordered_map<std::wstring, AppInfo>& current)
	{
		std::vector<AppDiffEntry> entries;

		for (const auto& [name, AppInfo] : current)
		{
			if (previous.find(name) == previous.end())
			{
				entries.emplace_back(AppInfo, AppChangeStatus::Added);
			}
			else 
			{
				entries.emplace_back(AppInfo, AppChangeStatus::Unchanged);
			}
		}

		for (const auto& [name, AppInfo] : previous)
		{
			if (current.find(name) == current.end())
			{
				entries.emplace_back(AppInfo, AppChangeStatus::Removed);
			}
		}

		return entries;
	}	// std::vector<AppDiffEntry> AppDiffTracker::diff

	bool AppDiffTracker::hasChanges(const std::vector<AppDiffEntry>& entries)
	{
		for (const auto& entry : entries)
		{
			if (entry.status != AppChangeStatus::Unchanged)
				return true;
		}

		return false;
	}
}	// namespace SystemInfoAgent::ApplicationReport 