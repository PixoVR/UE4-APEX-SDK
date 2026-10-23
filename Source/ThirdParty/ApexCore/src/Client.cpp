#include "Client.hpp"

#include "XapiStatements.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace apex
{

bool GetEnvironmentHosts(int32_t environment, PlatformHosts& hosts)
{
	switch (environment)
	{
		case APEX_ENV_NA_PRODUCTION:
			hosts.SetHeadsetApiUrl("https://modules.apex.pixovr.com");
			hosts.SetPlatformApiUrl("https://apex.pixovr.com");
			return true;
		case APEX_ENV_NA_STAGING:
			hosts.SetHeadsetApiUrl("https://modules.apex.stage.pixovr.com");
			hosts.SetPlatformApiUrl("https://apex.stage.pixovr.com");
			return true;
		case APEX_ENV_NA_DEVELOPMENT:
			hosts.SetHeadsetApiUrl("https://modules.apex.dev.pixovr.com");
			hosts.SetPlatformApiUrl("https://apex.dev.pixovr.com");
			return true;
		case APEX_ENV_SA_PRODUCTION:
			hosts.SetHeadsetApiUrl("https://saudi.modules.apex.pixovr.com");
			hosts.SetPlatformApiUrl("https://saudi.apex.pixovr.com");
			return true;
		case APEX_ENV_LOCAL:
			hosts.SetHeadsetApiUrl("http://localhost:8001");
			hosts.SetPlatformApiUrl("http://localhost:8000");
			return true;
		default:
			return false;
	}
}

namespace
{

std::string TextOrEmpty(const char* text)
{
	return text != nullptr ? std::string(text) : std::string();
}

bool IsSetText(const char* text)
{
	return text != nullptr && text[0] != '\0';
}

bool IsPositiveFinite(double value)
{
	return value > 0.0 && std::isfinite(value);
}

double GetMonotonicSeconds()
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

int64_t GetUnixMilliseconds()
{
	return static_cast<int64_t>(
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
			.count());
}

// "METHOD /path" with the query string removed, since it may carry serial numbers.
std::string MakeRequestLabel(const HttpRequest& request)
{
	const std::string& url = request.GetUrl();
	const size_t pathStart = url.find('/', url.find("://") + 3);
	std::string requestLabel =
		request.GetMethod() + " " + (pathStart == std::string::npos ? std::string("/") : url.substr(pathStart));
	const size_t queryStart = requestLabel.find('?');
	if (queryStart != std::string::npos)
	{
		requestLabel.erase(queryStart);
	}

	return requestLabel;
}

// Sets a flag for its lifetime, so Poll's in-progress flag is cleared however
// Poll leaves.
class ScopedFlag
{
public:
	explicit ScopedFlag(bool& flag) : m_flag(flag)
	{
		m_flag = true;
	}
	~ScopedFlag()
	{
		m_flag = false;
	}

	ScopedFlag(const ScopedFlag&) = delete;
	ScopedFlag& operator=(const ScopedFlag&) = delete;

private:
	bool& m_flag;
};

}  // namespace

std::unique_ptr<Client> Client::Create(const ApexConfig& config, std::string& errorMessage)
{
	PlatformHosts hosts;
	if (config.Environment != APEX_ENV_CUSTOM && !GetEnvironmentHosts(config.Environment, hosts))
	{
		errorMessage = "Unknown environment " + std::to_string(config.Environment);
		return nullptr;
	}

	if (IsSetText(config.HeadsetApiUrl))
	{
		hosts.SetHeadsetApiUrl(config.HeadsetApiUrl);
	}

	if (IsSetText(config.PlatformApiUrl))
	{
		hosts.SetPlatformApiUrl(config.PlatformApiUrl);
	}

	if (hosts.GetHeadsetApiUrl().empty() || hosts.GetPlatformApiUrl().empty())
	{
		errorMessage = "APEX_ENV_CUSTOM needs m_headsetApiUrl and m_platformApiUrl";
		return nullptr;
	}

	// The constructor is private, so std::make_unique cannot call it.
	std::unique_ptr<Client> client(new Client(config));
	client->m_hosts.SetHeadsetApiUrl(hosts.GetHeadsetApiUrl());
	client->m_hosts.SetPlatformApiUrl(hosts.GetPlatformApiUrl());
	if (client->m_httpSendCallback == nullptr)
	{
		client->m_transport = MakeBuiltinTransport();
		if (!client->m_transport)
		{
			client->Log(APEX_LOG_WARNING,
						"No built-in HTTP backend on this platform; set ApexConfig.m_httpSendCallback to make calls");
		}
	}

	return client;
}

Client::Client(const ApexConfig& config) : m_responseInbox(std::make_shared<ResponseInbox>())
{
	std::string eventPath = IsSetText(config.EventPath) ? std::string(config.EventPath) : std::string("/event");
	if (eventPath.front() != '/')
	{
		eventPath.insert(eventPath.begin(), '/');
	}

	m_hosts.SetEventPath(eventPath);
	m_moduleId = config.ModuleId;
	m_moduleVersion = TextOrEmpty(config.ModuleVersion);
	m_deviceId = TextOrEmpty(config.DeviceId);
	m_deviceModel = TextOrEmpty(config.DeviceModel);
	m_platform = TextOrEmpty(config.Platform);
	m_clientIp = TextOrEmpty(config.ClientIp);
	SetSessionFormat(config.SessionFormat);
	if (IsPositiveFinite(config.HeartbeatIntervalSeconds))
	{
		m_heartbeatIntervalSeconds = config.HeartbeatIntervalSeconds;
	}

	if (config.RequestTimeoutMilliseconds > 0)
	{
		m_requestTimeoutMilliseconds = config.RequestTimeoutMilliseconds;
	}

	m_logCallback = config.LogCallback;
	m_logUserData = config.LogUserData;
	m_logLevel = config.LogLevel;
	m_httpSendCallback = config.HttpSendCallback;
	m_httpSendUserData = config.HttpSendUserData;

	m_telemetrySink = config.TelemetrySink;
	m_telemetryUrl = TextOrEmpty(config.TelemetryUrl);
	m_telemetryCallback = config.TelemetryCallback;
	m_telemetryUserData = config.TelemetryUserData;
	TelemetryOptions telemetryOptions;
	if (IsPositiveFinite(config.TelemetryFlushIntervalSeconds))
	{
		telemetryOptions.SetFlushIntervalSeconds(config.TelemetryFlushIntervalSeconds);
	}

	if (IsPositiveFinite(config.TelemetryPoseSamplesPerSecond))
	{
		telemetryOptions.SetPoseSamplesPerSecond(config.TelemetryPoseSamplesPerSecond);
	}

	if (config.TelemetryMaximumRecordsPerPacket > 0)
	{
		telemetryOptions.SetMaximumRecordsPerPacket(config.TelemetryMaximumRecordsPerPacket);
	}

	m_telemetryBuffer = std::make_unique<TelemetryBuffer>(telemetryOptions, GetMonotonicSeconds, GetUnixMilliseconds,
														  [this]
	{
		return BuildTelemetryContext();
	}, [this](const Json& packet)
	{
		SendTelemetryPacket(packet);
	});
}

Client::~Client()
{
	// Stops the transport's workers; late completions land in the orphaned inbox.
	m_transport.reset();
}

void Client::Log(EApexLogLevel logLevel, const std::string& message) const
{
	if (m_logCallback != nullptr && logLevel >= m_logLevel)
	{
		m_logCallback(m_logUserData, logLevel, message.c_str());
	}
}

void Client::CompletedCall::Deliver() const
{
	if (m_onResult == nullptr)
	{
		return;
	}

	const bool succeeded = m_outcome.Succeeded();
	const std::string payloadText = succeeded ? SerializeJson(m_outcome.GetPayload()) : std::string();
	ApexResult result = {};
	result.CallId = m_callId;
	result.Error = m_outcome.GetError();
	result.HttpStatus = m_outcome.GetHttpStatus();
	result.Message = m_outcome.GetErrorMessage().c_str();
	result.Json = succeeded ? payloadText.c_str() : nullptr;
	m_onResult(m_userData, &result);
}

ApexCallId Client::FailImmediately(EApexError error, std::string message, ApexResultCallback onResult, void* userData)
{
	const ApexCallId callId = m_nextCallId++;
	m_completedCalls.emplace_back(callId, onResult, userData, CallOutcome::Failure(error, 0, std::move(message)));
	return callId;
}

ApexCallId Client::SucceedImmediately(Json payload, ApexResultCallback onResult, void* userData)
{
	const ApexCallId callId = m_nextCallId++;
	CallOutcome success;
	success.SetPayload(std::move(payload));
	m_completedCalls.emplace_back(callId, onResult, userData, std::move(success));
	return callId;
}

ApexCallId Client::StartCall(HttpRequest request, ResponseParser responseParser, FinishHandler finishHandler,
							 ApexResultCallback onResult, void* userData)
{
	const uint64_t callId = m_nextCallId++;
	request.SetRequestId(callId);
	request.SetTimeoutMilliseconds(m_requestTimeoutMilliseconds);

	m_pendingCalls.emplace(callId, PendingCall(onResult, userData, std::move(responseParser), std::move(finishHandler),
											   MakeRequestLabel(request)));
	try
	{
		DispatchRequest(request);
	}
	catch (...)
	{
		m_pendingCalls.erase(callId);
		throw;
	}

	return callId;
}

void Client::DispatchToEngine(const HttpRequest& request)
{
	std::vector<ApexHttpHeader> engineHeaders;
	engineHeaders.reserve(request.GetHeaders().size());
	for (const auto& [headerName, headerValue] : request.GetHeaders())
	{
		engineHeaders.push_back({headerName.c_str(), headerValue.c_str()});
	}

	ApexHttpRequest engineRequest = {};
	engineRequest.RequestId = request.GetRequestId();
	engineRequest.Method = request.GetMethod().c_str();
	engineRequest.Url = request.GetUrl().c_str();
	engineRequest.Headers = engineHeaders.data();
	engineRequest.HeaderCount = engineHeaders.size();
	engineRequest.Body = request.GetBody().empty() ? nullptr : request.GetBody().data();
	engineRequest.BodyLength = request.GetBody().size();
	engineRequest.TimeoutMilliseconds = request.GetTimeoutMilliseconds();
	m_httpSendCallback(m_httpSendUserData, &engineRequest);
}

void Client::DispatchRequest(HttpRequest& request)
{
	if (m_httpSendCallback != nullptr)
	{
		DispatchToEngine(request);
		return;
	}

	if (!m_transport)
	{
		HttpResponse noBackendResponse;
		noBackendResponse.SetTransportError("no HTTP backend");
		CompleteHttpRequest(request.GetRequestId(), std::move(noBackendResponse));
		return;
	}

	const std::weak_ptr<ResponseInbox> weakResponseInbox = m_responseInbox;
	m_transport->Send(std::move(request), [weakResponseInbox](uint64_t requestId, HttpResponse response)
	{
		const std::shared_ptr<ResponseInbox> responseInbox = weakResponseInbox.lock();
		if (responseInbox)
		{
			responseInbox->Add(requestId, std::move(response));
		}
	});
}

void Client::CompleteHttpRequest(uint64_t requestId, HttpResponse response)
{
	m_responseInbox->Add(requestId, std::move(response));
}

void Client::Poll(double nowSeconds)
{
	if (m_insidePoll)
	{
		return;  // Poll called from inside a callback
	}

	const ScopedFlag insidePoll(m_insidePoll);
	m_nowSeconds = nowSeconds;

	ProcessReceivedResponses();
	DriveTelemetryAndHeartbeat();
	DeliverCompletedCalls();
}

void Client::ProcessReceivedResponses()
{
	for (auto& [requestId, response] : m_responseInbox->TakeAll())
	{
		const auto pendingEntry = m_pendingCalls.find(requestId);
		if (pendingEntry == m_pendingCalls.end())
		{
			continue;  // cancelled, or a reply to an unknown id
		}

		const PendingCall pendingCall = std::move(pendingEntry->second);
		m_pendingCalls.erase(pendingEntry);

		CallOutcome outcome;
		try
		{
			if (!m_transport && m_httpSendCallback == nullptr)
			{
				outcome = CallOutcome::Failure(APEX_ERR_NO_TRANSPORT, 0,
											   "No HTTP backend on this platform; set ApexConfig.m_httpSendCallback");
			}
			else
			{
				outcome = pendingCall.ParseResponse(response);
			}

			const std::string statusText =
				response.GetStatusCode() > 0 ? std::to_string(response.GetStatusCode()) : std::string("no response");
			Log(outcome.Succeeded() ? APEX_LOG_DEBUG : APEX_LOG_WARNING,
				pendingCall.GetRequestLabel() + " -> " + statusText +
					(outcome.Succeeded() ? std::string() : " (" + outcome.GetErrorMessage() + ")"));
			pendingCall.RunFinishHandler(outcome);
		}
		catch (const std::exception& exception)
		{
			outcome = CallOutcome::Failure(APEX_ERR_INTERNAL, response.GetStatusCode(), exception.what());
		}

		m_completedCalls.emplace_back(requestId, pendingCall.GetResultCallback(), pendingCall.GetUserData(),
									  std::move(outcome));
	}
}

void Client::DriveTelemetryAndHeartbeat()
{
	try
	{
		m_telemetryBuffer->Tick(IsSessionOpen());
	}
	catch (const std::exception& exception)
	{
		Log(APEX_LOG_WARNING, std::string("Telemetry not sent: ") + exception.what());
	}

	if (!IsSessionOpen() || m_nowSeconds < m_nextHeartbeatSeconds)
	{
		return;
	}

	m_nextHeartbeatSeconds = m_nowSeconds + m_heartbeatIntervalSeconds;
	if (m_heartbeatInFlight)
	{
		return;
	}

	try
	{
		SendHeartbeat();
	}
	catch (const std::exception& exception)
	{
		m_heartbeatInFlight = false;
		Log(APEX_LOG_WARNING, std::string("Heartbeat not sent: ") + exception.what());
	}
}

void Client::DeliverCompletedCalls()
{
	std::vector<CompletedCall> callsToDeliver;
	callsToDeliver.swap(m_completedCalls);
	for (const CompletedCall& completedCall : callsToDeliver)
	{
		if (m_destroyRequested)
		{
			break;  // ApexClientDestroy was called during this Poll
		}

		completedCall.Deliver();
	}
}

bool Client::RequestDestroy()
{
	if (m_insidePoll)
	{
		m_destroyRequested = true;
		return false;
	}

	return true;
}

void Client::Cancel(ApexCallId callId)
{
	const auto pendingEntry = m_pendingCalls.find(callId);
	if (pendingEntry == m_pendingCalls.end())
	{
		return;  // unknown or already finished
	}

	const PendingCall pendingCall = std::move(pendingEntry->second);
	m_pendingCalls.erase(pendingEntry);

	CallOutcome outcome = CallOutcome::Failure(APEX_ERR_CANCELLED, 0, "Cancelled");
	pendingCall.RunFinishHandler(outcome);
	m_completedCalls.emplace_back(callId, pendingCall.GetResultCallback(), pendingCall.GetUserData(),
								  std::move(outcome));
}

// ---------------------------------------------------------------------------
// Auth and users
// ---------------------------------------------------------------------------

Client::FinishHandler Client::MakeLoginFinishHandler()
{
	return [this](CallOutcome& outcome)
	{
		if (outcome.Succeeded())
		{
			EndSession();
			m_moduleCatalog = Json();
			m_currentUser = outcome.GetPayload();
			m_userToken = GetString(outcome.GetPayload(), "token").value_or("");
		}
	};
}

ApexCallId Client::Ping(ApexResultCallback onResult, void* userData)
{
	return StartCall(endpoints::MakePingRequest(m_hosts), endpoints::ParsePingResponse, nullptr, onResult, userData);
}

ApexCallId Client::Login(const char* username, const char* password, ApexResultCallback onResult, void* userData)
{
	if (!IsSetText(username) || password == nullptr)
	{
		return FailImmediately(APEX_ERR_INVALID_ARGUMENT, "Username and password are required", onResult, userData);
	}

	return StartCall(endpoints::MakeLoginRequest(m_hosts, username, password), endpoints::ParseLoginResponse,
					 MakeLoginFinishHandler(), onResult, userData);
}

ApexCallId Client::LoginWithToken(const char* userToken, ApexResultCallback onResult, void* userData)
{
	std::string tokenToValidate = TextOrEmpty(userToken);
	if (tokenToValidate.empty())
	{
		tokenToValidate = GetLaunchArgument(launch_keys::kToken);
	}

	if (tokenToValidate.empty())
	{
		return FailImmediately(APEX_ERR_INVALID_ARGUMENT, "No token given and no pixotoken launch argument", onResult,
							   userData);
	}

	return StartCall(endpoints::MakeTokenLoginRequest(m_hosts, tokenToValidate),
					 [tokenToValidate](const HttpResponse& response)
	{
		return endpoints::ParseTokenLoginResponse(response, tokenToValidate);
	}, MakeLoginFinishHandler(), onResult, userData);
}

ApexCallId Client::QuickIdGetUsers(const char* serialNumber, ApexResultCallback onResult, void* userData)
{
	const std::string deviceSerial = serialNumber != nullptr ? std::string(serialNumber) : m_deviceId;
	if (deviceSerial.empty())
	{
		return FailImmediately(APEX_ERR_INVALID_ARGUMENT, "A device serial number is required", onResult, userData);
	}

	return StartCall(endpoints::MakeQuickIdUsersRequest(m_hosts, deviceSerial), endpoints::ParseQuickIdUsersResponse,
					 nullptr, onResult, userData);
}

ApexCallId Client::QuickIdLogin(const char* serialNumber, const char* username, ApexResultCallback onResult,
								void* userData)
{
	const std::string deviceSerial = serialNumber != nullptr ? std::string(serialNumber) : m_deviceId;
	if (deviceSerial.empty() || !IsSetText(username))
	{
		return FailImmediately(APEX_ERR_INVALID_ARGUMENT, "A device serial number and a username are required",
							   onResult, userData);
	}

	return StartCall(endpoints::MakeQuickIdLoginRequest(m_hosts, deviceSerial, username),
					 endpoints::ParseQuickIdLoginResponse, MakeLoginFinishHandler(), onResult, userData);
}

ApexCallId Client::GenerateLoginCode(int32_t userId, ApexResultCallback onResult, void* userData)
{
	if (!IsLoggedIn())
	{
		return FailImmediately(APEX_ERR_NOT_LOGGED_IN, "Log in before generating a login code", onResult, userData);
	}

	return StartCall(endpoints::MakeLoginCodeRequest(m_hosts, m_userToken, userId), endpoints::ParseLoginCodeResponse,
					 nullptr, onResult, userData);
}

void Client::Logout()
{
	EndSession();
	m_currentUser = Json();
	m_userToken.clear();
	m_moduleCatalog = Json();
}

ApexCallId Client::GetUserModules(int32_t userId, ApexResultCallback onResult, void* userData)
{
	if (!IsLoggedIn())
	{
		return FailImmediately(APEX_ERR_NOT_LOGGED_IN, "Log in before requesting user modules", onResult, userData);
	}

	const int64_t requestedUserId = userId >= 0 ? userId : GetCurrentUserId();
	return StartCall(endpoints::MakeUserModulesRequest(m_hosts, m_userToken, requestedUserId),
					 endpoints::ParseUserModulesResponse, nullptr, onResult, userData);
}

ApexCallId Client::CheckModuleAccess(int32_t moduleId, ApexResultCallback onResult, void* userData)
{
	if (!IsLoggedIn())
	{
		return FailImmediately(APEX_ERR_NOT_LOGGED_IN, "Log in before checking module access", onResult, userData);
	}

	const int64_t checkedModuleId = moduleId >= 0 ? moduleId : m_moduleId;
	if (checkedModuleId <= 0)
	{
		return FailImmediately(APEX_ERR_INVALID_ARGUMENT, "No module ID given and none configured", onResult, userData);
	}

	return StartCall(endpoints::MakeModuleAccessRequest(m_hosts, GetCurrentUserId(), checkedModuleId, m_deviceId),
					 endpoints::ParseModuleAccessResponse, nullptr, onResult, userData);
}

ApexCallId Client::GetModuleList(const char* platformName, ApexResultCallback onResult, void* userData)
{
	if (!IsLoggedIn())
	{
		return FailImmediately(APEX_ERR_NOT_LOGGED_IN, "Log in before requesting the module list", onResult, userData);
	}

	const std::string platformFilter = TextOrEmpty(platformName);
	return FetchModuleCatalog([platformFilter](const Json& moduleCatalog)
	{
		return Json{
			{"modules", endpoints::FilterModules(moduleCatalog, platformFilter, endpoints::kEveryReturnedLifecycle)}};
	}, onResult, userData);
}

ApexCallId Client::GetModulePage(const char* platformName, uint32_t lifecycleMask, int32_t sortOrder, int32_t pageIndex,
								 int32_t pageSize, bool refresh, ApexResultCallback onResult, void* userData)
{
	if (!IsLoggedIn())
	{
		return FailImmediately(APEX_ERR_NOT_LOGGED_IN, "Log in before requesting modules", onResult, userData);
	}

	if (pageIndex < 0 || pageSize < 1)
	{
		return FailImmediately(APEX_ERR_INVALID_ARGUMENT, "pageIndex must be 0 or more and pageSize 1 or more",
							   onResult, userData);
	}

	if (sortOrder < APEX_MODULE_SORT_PLATFORM || sortOrder > APEX_MODULE_SORT_NEWEST_VERSION)
	{
		return FailImmediately(APEX_ERR_INVALID_ARGUMENT, "Unknown sort order", onResult, userData);
	}

	if ((lifecycleMask & ~endpoints::kEveryReturnedLifecycle) != 0)
	{
		return FailImmediately(APEX_ERR_INVALID_ARGUMENT, "Unknown lifecycle flag", onResult, userData);
	}

	const std::string platformFilter = TextOrEmpty(platformName);
	const uint32_t lifecycleFilter =
		lifecycleMask != 0 ? lifecycleMask : static_cast<uint32_t>(APEX_LIFECYCLE_RELEASED);
	const auto buildPage = [platformFilter, lifecycleFilter, sortOrder, pageIndex, pageSize](const Json& moduleCatalog)
	{
		Json normalizedModules = endpoints::FilterModules(moduleCatalog, platformFilter, lifecycleFilter);
		endpoints::SortModules(normalizedModules, static_cast<EApexModuleSort>(sortOrder));
		return endpoints::BuildModulePage(normalizedModules, pageIndex, pageSize);
	};

	if (!refresh && !m_moduleCatalog.is_null())
	{
		return SucceedImmediately(buildPage(m_moduleCatalog), onResult, userData);
	}

	return FetchModuleCatalog(buildPage, onResult, userData);
}

ApexCallId Client::FetchModuleCatalog(std::function<Json(const Json&)> finishWithCatalog, ApexResultCallback onResult,
									  void* userData)
{
	const int64_t requestingUserId = GetCurrentUserId();
	return StartCall(endpoints::MakeModuleListRequest(m_hosts, m_userToken, requestingUserId),
					 endpoints::ParseModuleCatalogResponse,
					 [this, requestingUserId, finishWithCatalog](CallOutcome& outcome)
	{
		if (!outcome.Succeeded())
		{
			return;
		}

		const Json& moduleCatalog = outcome.GetPayload()["modules"];
		// A reply for a user who has since logged out is still answered, but not kept.
		if (IsLoggedIn() && GetCurrentUserId() == requestingUserId)
		{
			m_moduleCatalog = moduleCatalog;
		}

		outcome.SetPayload(finishWithCatalog(moduleCatalog));
	}, onResult, userData);
}

int64_t Client::GetCurrentUserId() const
{
	return GetInteger(m_currentUser, "id").value_or(-1);
}

std::string Client::GetCurrentUserJson() const
{
	return m_currentUser.is_null() ? std::string() : SerializeJson(m_currentUser);
}

// ---------------------------------------------------------------------------
// Sessions
// ---------------------------------------------------------------------------

bool Client::IsSessionOpen() const
{
	return m_sessionState == ESessionState::ACTIVE || m_sessionState == ESessionState::COMPLETING;
}

int64_t Client::GetActiveSessionId() const
{
	return IsSessionOpen() ? m_session.GetSessionId() : 0;
}

void Client::SetSessionFormat(int32_t sessionFormat)
{
	if (sessionFormat == APEX_SESSION_FORMAT_DATA || sessionFormat == APEX_SESSION_FORMAT_XAPI)
	{
		m_sessionFormat = sessionFormat;
	}
	else
	{
		Log(APEX_LOG_WARNING, "Unknown session format " + std::to_string(sessionFormat) + " ignored");
	}
}

void Client::SetDeviceId(const char* deviceId)
{
	m_deviceId = TextOrEmpty(deviceId);
}

void Client::EndSession()
{
	if (IsSessionOpen())
	{
		// The session's last packet, marked "end", still carries its IDs.
		try
		{
			m_telemetryBuffer->FlushSessionEnd();
		}
		catch (const std::exception& exception)
		{
			Log(APEX_LOG_WARNING, std::string("Final telemetry packet not sent: ") + exception.what());
		}
	}

	m_sessionState = ESessionState::IDLE;
	m_session = SessionInfo();
	m_heartbeatInFlight = false;
	++m_sessionGeneration;
}

ApexCallId Client::JoinSession(const std::string& moduleData, ApexResultCallback onResult, void* userData)
{
	if (!IsLoggedIn())
	{
		return FailImmediately(APEX_ERR_NOT_LOGGED_IN, "Log in before joining a session", onResult, userData);
	}

	if (m_sessionState != ESessionState::IDLE)
	{
		return FailImmediately(APEX_ERR_INVALID_STATE, "A session is already active", onResult, userData);
	}

	SessionInfo joiningSession;
	joiningSession.SetUuid(MakeUuid());
	joiningSession.SetModuleId(m_moduleId);
	joiningSession.SetSessionFormat(m_sessionFormat);
	Json joinBody;
	try
	{
		if (m_sessionFormat == APEX_SESSION_FORMAT_XAPI)
		{
			// The join data's device and SDK details go on every statement of the session.
			const Json joinData = Json::parse(moduleData, nullptr, false);
			const Json noJoinData = Json::object();
			const Json& joinDetails = joinData.is_object() ? joinData : noJoinData;
			const std::string joinPlatform = GetString(joinDetails, "platform").value_or("");
			const std::string joinDeviceId = GetString(joinDetails, "deviceId").value_or("");
			joiningSession.SetXapiPlatform(joinPlatform.empty() ? m_platform : joinPlatform);
			joiningSession.SetXapiDeviceId(joinDeviceId.empty() ? m_deviceId : joinDeviceId);
			joiningSession.SetXapiStandardExtensions(
				xapi::MakeStandardExtensions(joinDetails, m_moduleId, m_deviceId, m_deviceModel));
		}

		joinBody = MakeSessionBody(event_types::kJoined, joiningSession, moduleData);
	}
	catch (const std::invalid_argument& invalidData)
	{
		return FailImmediately(APEX_ERR_INVALID_ARGUMENT, invalidData.what(), onResult, userData);
	}

	HttpRequest joinRequest = endpoints::MakeSessionEventRequest(m_hosts, m_userToken, joinBody);
	// State changes only once the request is built, so a failure above
	// leaves the client idle.
	m_session = std::move(joiningSession);
	m_sessionState = ESessionState::JOINING;
	const uint64_t joinGeneration = m_sessionGeneration;
	try
	{
		return StartCall(std::move(joinRequest), endpoints::ParseSessionJoinResponse,
						 [this, joinGeneration](CallOutcome& outcome)
		{
			if (joinGeneration != m_sessionGeneration || m_sessionState != ESessionState::JOINING)
			{
				// Logged out or logged in again meanwhile: the session is
				// not this client's any more.
				if (outcome.Succeeded())
				{
					outcome = CallOutcome::Failure(APEX_ERR_INVALID_STATE, outcome.GetHttpStatus(),
												   "The session ended before the platform replied");
				}

				return;
			}

			if (outcome.Succeeded())
			{
				m_session.SetSessionId(GetInteger(outcome.GetPayload(), "sessionId").value_or(0));
				m_sessionState = ESessionState::ACTIVE;
				m_nextHeartbeatSeconds = m_nowSeconds + m_heartbeatIntervalSeconds;
				m_telemetryBuffer->RestartFlushInterval();
			}
			else
			{
				EndSession();
			}
		}, onResult, userData);
	}
	catch (...)
	{
		EndSession();
		throw;
	}
}

ApexCallId Client::SendSessionEvent(const std::string& moduleData, ApexResultCallback onResult, void* userData)
{
	if (m_sessionState != ESessionState::ACTIVE)
	{
		return FailImmediately(APEX_ERR_INVALID_STATE, "No active session", onResult, userData);
	}

	Json eventBody;
	try
	{
		eventBody = MakeSessionBody(event_types::kEvent, m_session, moduleData);
	}
	catch (const std::invalid_argument& invalidData)
	{
		return FailImmediately(APEX_ERR_INVALID_ARGUMENT, invalidData.what(), onResult, userData);
	}

	return StartCall(endpoints::MakeSessionEventRequest(m_hosts, m_userToken, eventBody),
					 endpoints::ParseAcknowledgementResponse, nullptr, onResult, userData);
}

ApexCallId Client::CompleteSession(const std::string& moduleData, ApexResultCallback onResult, void* userData)
{
	if (m_sessionState != ESessionState::ACTIVE)
	{
		return FailImmediately(APEX_ERR_INVALID_STATE, "No active session", onResult, userData);
	}

	Json completeBody;
	try
	{
		completeBody = MakeSessionBody(event_types::kComplete, m_session, moduleData);
	}
	catch (const std::invalid_argument& invalidData)
	{
		return FailImmediately(APEX_ERR_INVALID_ARGUMENT, invalidData.what(), onResult, userData);
	}

	HttpRequest completeRequest = endpoints::MakeSessionEventRequest(m_hosts, m_userToken, completeBody);
	m_sessionState = ESessionState::COMPLETING;
	const uint64_t completeGeneration = m_sessionGeneration;
	try
	{
		return StartCall(std::move(completeRequest), endpoints::ParseAcknowledgementResponse,
						 [this, completeGeneration](CallOutcome& outcome)
		{
			if (completeGeneration != m_sessionGeneration || m_sessionState != ESessionState::COMPLETING)
			{
				return;
			}

			if (outcome.Succeeded())
			{
				EndSession();
			}
			else
			{
				m_sessionState = ESessionState::ACTIVE;  // the module may retry
			}
		}, onResult, userData);
	}
	catch (...)
	{
		m_sessionState = ESessionState::ACTIVE;
		throw;
	}
}

Json Client::MakeSessionBody(const char* eventType, const SessionInfo& session, const std::string& moduleData) const
{
	if (session.GetSessionFormat() != APEX_SESSION_FORMAT_XAPI)
	{
		return MakeEventBody(eventType, session, moduleData);
	}

	const Json moduleJson = Json::parse(moduleData, nullptr, false);
	if (!moduleJson.is_object())
	{
		throw std::invalid_argument("xAPI session data must be a JSON object");
	}

	xapi::StatementContext statementContext;
	statementContext.SetActorEmail(GetString(m_currentUser, "email").value_or(""));
	statementContext.SetRegistration(session.GetUuid());
	statementContext.SetRevision(m_moduleVersion);
	statementContext.SetPlatform(session.GetXapiPlatform());
	statementContext.SetModuleId(session.GetModuleId());
	statementContext.SetStandardExtensions(session.GetXapiStandardExtensions());
	statementContext.SetStatementId(MakeUuid());
	statementContext.SetTimestamp(xapi::FormatTimestamp(std::chrono::system_clock::now()));
	Json statement = xapi::MakeStatement(eventType, statementContext, moduleJson);
	return xapi::MakeEventBody(eventType, statementContext, session.GetXapiDeviceId(), m_clientIp, std::move(statement),
							   moduleJson);
}

void Client::SendHeartbeat()
{
	m_heartbeatInFlight = true;
	const uint64_t heartbeatGeneration = m_sessionGeneration;
	StartCall(endpoints::MakeHeartbeatRequest(m_hosts, m_userToken, m_session.GetSessionId()),
			  endpoints::ParseAcknowledgementResponse, [this, heartbeatGeneration](CallOutcome& /*outcome*/)
	{
		if (heartbeatGeneration == m_sessionGeneration)
		{
			m_heartbeatInFlight = false;
		}
	}, nullptr, nullptr);
}

// ---------------------------------------------------------------------------
// Launch arguments
// ---------------------------------------------------------------------------

void Client::ParseLaunchCommandLine(int argumentCount, const char* const* argumentValues)
{
	ParseCommandLine(argumentCount, argumentValues, m_launchArguments);
}

void Client::ParseLaunchUrl(const char* launchUrl)
{
	if (launchUrl != nullptr)
	{
		ParseUrlArguments(launchUrl, m_launchArguments);
	}
}

void Client::SetLaunchArgument(const char* key, const char* value)
{
	if (!IsSetText(key))
	{
		return;
	}

	if (value == nullptr)
	{
		m_launchArguments.erase(key);
	}
	else
	{
		m_launchArguments[key] = value;
	}
}

std::string Client::GetLaunchArgument(const char* key) const
{
	if (key == nullptr)
	{
		return std::string();
	}

	const auto foundArgument = m_launchArguments.find(key);
	return foundArgument == m_launchArguments.end() ? std::string() : foundArgument->second;
}

std::string Client::GetExitTargetJson(const char* nextTarget) const
{
	return SerializeJson(BuildExitTarget(m_launchArguments, m_userToken, TextOrEmpty(nextTarget)));
}

// ---------------------------------------------------------------------------
// Telemetry
// ---------------------------------------------------------------------------

TelemetryContext Client::BuildTelemetryContext() const
{
	TelemetryContext telemetryContext;
	telemetryContext.SetUserId(IsLoggedIn() ? std::max<int64_t>(GetCurrentUserId(), 0) : 0);
	telemetryContext.SetModuleId(m_moduleId);
	if (IsSessionOpen())
	{
		telemetryContext.SetSessionId(m_session.GetSessionId());
		// Same ID as the events' uuid, without dashes, as 2.0 sends it.
		std::string registration = m_session.GetUuid();
		registration.erase(std::remove(registration.begin(), registration.end(), '-'), registration.end());
		telemetryContext.SetRegistration(std::move(registration));
	}

	return telemetryContext;
}

void Client::SendTelemetryPacket(const Json& packet)
{
	switch (m_telemetrySink)
	{
		case APEX_TELEMETRY_HTTP:
		{
			if (m_telemetryUrl.empty())
			{
				Log(APEX_LOG_WARNING, "Telemetry sink is HTTP but m_telemetryUrl is not set; packet dropped");
				return;
			}

			HttpRequest uploadRequest;
			uploadRequest.SetMethod("POST");
			uploadRequest.SetUrl(m_telemetryUrl);
			uploadRequest.SetBody(SerializeJson(packet));
			uploadRequest.AddHeader("Content-Type", "application/json");
			if (!m_userToken.empty())
			{
				uploadRequest.AddHeader("Authorization", "Bearer " + m_userToken);
			}

			// Like the 2.0 HTTP sink, a failed upload is logged and not retried.
			StartCall(std::move(uploadRequest), endpoints::ParseAcknowledgementResponse, nullptr, nullptr, nullptr);
			return;
		}
		case APEX_TELEMETRY_CALLBACK:
		{
			if (m_telemetryCallback != nullptr)
			{
				const std::string packetText = SerializeJson(packet);
				m_telemetryCallback(m_telemetryUserData, packetText.c_str());
			}

			return;
		}
		default:
			return;
	}
}

}  // namespace apex
