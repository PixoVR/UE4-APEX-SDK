#pragma once

#include "Endpoints.hpp"
#include "LaunchArguments.hpp"
#include "SessionInfo.hpp"
#include "TelemetryBuffer.hpp"
#include "Transport.hpp"
#include "apex/Apex.h"

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace apex
{

// Hosts for a built-in environment. False for APEX_ENV_CUSTOM or unknown values.
bool GetEnvironmentHosts(int32_t environment, PlatformHosts& hosts);

class Client
{
public:
	// Returns nullptr and sets errorMessage when the configuration is unusable.
	static std::unique_ptr<Client> Create(const ApexConfig& config, std::string& errorMessage);
	~Client();

	Client(const Client&) = delete;
	Client& operator=(const Client&) = delete;

	void Poll(double nowSeconds);
	void Cancel(ApexCallId callId);
	// Thread-safe: hands an overridden request's response to the next Poll.
	void CompleteHttpRequest(uint64_t requestId, HttpResponse response);
	// True when the caller may delete the client now; inside Poll the delete
	// is deferred to the end of Poll.
	bool RequestDestroy();
	bool IsDestroyPending() const
	{
		return m_destroyRequested;
	}

	ApexCallId Ping(ApexResultCallback onResult, void* userData);
	ApexCallId Login(const char* username, const char* password, ApexResultCallback onResult, void* userData);
	ApexCallId LoginWithToken(const char* userToken, ApexResultCallback onResult, void* userData);
	ApexCallId QuickIdGetUsers(const char* serialNumber, ApexResultCallback onResult, void* userData);
	ApexCallId QuickIdLogin(const char* serialNumber, const char* username, ApexResultCallback onResult,
							void* userData);
	ApexCallId GenerateLoginCode(int32_t userId, ApexResultCallback onResult, void* userData);
	void Logout();
	ApexCallId GetUserModules(int32_t userId, ApexResultCallback onResult, void* userData);
	ApexCallId CheckModuleAccess(int32_t moduleId, ApexResultCallback onResult, void* userData);
	ApexCallId GetModuleList(const char* platformName, ApexResultCallback onResult, void* userData);
	ApexCallId GetModulePage(const char* platformName, uint32_t lifecycleMask, int32_t sortOrder, int32_t pageIndex,
							 int32_t pageSize, bool refresh, ApexResultCallback onResult, void* userData);

	// moduleData: the module's bytes, sent base64-encoded as "data".
	ApexCallId JoinSession(const std::string& moduleData, ApexResultCallback onResult, void* userData);
	ApexCallId SendSessionEvent(const std::string& moduleData, ApexResultCallback onResult, void* userData);
	ApexCallId CompleteSession(const std::string& moduleData, ApexResultCallback onResult, void* userData);

	bool IsLoggedIn() const
	{
		return !m_userToken.empty();
	}
	std::string GetCurrentUserJson() const;
	// 0 when no session is active.
	int64_t GetActiveSessionId() const;
	void SetDeviceId(const char* deviceId);
	void SetSessionFormat(int32_t sessionFormat);

	void ParseLaunchCommandLine(int argumentCount, const char* const* argumentValues);
	void ParseLaunchUrl(const char* launchUrl);
	void SetLaunchArgument(const char* key, const char* value);
	std::string GetLaunchArgument(const char* key) const;
	std::string GetExitTargetJson(const char* nextTarget) const;

	// Telemetry records; see Apex.h. Owner thread only.
	TelemetryBuffer& GetTelemetryBuffer()
	{
		return *m_telemetryBuffer;
	}

	// Delivers a failure, on the next Poll, for a call that could not start
	// (used by the C layer too).
	ApexCallId FailImmediately(EApexError error, std::string message, ApexResultCallback onResult, void* userData);

private:
	using ResponseParser = std::function<CallOutcome(const HttpResponse&)>;
	// Runs before the result callback, success or failure; may turn a success
	// into a failure when the client's state moved on meanwhile.
	using FinishHandler = std::function<void(CallOutcome&)>;

	enum class ESessionState
	{
		IDLE,
		JOINING,
		ACTIVE,
		COMPLETING
	};

	// A call whose HTTP request is in flight.
	class PendingCall
	{
	public:
		PendingCall(ApexResultCallback onResult, void* userData, ResponseParser responseParser,
					FinishHandler finishHandler, std::string requestLabel)
			: m_onResult(onResult),
			  m_userData(userData),
			  m_responseParser(std::move(responseParser)),
			  m_finishHandler(std::move(finishHandler)),
			  m_requestLabel(std::move(requestLabel))
		{
		}

		ApexResultCallback GetResultCallback() const
		{
			return m_onResult;
		}
		void* GetUserData() const
		{
			return m_userData;
		}
		CallOutcome ParseResponse(const HttpResponse& response) const
		{
			return m_responseParser(response);
		}
		void RunFinishHandler(CallOutcome& outcome) const
		{
			if (m_finishHandler)
			{
				m_finishHandler(outcome);
			}
		}
		// "POST /login", for logs.
		const std::string& GetRequestLabel() const
		{
			return m_requestLabel;
		}

	private:
		ApexResultCallback m_onResult;
		void* m_userData;
		ResponseParser m_responseParser;
		FinishHandler m_finishHandler;
		std::string m_requestLabel;
	};

	// A call whose result waits for delivery in Poll.
	class CompletedCall
	{
	public:
		CompletedCall(ApexCallId callId, ApexResultCallback onResult, void* userData, CallOutcome outcome)
			: m_callId(callId), m_onResult(onResult), m_userData(userData), m_outcome(std::move(outcome))
		{
		}

		// Calls the result callback, if the call has one.
		void Deliver() const;

	private:
		ApexCallId m_callId;
		ApexResultCallback m_onResult;
		void* m_userData;
		CallOutcome m_outcome;
	};

	// Responses handed over by transport threads and ApexHttpComplete. Shared
	// with the transport's completions, so it outlives the client if needed.
	class ResponseInbox
	{
	public:
		using ReceivedResponses = std::vector<std::pair<uint64_t, HttpResponse>>;

		void Add(uint64_t requestId, HttpResponse response)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_receivedResponses.emplace_back(requestId, std::move(response));
		}

		ReceivedResponses TakeAll()
		{
			ReceivedResponses takenResponses;
			std::lock_guard<std::mutex> lock(m_mutex);
			takenResponses.swap(m_receivedResponses);
			return takenResponses;
		}

	private:
		std::mutex m_mutex;
		ReceivedResponses m_receivedResponses;
	};

	explicit Client(const ApexConfig& config);

	ApexCallId StartCall(HttpRequest request, ResponseParser responseParser, FinishHandler finishHandler,
						 ApexResultCallback onResult, void* userData);
	// Completes on the next poll without a request, for answers the client already has.
	ApexCallId SucceedImmediately(Json payload, ApexResultCallback onResult, void* userData);
	// Fetches the module catalog; finishWithCatalog turns the stored catalog into the call's payload.
	ApexCallId FetchModuleCatalog(std::function<Json(const Json&)> finishWithCatalog, ApexResultCallback onResult,
								  void* userData);
	void DispatchRequest(HttpRequest& request);
	void DispatchToEngine(const HttpRequest& request);
	void ProcessReceivedResponses();
	void DriveTelemetryAndHeartbeat();
	void DeliverCompletedCalls();
	// The finish handler for every login call: the user becomes current.
	FinishHandler MakeLoginFinishHandler();
	// The /event body for a session call in the session's format. Throws
	// std::invalid_argument when xAPI session data is not a JSON object.
	Json MakeSessionBody(const char* eventType, const SessionInfo& session, const std::string& moduleData) const;
	void SendHeartbeat();
	void EndSession();
	bool IsSessionOpen() const;
	void SendTelemetryPacket(const Json& packet);
	TelemetryContext BuildTelemetryContext() const;
	int64_t GetCurrentUserId() const;
	void Log(EApexLogLevel logLevel, const std::string& message) const;

	PlatformHosts m_hosts;
	int32_t m_moduleId = 0;
	std::string m_moduleVersion;
	std::string m_deviceId;
	std::string m_deviceModel;
	std::string m_platform;
	std::string m_clientIp;
	int32_t m_sessionFormat = APEX_SESSION_FORMAT_DATA;
	double m_heartbeatIntervalSeconds = 5.0;
	uint32_t m_requestTimeoutMilliseconds = 30000;
	ApexLogCallback m_logCallback = nullptr;
	void* m_logUserData = nullptr;
	int32_t m_logLevel = APEX_LOG_INFO;
	ApexHttpSendCallback m_httpSendCallback = nullptr;
	void* m_httpSendUserData = nullptr;

	int32_t m_telemetrySink = APEX_TELEMETRY_OFF;
	std::string m_telemetryUrl;
	ApexTelemetryCallback m_telemetryCallback = nullptr;
	void* m_telemetryUserData = nullptr;
	std::unique_ptr<TelemetryBuffer> m_telemetryBuffer;

	std::unique_ptr<Transport> m_transport;
	std::shared_ptr<ResponseInbox> m_responseInbox;
	uint64_t m_nextCallId = 1;
	std::unordered_map<uint64_t, PendingCall> m_pendingCalls;
	std::vector<CompletedCall> m_completedCalls;
	double m_nowSeconds = 0.0;
	bool m_insidePoll = false;
	bool m_destroyRequested = false;

	Json m_currentUser;  // null when logged out
	std::string m_userToken;
	Json m_moduleCatalog;  // the user's modules as the platform sent them; null until fetched

	ESessionState m_sessionState = ESessionState::IDLE;
	SessionInfo m_session;
	uint64_t m_sessionGeneration = 0;  // bumped when a session ends, to drop stale replies
	double m_nextHeartbeatSeconds = 0.0;
	bool m_heartbeatInFlight = false;

	LaunchArguments m_launchArguments;
};

}  // namespace apex
