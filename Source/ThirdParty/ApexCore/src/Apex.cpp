// The C interface. Every entry point catches C++ exceptions so none crosses
// into engine code.

#include "apex/Apex.h"

#include "Client.hpp"
#include "LaunchArguments.hpp"

#include <cstddef>
#include <cstring>
#include <new>

// The opaque handle engines hold. Raw ApexClient pointers cross the C boundary;
// inside, the client is owned by a std::unique_ptr.
struct ApexClient
{
public:
	explicit ApexClient(std::unique_ptr<apex::Client> apexImplementation)
		: m_apexImplementation(std::move(apexImplementation))
	{
	}

	apex::Client& GetImplementation() const
	{
		return *m_apexImplementation;
	}

private:
	std::unique_ptr<apex::Client> m_apexImplementation;
};

namespace
{

constexpr const char* kLibraryVersion = "0.2.0";
constexpr int kVectorComponentCount = 3;

// The client behind a handle, or nullptr for a null handle.
apex::Client* FindImplementation(const ApexClient* client)
{
	return client != nullptr ? &client->GetImplementation() : nullptr;
}

// Runs callBody, turning an exception into an immediate failed result.
template <typename CallBody>
ApexCallId RunGuardedCall(ApexClient* client, ApexResultCallback onResult, void* userData, CallBody callBody)
{
	apex::Client* implementation = FindImplementation(client);
	if (implementation == nullptr)
	{
		return 0;
	}

	try
	{
		return callBody(*implementation);
	}
	catch (const std::bad_alloc&)
	{
		try
		{
			return implementation->FailImmediately(APEX_ERR_INTERNAL, "Out of memory", onResult, userData);
		}
		catch (...)
		{
			return 0;
		}
	}
	catch (const std::exception& exception)
	{
		try
		{
			return implementation->FailImmediately(APEX_ERR_INTERNAL, exception.what(), onResult, userData);
		}
		catch (...)
		{
			return 0;
		}
	}
	catch (...)
	{
		return 0;
	}
}

// Runs actionBody on the client, swallowing any exception.
template <typename ActionBody>
void RunGuardedAction(ApexClient* client, ActionBody actionBody)
{
	apex::Client* implementation = FindImplementation(client);
	if (implementation == nullptr)
	{
		return;
	}

	try
	{
		actionBody(*implementation);
	}
	catch (...)
	{
	}
}

// Copies the text produced by textSource into the caller's buffer; on any
// failure the buffer receives an empty string.
template <typename TextSource>
size_t CopyGuardedText(const ApexClient* client, char* buffer, size_t bufferSize, TextSource textSource)
{
	const apex::Client* implementation = FindImplementation(client);
	if (implementation == nullptr)
	{
		return apex::CopyToCallerBuffer(std::string(), buffer, bufferSize);
	}

	try
	{
		return apex::CopyToCallerBuffer(textSource(*implementation), buffer, bufferSize);
	}
	catch (...)
	{
		return apex::CopyToCallerBuffer(std::string(), buffer, bufferSize);
	}
}

std::string CopyModuleData(const void* moduleData, size_t moduleDataLength)
{
	return moduleData != nullptr && moduleDataLength > 0
			   ? std::string(static_cast<const char*>(moduleData), moduleDataLength)
			   : std::string();
}

std::string TextOrEmpty(const char* text)
{
	return text != nullptr ? std::string(text) : std::string();
}

bool IsSetText(const char* text)
{
	return text != nullptr && text[0] != '\0';
}

apex::Pose ConvertPose(const ApexPose* enginePose)
{
	apex::Pose convertedPose;
	if (enginePose != nullptr)
	{
		apex::Vector3 position = {};
		apex::Vector3 rotation = {};
		for (int component = 0; component < kVectorComponentCount; ++component)
		{
			position[component] = enginePose->Position[component];
			rotation[component] = enginePose->Rotation[component];
		}

		convertedPose.SetPosition(position);
		convertedPose.SetRotation(rotation);
	}

	return convertedPose;
}

// Telemetry calls have no result; a missing tracked ID is ignored by the callers.
template <typename TelemetryBody>
void RunTelemetryCall(ApexClient* client, TelemetryBody telemetryBody)
{
	RunGuardedAction(client, [&](apex::Client& implementation)
	{
		telemetryBody(implementation.GetTelemetryBuffer());
	});
}

}  // namespace

extern "C"
{
APEX_API uint32_t APEX_CALL ApexAbiVersion(void)
{
	return (static_cast<uint32_t>(APEX_ABI_MAJOR) << 16) | APEX_ABI_MINOR;
}

APEX_API const char* APEX_CALL ApexVersionString(void)
{
	return kLibraryVersion;
}

APEX_API const char* APEX_CALL ApexBuiltinTransportName(void)
{
	return apex::BuiltinTransportName();
}

APEX_API void APEX_CALL ApexConfigInit(ApexConfig* config)
{
	if (config == nullptr)
	{
		return;
	}

	*config = ApexConfig{};
	config->StructSize = sizeof(ApexConfig);
	config->Environment = APEX_ENV_NA_PRODUCTION;
	config->HeartbeatIntervalSeconds = 5.0;
	config->RequestTimeoutMilliseconds = 30000;
	config->LogLevel = APEX_LOG_INFO;
	config->TelemetrySink = APEX_TELEMETRY_OFF;
	config->TelemetryFlushIntervalSeconds = 10.0;
	config->TelemetryPoseSamplesPerSecond = 2.0;
	config->TelemetryMaximumRecordsPerPacket = 500;
	config->SessionFormat = APEX_SESSION_FORMAT_DATA;
}

APEX_API ApexClient* APEX_CALL ApexClientCreate(const ApexConfig* config)
{
	// ABI 0.1 configs end where the telemetry fields begin.
	if (config == nullptr || config->StructSize < offsetof(ApexConfig, TelemetrySink))
	{
		return nullptr;
	}

	try
	{
		// Fields an older caller does not know about keep their defaults.
		ApexConfig completeConfig;
		ApexConfigInit(&completeConfig);
		const size_t knownSize = config->StructSize < sizeof(ApexConfig) ? config->StructSize : sizeof(ApexConfig);
		std::memcpy(&completeConfig, config, knownSize);
		completeConfig.StructSize = sizeof(ApexConfig);

		std::string errorMessage;
		std::unique_ptr<apex::Client> implementation = apex::Client::Create(completeConfig, errorMessage);
		if (!implementation)
		{
			if (completeConfig.LogCallback != nullptr)
			{
				completeConfig.LogCallback(completeConfig.LogUserData, APEX_LOG_ERROR, errorMessage.c_str());
			}

			return nullptr;
		}

		// Ownership passes to the engine, which returns it through ApexClientDestroy.
		return std::make_unique<ApexClient>(std::move(implementation)).release();
	}
	catch (...)
	{
		return nullptr;
	}
}

APEX_API void APEX_CALL ApexClientDestroy(ApexClient* client)
{
	if (client == nullptr)
	{
		return;
	}

	// Retakes the ownership ApexClientCreate handed out.
	std::unique_ptr<ApexClient> ownedClient(client);
	try
	{
		if (!ownedClient->GetImplementation().RequestDestroy())
		{
			(void)ownedClient.release();  // inside Poll: deleted when Poll returns
		}
	}
	catch (...)
	{
	}
}

APEX_API void APEX_CALL ApexClientPoll(ApexClient* client, double nowSeconds)
{
	if (client == nullptr)
	{
		return;
	}

	RunGuardedAction(client, [&](apex::Client& implementation)
	{
		implementation.Poll(nowSeconds);
	});
	if (client->GetImplementation().IsDestroyPending())
	{
		// ApexClientDestroy was called from a callback during this Poll.
		std::unique_ptr<ApexClient> ownedClient(client);
	}
}

APEX_API void APEX_CALL ApexCancel(ApexClient* client, ApexCallId callId)
{
	RunGuardedAction(client, [&](apex::Client& implementation)
	{
		implementation.Cancel(callId);
	});
}

APEX_API void APEX_CALL ApexHttpComplete(ApexClient* client, uint64_t requestId, int32_t httpStatus,
										 const char* responseBody, size_t responseBodyLength,
										 const char* transportError)
{
	RunGuardedAction(client, [&](apex::Client& implementation)
	{
		apex::HttpResponse response;
		if (IsSetText(transportError))
		{
			response.SetTransportError(transportError);
		}
		else
		{
			response.SetStatusCode(httpStatus);
			if (responseBody != nullptr && responseBodyLength > 0)
			{
				response.SetBody(std::string(responseBody, responseBodyLength));
			}
		}

		implementation.CompleteHttpRequest(requestId, std::move(response));
	});
}

APEX_API ApexCallId APEX_CALL ApexPing(ApexClient* client, ApexResultCallback onResult, void* userData)
{
	return RunGuardedCall(client, onResult, userData, [&](apex::Client& implementation)
	{
		return implementation.Ping(onResult, userData);
	});
}

APEX_API ApexCallId APEX_CALL ApexLogin(ApexClient* client, const char* username, const char* password,
										ApexResultCallback onResult, void* userData)
{
	return RunGuardedCall(client, onResult, userData, [&](apex::Client& implementation)
	{
		return implementation.Login(username, password, onResult, userData);
	});
}

APEX_API ApexCallId APEX_CALL ApexLoginWithToken(ApexClient* client, const char* userToken, ApexResultCallback onResult,
												 void* userData)
{
	return RunGuardedCall(client, onResult, userData, [&](apex::Client& implementation)
	{
		return implementation.LoginWithToken(userToken, onResult, userData);
	});
}

APEX_API ApexCallId APEX_CALL ApexQuickIdGetUsers(ApexClient* client, const char* serialNumber,
												  ApexResultCallback onResult, void* userData)
{
	return RunGuardedCall(client, onResult, userData, [&](apex::Client& implementation)
	{
		return implementation.QuickIdGetUsers(serialNumber, onResult, userData);
	});
}

APEX_API ApexCallId APEX_CALL ApexQuickIdLogin(ApexClient* client, const char* serialNumber, const char* username,
											   ApexResultCallback onResult, void* userData)
{
	return RunGuardedCall(client, onResult, userData, [&](apex::Client& implementation)
	{
		return implementation.QuickIdLogin(serialNumber, username, onResult, userData);
	});
}

APEX_API ApexCallId APEX_CALL ApexGenerateLoginCode(ApexClient* client, int32_t userId, ApexResultCallback onResult,
													void* userData)
{
	return RunGuardedCall(client, onResult, userData, [&](apex::Client& implementation)
	{
		return implementation.GenerateLoginCode(userId, onResult, userData);
	});
}

APEX_API void APEX_CALL ApexLogout(ApexClient* client)
{
	RunGuardedAction(client, [](apex::Client& implementation)
	{
		implementation.Logout();
	});
}

APEX_API ApexCallId APEX_CALL ApexGetUserModules(ApexClient* client, int32_t userId, ApexResultCallback onResult,
												 void* userData)
{
	return RunGuardedCall(client, onResult, userData, [&](apex::Client& implementation)
	{
		return implementation.GetUserModules(userId, onResult, userData);
	});
}

APEX_API ApexCallId APEX_CALL ApexCheckModuleAccess(ApexClient* client, int32_t moduleId, ApexResultCallback onResult,
													void* userData)
{
	return RunGuardedCall(client, onResult, userData, [&](apex::Client& implementation)
	{
		return implementation.CheckModuleAccess(moduleId, onResult, userData);
	});
}

APEX_API ApexCallId APEX_CALL ApexGetModuleList(ApexClient* client, const char* platformName,
												ApexResultCallback onResult, void* userData)
{
	return RunGuardedCall(client, onResult, userData, [&](apex::Client& implementation)
	{
		return implementation.GetModuleList(platformName, onResult, userData);
	});
}

APEX_API ApexCallId APEX_CALL ApexGetModulePage(ApexClient* client, const char* platformName, uint32_t lifecycleMask,
												int32_t sortOrder, int32_t pageIndex, int32_t pageSize, int32_t refresh,
												ApexResultCallback onResult, void* userData)
{
	return RunGuardedCall(client, onResult, userData, [&](apex::Client& implementation)
	{
		return implementation.GetModulePage(platformName, lifecycleMask, sortOrder, pageIndex, pageSize, refresh != 0,
											onResult, userData);
	});
}

APEX_API ApexCallId APEX_CALL ApexSessionJoin(ApexClient* client, const void* moduleData, size_t moduleDataLength,
											  ApexResultCallback onResult, void* userData)
{
	return RunGuardedCall(client, onResult, userData, [&](apex::Client& implementation)
	{
		return implementation.JoinSession(CopyModuleData(moduleData, moduleDataLength), onResult, userData);
	});
}

APEX_API ApexCallId APEX_CALL ApexSessionEvent(ApexClient* client, const void* moduleData, size_t moduleDataLength,
											   ApexResultCallback onResult, void* userData)
{
	return RunGuardedCall(client, onResult, userData, [&](apex::Client& implementation)
	{
		return implementation.SendSessionEvent(CopyModuleData(moduleData, moduleDataLength), onResult, userData);
	});
}

APEX_API ApexCallId APEX_CALL ApexSessionComplete(ApexClient* client, const void* moduleData, size_t moduleDataLength,
												  ApexResultCallback onResult, void* userData)
{
	return RunGuardedCall(client, onResult, userData, [&](apex::Client& implementation)
	{
		return implementation.CompleteSession(CopyModuleData(moduleData, moduleDataLength), onResult, userData);
	});
}

APEX_API int32_t APEX_CALL ApexIsLoggedIn(const ApexClient* client)
{
	const apex::Client* implementation = FindImplementation(client);
	return implementation != nullptr && implementation->IsLoggedIn() ? 1 : 0;
}

APEX_API size_t APEX_CALL ApexGetCurrentUser(const ApexClient* client, char* buffer, size_t bufferSize)
{
	return CopyGuardedText(client, buffer, bufferSize, [](const apex::Client& implementation)
	{
		return implementation.GetCurrentUserJson();
	});
}

APEX_API int64_t APEX_CALL ApexGetSessionId(const ApexClient* client)
{
	const apex::Client* implementation = FindImplementation(client);
	return implementation != nullptr ? implementation->GetActiveSessionId() : 0;
}

APEX_API void APEX_CALL ApexSetDeviceId(ApexClient* client, const char* deviceId)
{
	RunGuardedAction(client, [&](apex::Client& implementation)
	{
		implementation.SetDeviceId(deviceId);
	});
}

APEX_API void APEX_CALL ApexSetSessionFormat(ApexClient* client, int32_t sessionFormat)
{
	RunGuardedAction(client, [&](apex::Client& implementation)
	{
		implementation.SetSessionFormat(sessionFormat);
	});
}

APEX_API void APEX_CALL ApexLaunchParseArgs(ApexClient* client, int32_t argumentCount,
											const char* const* argumentValues)
{
	RunGuardedAction(client, [&](apex::Client& implementation)
	{
		implementation.ParseLaunchCommandLine(argumentCount, argumentValues);
	});
}

APEX_API void APEX_CALL ApexLaunchParseUrl(ApexClient* client, const char* launchUrl)
{
	RunGuardedAction(client, [&](apex::Client& implementation)
	{
		implementation.ParseLaunchUrl(launchUrl);
	});
}

APEX_API void APEX_CALL ApexLaunchSet(ApexClient* client, const char* key, const char* value)
{
	RunGuardedAction(client, [&](apex::Client& implementation)
	{
		implementation.SetLaunchArgument(key, value);
	});
}

APEX_API size_t APEX_CALL ApexLaunchGet(const ApexClient* client, const char* key, char* buffer, size_t bufferSize)
{
	return CopyGuardedText(client, buffer, bufferSize, [&](const apex::Client& implementation)
	{
		return implementation.GetLaunchArgument(key);
	});
}

APEX_API size_t APEX_CALL ApexLaunchExitTarget(const ApexClient* client, const char* nextTarget, char* buffer,
											   size_t bufferSize)
{
	return CopyGuardedText(client, buffer, bufferSize, [&](const apex::Client& implementation)
	{
		return implementation.GetExitTargetJson(nextTarget);
	});
}

APEX_API int32_t APEX_CALL ApexModuleVersionIsValid(const char* moduleVersion)
{
	if (moduleVersion == nullptr)
	{
		return 0;
	}

	try
	{
		return apex::IsModuleVersionValid(moduleVersion) ? 1 : 0;
	}
	catch (...)
	{
		return 0;
	}
}

APEX_API void APEX_CALL ApexTelemetryObjectRegistered(ApexClient* client, const char* trackedId, const char* objectName,
													  const char* meshName, const ApexPose* pose)
{
	if (!IsSetText(trackedId))
	{
		return;
	}

	RunTelemetryCall(client, [&](apex::TelemetryBuffer& telemetryBuffer)
	{
		telemetryBuffer.ObjectRegistered(trackedId, TextOrEmpty(objectName), TextOrEmpty(meshName), ConvertPose(pose));
	});
}

APEX_API void APEX_CALL ApexTelemetryObjectUnregistered(ApexClient* client, const char* trackedId, const ApexPose* pose)
{
	if (!IsSetText(trackedId))
	{
		return;
	}

	RunTelemetryCall(client, [&](apex::TelemetryBuffer& telemetryBuffer)
	{
		telemetryBuffer.ObjectUnregistered(trackedId, ConvertPose(pose));
	});
}

APEX_API void APEX_CALL ApexTelemetryEngagementBegin(ApexClient* client, const char* trackedId,
													 const char* engagementName)
{
	if (!IsSetText(trackedId))
	{
		return;
	}

	RunTelemetryCall(client, [&](apex::TelemetryBuffer& telemetryBuffer)
	{
		telemetryBuffer.EngagementBegin(trackedId, TextOrEmpty(engagementName));
	});
}

APEX_API void APEX_CALL ApexTelemetryEngagementEnd(ApexClient* client, const char* trackedId,
												   const char* engagementName)
{
	if (!IsSetText(trackedId))
	{
		return;
	}

	RunTelemetryCall(client, [&](apex::TelemetryBuffer& telemetryBuffer)
	{
		telemetryBuffer.EngagementEnd(trackedId, TextOrEmpty(engagementName));
	});
}

APEX_API void APEX_CALL ApexTelemetryInteraction(ApexClient* client, const char* trackedId, const char* actionName)
{
	if (!IsSetText(trackedId))
	{
		return;
	}

	RunTelemetryCall(client, [&](apex::TelemetryBuffer& telemetryBuffer)
	{
		telemetryBuffer.Interaction(trackedId, TextOrEmpty(actionName));
	});
}

APEX_API void APEX_CALL ApexTelemetryStepBegin(ApexClient* client, const char* stepName)
{
	RunTelemetryCall(client, [&](apex::TelemetryBuffer& telemetryBuffer)
	{
		telemetryBuffer.StepBegin(TextOrEmpty(stepName));
	});
}

APEX_API void APEX_CALL ApexTelemetryStepEnd(ApexClient* client, const char* stepName, int32_t succeeded, double score)
{
	RunTelemetryCall(client, [&](apex::TelemetryBuffer& telemetryBuffer)
	{
		telemetryBuffer.StepEnd(TextOrEmpty(stepName), succeeded != 0, score);
	});
}

APEX_API int32_t APEX_CALL ApexTelemetryPoseDue(ApexClient* client)
{
	int32_t poseSampleDue = 0;
	RunTelemetryCall(client, [&](apex::TelemetryBuffer& telemetryBuffer)
	{
		poseSampleDue = telemetryBuffer.PoseSampleDue() ? 1 : 0;
	});
	return poseSampleDue;
}

APEX_API void APEX_CALL ApexTelemetryPose(ApexClient* client, const char* trackedId, const ApexPose* pose)
{
	if (pose == nullptr)
	{
		return;
	}

	RunTelemetryCall(client, [&](apex::TelemetryBuffer& telemetryBuffer)
	{
		telemetryBuffer.RecordPose(TextOrEmpty(trackedId), ConvertPose(pose));
	});
}

APEX_API void APEX_CALL ApexTelemetryFlush(ApexClient* client)
{
	RunTelemetryCall(client, [](apex::TelemetryBuffer& telemetryBuffer)
	{
		telemetryBuffer.Flush();
	});
}

#if !defined(APEX_TRANSPORT_ANDROID)
APEX_API void APEX_CALL ApexAndroidSetJavaVm(void* /*javaVirtualMachine*/)
{
}
#endif

}  // extern "C"
