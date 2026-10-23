#pragma once

// Launch arguments passed by the Apex launcher, and the hand-off back to it.
// Behaviour matches the Unity SDK 2.0 (ApexSystem and PixoPlatformUtilities)
// so launchers keep working unchanged.

#include "JsonHelpers.hpp"

#include <map>
#include <string>

namespace apex
{

using LaunchArguments = std::map<std::string, std::string>;

namespace launch_keys
{
constexpr const char* kToken = "pixotoken";
constexpr const char* kOptional = "optional";
constexpr const char* kReturnTarget = "returntarget";
constexpr const char* kTargetType = "targettype";
}  // namespace launch_keys

// Decodes %XX escapes. '+' is left as is, so tokens are never altered.
std::string UrlUnescape(const std::string& escapedText);

// One argument after the executable is launcher URL data; otherwise
// "-key value" pairs (leading dashes are removed from the key).
void ParseCommandLine(int argumentCount, const char* const* argumentValues, LaunchArguments& launchArguments);

// Reads "key=value&..." after the first '?'. The "optional" value may be a JSON
// object containing '&' and '=', so it is cut at its last '}'.
void ParseUrlArguments(const std::string& launchUrl, LaunchArguments& launchArguments);

// What the engine does to return to the launcher (see ApexLaunchExitTarget).
// userToken is the current user's token, or empty to fall back to the passed one.
Json BuildExitTarget(const LaunchArguments& launchArguments, const std::string& userToken,
					 const std::string& nextTarget);

bool IsModuleVersionValid(const std::string& moduleVersion);

}  // namespace apex
