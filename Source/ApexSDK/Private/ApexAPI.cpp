// Copyright 2022 PixoVR Corp. All Rights Reserved.


#include "ApexAPI.h"
#include "VaRest/Public/VaRestSubsystem.h"
#include "ApexSDK.h"
#include "ApexSDKSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "GenericPlatform/GenericPlatformMisc.h"

DEFINE_LOG_CATEGORY_STATIC(LogApexAPI, Log, All);

#define LogAPEX(pmt, ...) UE_LOG(LogApexAPI, Log, TEXT(pmt), ##__VA_ARGS__)
#define WarnAPEX(pmt, ...) UE_LOG(LogApexAPI, Warning, TEXT(pmt), ##__VA_ARGS__)
#define ErrorAPEX(pmt, ...) UE_LOG(LogApexAPI, Error, TEXT(pmt), ##__VA_ARGS__)
#define FatalAPEX(pmt, ...) UE_LOG(LogApexAPI, Fatal, TEXT(pmt), ##__VA_ARGS__)

UApexAPI::UApexAPI()
    : UGameInstanceSubsystem()
    , ApexSettings(nullptr)
{
}

void UApexAPI::SetupModuleSettings()
{
    if (FApexSDKModule::IsAvailable())
    {
        ApexSettings = FApexSDKModule::Get().GetSettings();
    }

    if (ApexSettings != nullptr)
    {
        URL = ApexSettings->ServerURI;

        if (URL.IsEmpty())
        {
            FatalAPEX("No URL available. Please set the Server URI in the settings.");
        }

        URL.RemoveFromEnd("/");
        if (!URL.Contains("https://"))
        {
            if (URL.Contains("http://"))
            {
                URL = URL.Replace(TEXT("http://"), TEXT(""));
            }

            URL.InsertAt(0, "https://");
        }

        LoadedModuleId = ApexSettings->ModuleId;

        WebSocketURL = ApexSettings->ServerURI;
        if (!WebSocketURL.IsEmpty())
        {
            WebSocketURL.RemoveFromEnd("/");
            if (!WebSocketURL.Contains("wss://"))
            {
                FString URLBody, URLScheme;
                if (WebSocketURL.Split("://", &URLScheme, &URLBody))
                {
                    WebSocketURL = URLBody;
                }

                WebSocketURL = "wss://" + WebSocketURL;
            }

            if (!WebSocketURL.EndsWith("/ws"))
            {
                WebSocketURL = WebSocketURL + "/ws";
            }

            WebSocketEnabled = true;
        }
        else
        {
            WebSocketEnabled = false;
        }

        check(ApexSettings->CheckModuleVersion())
        ModuleVersion = ApexSettings->ModuleVersion;
        LogAPEX("Launching module %d version %s", LoadedModuleId, *ModuleVersion);
    }
}

void UApexAPI::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    VaRestSubsystem = GEngine->GetEngineSubsystem<UVaRestSubsystem>();

    SetupModuleSettings();

    DeviceModel = UHeadMountedDisplayFunctionLibrary::GetHMDDeviceName().ToString();
    // Tag PC launches specifically if there is no HMD available.
    if (DeviceModel.Equals("None", ESearchCase::IgnoreCase))
    {
        DeviceModel = "PC";
    }
    DeviceId = FGenericPlatformMisc::GetDeviceId();
    if (DeviceId.IsEmpty())
    {
        DeviceId = FGenericPlatformMisc::GetDeviceMakeAndModel();
    }

    if (WebSocketEnabled)
    {
        ApexSocket = MakeUnique<ApexWebSocket>();

        ApexSocket->OnApexWebSocketConnected().AddLambda([&]() -> void {OnWebSocketConnected.Broadcast(); OnStaticWebSocketConnected.Broadcast(); });
        ApexSocket->OnApexWebSocketConnectFailed().AddLambda([&](const FString& Error) -> void {OnWebSocketConnectFailed.Broadcast(Error); OnStaticWebSocketConnectFailed.Broadcast(Error); });
        ApexSocket->OnApexWebSocketDisconnected().AddLambda([&](int32 StatusCode, const FString& Reason, bool bWasClean) -> void {OnWebSocketDisconnected.Broadcast(StatusCode, Reason, bWasClean); OnStaticWebSocketDisconnected.Broadcast(StatusCode, Reason, bWasClean); });
        ApexSocket->OnApexAuthCodeRetrieved().AddLambda([&](const FString& AuthorizationCode) -> void {OnAuthorizationCodeRetrieved.Broadcast(AuthorizationCode); OnStaticAuthorizationCodeRetrieved.Broadcast(AuthorizationCode); });
        ApexSocket->OnApexAuthenticateComplete().AddLambda([&](UVaRestJsonObject* LoginJsonObject) -> void { HandleLogin(LoginJsonObject); });

        ApexSocket->AttemptConnect(WebSocketURL);
    }
}

bool UApexAPI::IsWebSocketConnected()
{
    if (WebSocketEnabled)
    {
        return ApexSocket->IsWebsocketConnected();
    }

    return false;
}


void UApexAPI::ConnectWebSocketService()
{
    if (WebSocketEnabled)
    {
        if (!ApexSocket->AttemptConnect(WebSocketURL))
        {
            WarnAPEX("Web Socket already connected.");
        }
    }
}

void UApexAPI::RequestAuthorizationCode()
{
    if (WebSocketEnabled)
    {
        ApexSocket->RequestAuthorizationCode();
    }
}

void UApexAPI::Ping()
{
    UVaRestRequestJSON* Request = VaRestSubsystem->ConstructVaRestRequestExt(EVaRestRequestVerb::GET, EVaRestRequestContentType::none);
    FString RequestURL = URL + "/ping";
    Request->SetURL(RequestURL);

    Request->OnStaticRequestComplete.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnPingComplete(Request); });
    Request->OnStaticRequestFail.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnPingFail(Request); });

    Request->ExecuteProcessRequest();
}

void UApexAPI::OnPingComplete(const UVaRestRequestJSON* Request)
{
    LogAPEX("On Ping Completed!");
    OnRequestComplete.Broadcast(EApexRequestType::Ping, Request);
    OnStaticRequestComplete.Broadcast(EApexRequestType::Ping, Request);
}

void UApexAPI::OnPingFail(const UVaRestRequestJSON* Request)
{
    LogAPEX("On Ping Failed!");

    UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
    FAPEXRequestFailed APEXFailedRequest;

    APEXFailedRequest.FromJsonObject(ResponseObject->GetRootObject());

    OnRequestFail.Broadcast(EApexRequestType::Ping, Request, APEXFailedRequest);
    OnStaticRequestFail.Broadcast(EApexRequestType::Ping, Request);
}

void UApexAPI::Login(const FApexLoginInfo& InLoginInfo, bool bShouldLogout)
{
    if (bShouldLogout && CurrentActiveLogin.IsSet())
    {
        Logout();
    }

    UVaRestRequestJSON* Request = VaRestSubsystem->ConstructVaRestRequestExt(EVaRestRequestVerb::POST, EVaRestRequestContentType::json);
    FString RequestURL = URL + "/login";
    Request->SetURL(RequestURL);

    UVaRestJsonObject* LoginJson = VaRestSubsystem->ConstructVaRestJsonObject();
    LoginJson->SetField("login", VaRestSubsystem->ConstructJsonValueString(InLoginInfo.Username));
    LoginJson->SetField("password", VaRestSubsystem->ConstructJsonValueString(InLoginInfo.Password));

    Request->SetRequestObject(LoginJson);

    Request->OnStaticRequestComplete.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnLoginComplete(Request); });
    Request->OnStaticRequestFail.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnLoginFail(Request); });

    Request->ExecuteProcessRequest();
}

void UApexAPI::OnLoginComplete(const UVaRestRequestJSON* Request)
{
    // We have to handle errors that show up as valid messages.
    bool FailedResponse = true;
    if (Request->GetResponseCode() == 200)
    {
        if (!Request->GetResponseObject()->GetRootObject()->HasField("Error"))
        {
            FailedResponse = false;
            HandleLogin(Request->GetResponseObject());
        }
    }

    if (FailedResponse)
    {
        OnLoginFail(Request);
    }
}

void UApexAPI::OnLoginFail(const UVaRestRequestJSON* Request)
{
    LogAPEX("Login failed with response code %i", Request->GetResponseCode());

    UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
    FAPEXRequestFailed APEXFailedRequest;
    
    APEXFailedRequest.FromJsonObject(ResponseObject->GetRootObject());

    OnRequestFail.Broadcast(EApexRequestType::Login, Request, APEXFailedRequest);
    OnStaticRequestFail.Broadcast(EApexRequestType::Login, Request);
}


void UApexAPI::HandleLogin(UVaRestJsonObject* LoginJsonObject)
{
    LogAPEX("Login Completed!");
    if (CurrentActiveLogin.IsSet())
    {
        WarnAPEX("Replacing the current login.");
        CurrentActiveLogin.Invalidate();
    }

    CurrentActiveLogin.FromJsonObject(LoginJsonObject->GetRootObject());

    VerifyModuleAcess();
}

void UApexAPI::Logout()
{
    const UVaRestRequestJSON* DummyRequestJSON = VaRestSubsystem->ConstructVaRestRequest();
    if (CurrentActiveLogin.IsSet())
    {
        // TODO MGruber - Uncomment once the logout endpoint is added to the v2 of the platform API.
        /*UVaRestRequestJSON* Request = VaRestSubsystem->ConstructVaRestRequestExt(EVaRestRequestVerb::POST, EVaRestRequestContentType::json);
        FString RequestURL = URL + "/logout";
        Request->SetURL(RequestURL);

        UVaRestJsonObject* LogoutJson = VaRestSubsystem->ConstructVaRestJsonObject();
        LogoutJson->SetField("id", VaRestSubsystem->ConstructJsonValueNumber(CurrentActiveLogin.ID));

        Request->SetRequestObject(LogoutJson);

        Request->OnStaticRequestComplete.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnLogoutComplete(Request); });
        Request->OnStaticRequestFail.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnLogoutFail(Request); });

        Request->ExecuteProcessRequest();*/

        CurrentActiveLogin.Invalidate();

        OnRequestComplete.Broadcast(EApexRequestType::Logout, DummyRequestJSON);
        OnStaticRequestComplete.Broadcast(EApexRequestType::Logout, DummyRequestJSON);
    }
    else
    {
        FAPEXRequestFailed APEXFailedRequest;
        APEXFailedRequest.Error = true;
        APEXFailedRequest.Message = "No user currently logged in.";

        UVaRestJsonObject* ResponseObject = DummyRequestJSON->GetResponseObject();
        ResponseObject->SetBoolField("Error", APEXFailedRequest.Error);
        ResponseObject->SetStringField("Message", APEXFailedRequest.Message);

        OnRequestFail.Broadcast(EApexRequestType::Logout, DummyRequestJSON, APEXFailedRequest);
        OnStaticRequestFail.Broadcast(EApexRequestType::Logout, DummyRequestJSON);
    }
}

void UApexAPI::OnLogoutComplete(const UVaRestRequestJSON* Request)
{
    bool FailedResponse = true;
    if (Request->GetResponseCode() == 200)
    {
        if (!Request->GetResponseObject()->GetRootObject()->HasField("Error"))
        {
            OnRequestComplete.Broadcast(EApexRequestType::Logout, Request);
            OnStaticRequestComplete.Broadcast(EApexRequestType::Logout, Request);
        }
    }

    if (FailedResponse)
    {
        OnLogoutFail(Request);
    }
}

void UApexAPI::OnLogoutFail(const UVaRestRequestJSON* Request)
{
    LogAPEX("Logout failed with response code %i", Request->GetResponseCode());

    UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
    FAPEXRequestFailed APEXFailedRequest;

    APEXFailedRequest.FromJsonObject(ResponseObject->GetRootObject());

    OnRequestFail.Broadcast(EApexRequestType::Logout, Request, APEXFailedRequest);
    OnStaticRequestFail.Broadcast(EApexRequestType::Logout, Request);
}

void UApexAPI::VerifyModuleAcess()
{
    UVaRestRequestJSON* Request = VaRestSubsystem->ConstructVaRestRequestExt(EVaRestRequestVerb::GET, EVaRestRequestContentType::none);

    FStringFormatOrderedArguments FormatArguments;
    FormatArguments.Add(FStringFormatArg(URL));
    FormatArguments.Add(FStringFormatArg(CurrentActiveLogin.ID));
    FormatArguments.Add(FStringFormatArg(LoadedModuleId));
    FString AccessURL = FString::Format(TEXT("{0}/access/user/{1}/module/{2}"), FormatArguments);
    Request->SetURL(AccessURL);
    Request->SetHeader("Authorization: Bearer", CurrentActiveLogin.SessionToken);

    Request->OnStaticRequestComplete.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnUserVerificationComplete(Request); });
    Request->OnStaticRequestFail.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnUserVerificationFail(Request); });

    Request->ExecuteProcessRequest();
}

void UApexAPI::OnUserVerificationComplete(const UVaRestRequestJSON* Request)
{
    bool FailedResponse = true;
    if (Request->GetResponseCode() == 200)
    {
        UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
        if (!ResponseObject->GetRootObject()->HasField("Error"))
        {
            LogAPEX("User Verification Completed!");

            if (ResponseObject->HasField("access"))
            {
                FailedResponse = !ResponseObject->GetBoolField("access");
            }

            if (FailedResponse == false)
            {
                OnRequestComplete.Broadcast(EApexRequestType::Login, Request);
                OnStaticRequestComplete.Broadcast(EApexRequestType::Login, Request);
            }
        }
    }

    if (FailedResponse)
    {
        OnUserVerificationFail(Request);
    }
}

void UApexAPI::OnUserVerificationFail(const UVaRestRequestJSON* Request)
{
    LogAPEX("User module access verification failed with response code %i", Request->GetResponseCode());

    UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
    FAPEXRequestFailed APEXFailedRequest;
    if (Request->GetResponseCode() == 200)
    {
        if (ResponseObject->GetRootObject()->HasField("access"))
        {
            APEXFailedRequest.Error = true;
            APEXFailedRequest.Message = "User does not have access to this module.";
        }
        else
        {
            APEXFailedRequest.FromJsonObject(ResponseObject->GetRootObject());
        }
    }

    OnRequestFail.Broadcast(EApexRequestType::Login, Request, APEXFailedRequest);
    OnStaticRequestFail.Broadcast(EApexRequestType::Login, Request);
}

bool UApexAPI::JoinSession(FString InScenarioId, const FXAPIExtension& InContextExtension)
{
    if (!CurrentActiveLogin.IsSet())
    {
        ErrorAPEX("Cannot join session with no active login.");
        return false;
    }

    if (!InScenarioId.IsEmpty())
    {
        ScenarioId = InScenarioId;
    }
    else
    {
        ScenarioId = TEXT("<Unavailable>");
    }

    if (SessionInProgress == true)
    {
        ErrorAPEX("Session is already in progress. The previous session didn't complete or a new session was started during an active session.");
    }

    CurrentSessionGuid = FGuid::NewGuid();

    // Finish filling this out
    FXAPIStatement Statement;
    FXAPIAgent Actor;
    Actor.MBox = CurrentActiveLogin.Email;
    Actor.Name = FString::Printf(TEXT("%s %s"), *CurrentActiveLogin.FirstName, *CurrentActiveLogin.LastName);

    FXAPIVerb Verb;
    Verb.ID = Apex::Verbs::JOINED_SESSION;
    Verb.Display.Add("en", "Joined Session");

    FXAPIActivity Activity;
    FStringFormatOrderedArguments IdFormat({ LoadedModuleId, ScenarioId });
    Activity.ID = FString::Format(TEXT("https://pixovr.com/xapi/objects/{0}/{1}"), IdFormat);

    FXAPIContext Context;
    Context.Registration = CurrentSessionGuid;
    Context.Revision = ModuleVersion;
    Context.Platform = Platform;

    FXAPIExtension ContextExtension;
    if (!InContextExtension.IsEmpty())
    {
        ContextExtension.Clone(InContextExtension);
    }

    Context.Extensions = ContextExtension;
    Context.Extensions.AddCustomString(Apex::Extensions::MODULE_IDS, FString::FromInt(LoadedModuleId));
    Context.Extensions.AddString("device_id", DeviceId);
    Context.Extensions.AddString("device_model", DeviceModel);

    Statement.Actor = Actor;
    Statement.Verb = Verb;
    Statement.Target = Activity;
    Statement.Context = Context;

    FJoinSessionData JoinData;
    JoinData.DeviceId = DeviceId;
    JoinData.IpAddress = "0.0.0.0"; // We should get rid of this as there isn't a good way to get external IP addresses
    JoinData.ModuleId = LoadedModuleId;
    JoinData.Uuid = CurrentSessionGuid.ToString();
    JoinData.EventType = Apex::EventTypes::PIXOVR_SESSION_JOINED;
    JoinData.JsonData = Statement;

    UVaRestRequestJSON* Request = VaRestSubsystem->ConstructVaRestRequestExt(EVaRestRequestVerb::POST, EVaRestRequestContentType::json);
    FString RequestURL = URL + "/event";
    Request->SetURL(RequestURL);
    Request->SetHeader("Authorization: Bearer", CurrentActiveLogin.SessionToken);

    UVaRestJsonObject* JoinSessionJson = VaRestSubsystem->ConstructVaRestJsonObject();
    JoinSessionJson->SetRootObject(JoinData.ToJsonObject());

    Request->SetRequestObject(JoinSessionJson);

    Request->OnStaticRequestComplete.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnJoinSessionComplete(Request); });
    Request->OnStaticRequestFail.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnJoinSessionFail(Request); });

    Request->ExecuteProcessRequest();

    return true;
}

void UApexAPI::OnJoinSessionComplete(const UVaRestRequestJSON* Request)
{
    LogAPEX("Session was successfully joined.");
    SessionInProgress = true;
    OnRequestComplete.Broadcast(EApexRequestType::JoinSession, Request);
    OnStaticRequestComplete.Broadcast(EApexRequestType::JoinSession, Request);
}

void UApexAPI::OnJoinSessionFail(const UVaRestRequestJSON* Request)
{
    LogAPEX("Failed to join session. %d", Request->GetResponseCode());
    SessionInProgress = false;
    CurrentSessionGuid.Invalidate();

    UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
    FAPEXRequestFailed APEXFailedRequest;

    APEXFailedRequest.FromJsonObject(ResponseObject->GetRootObject());

    OnRequestFail.Broadcast(EApexRequestType::JoinSession, Request, APEXFailedRequest);
    OnStaticRequestFail.Broadcast(EApexRequestType::JoinSession, Request);
}

bool UApexAPI::CompleteSession(const FSessionData& InSessionData)
{
    if (!CurrentActiveLogin.IsSet())
    {
        ErrorAPEX("Cannot complete session with no active login.");
        return false;
    }

    if (SessionInProgress == false)
    {
        ErrorAPEX("No current session in progress.");
        return false;
    }

    // Create our actor
    FXAPIAgent Actor;
    Actor.MBox = CurrentActiveLogin.Email;
    Actor.Name = FString::Printf(TEXT("%s %s"), *CurrentActiveLogin.FirstName, *CurrentActiveLogin.LastName);

    // Create our verb
    FXAPIVerb Verb;
    Verb.ID = Apex::Verbs::COMPLETED_SESSION;
    Verb.Display.Add("en", "Completed Session");

    // Create the session activity
    FXAPIActivity Activity;
    Activity.ID = FString::Printf(TEXT("https://pixovr.com/xapi/objects/%d/%s"), LoadedModuleId, *ScenarioId);

    // Create our context
    FXAPIContext Context;
    Context.Registration = CurrentSessionGuid;
    Context.Revision = ModuleVersion;
    Context.Platform = Platform;

    // Build an extension for the context
    Context.Extensions = InSessionData.AdditionalContextData;
    Context.Extensions.AddCustomString(Apex::Extensions::MODULE_IDS, FString::FromInt(LoadedModuleId));
    Context.Extensions.AddString("device_id", DeviceId);
    Context.Extensions.AddString("device_model", DeviceModel);

    // Create our results
    FXAPIResult Result;
    Result.Completion = InSessionData.Complete;
    Result.Success = InSessionData.Success;
    // Add score to the results
    Result.Score.Min = InSessionData.ScoreMin;
    Result.Score.Max = InSessionData.ScoreMax;
    Result.Score.Raw = InSessionData.Score;
    Result.Score.Scaled = InSessionData.ScoreScaled;
    Result.Duration = FTimespan::FromSeconds(InSessionData.Duration);
    Result.Extensions = InSessionData.AdditionalResultData;

    // Create our statement and add the pieces
    FXAPIStatement Statement;
    Statement.Actor = Actor;
    Statement.Verb = Verb;
    Statement.Target = Activity;
    Statement.Context = Context;
    Statement.Result = Result;

    FCompleteSessionData SessionData;
    SessionData.DeviceId = DeviceId;
    SessionData.ModuleId = LoadedModuleId;
    SessionData.Uuid = CurrentSessionGuid.ToString();
    SessionData.EventType = Apex::EventTypes::PIXOVR_SESSION_COMPLETE;
    SessionData.JsonData = Statement;
    SessionData.SessionData = InSessionData;

    // Now we create our request and send it!
    UVaRestRequestJSON* Request = VaRestSubsystem->ConstructVaRestRequestExt(EVaRestRequestVerb::POST, EVaRestRequestContentType::json);
    FString RequestURL = URL + "/event";
    Request->SetURL(RequestURL);
    Request->SetHeader("Authorization: Bearer", CurrentActiveLogin.SessionToken);

    UVaRestJsonObject* CompleteSessionJson = VaRestSubsystem->ConstructVaRestJsonObject();
    CompleteSessionJson->SetRootObject(SessionData.ToJsonObject());

    Request->SetRequestObject(CompleteSessionJson);

    Request->OnStaticRequestComplete.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnCompleteSessionComplete(Request); });
    Request->OnStaticRequestFail.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnCompleteSessionFail(Request); });

    Request->ExecuteProcessRequest();

    return true;
}

void UApexAPI::OnCompleteSessionComplete(const UVaRestRequestJSON* Request)
{
    LogAPEX("Session complete!");
    SessionInProgress = false;
    CurrentSessionGuid.Invalidate();
    OnRequestComplete.Broadcast(EApexRequestType::CompleteSession, Request);
    OnStaticRequestComplete.Broadcast(EApexRequestType::CompleteSession, Request);
}

void UApexAPI::OnCompleteSessionFail(const UVaRestRequestJSON* Request)
{
    WarnAPEX("Failed to complete session. %d", Request->GetResponseCode());

    UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
    FAPEXRequestFailed APEXFailedRequest;

    APEXFailedRequest.FromJsonObject(ResponseObject->GetRootObject());

    OnRequestFail.Broadcast(EApexRequestType::CompleteSession, Request, APEXFailedRequest);
    OnStaticRequestFail.Broadcast(EApexRequestType::CompleteSession, Request);
}

bool UApexAPI::SendSessionEvent(const FXAPIStatement& InStatement)
{
    FXAPIStatement EventStatement = InStatement;
    if (!CurrentActiveLogin.IsSet())
    {
        ErrorAPEX("Cannot complete session with no active login.");
        return false;
    }

    if (SessionInProgress == false)
    {
        ErrorAPEX("No current session in progress.");
        return false;
    }

    if (EventStatement.IsEmpty())
    {
        ErrorAPEX("No event data to send.");
        return false;
    }

    if (!EventStatement.Actor.IsEmpty())
    {
        WarnAPEX("Actor data should not be filled out.");
    }

    if (EventStatement.Verb.IsEmpty())
    {
        ErrorAPEX("Verb missing from eventStatement.");
        return false;
    }

    if (EventStatement.Verb.ID.IsEmpty())
    {
        ErrorAPEX("Verb ID missing from eventStatement.");
        return false;
    }

    if (EventStatement.Target.IsEmpty())
    {
        ErrorAPEX("Object (target) missing from eventStatement.");
        return false;
    }

    EventStatement.Actor.MBox = CurrentActiveLogin.Email;
    EventStatement.Actor.Name = FString::Printf(TEXT("%s %s"), *CurrentActiveLogin.FirstName, *CurrentActiveLogin.LastName);

    EventStatement.Context.Registration = CurrentSessionGuid;
    EventStatement.Context.Revision = ModuleVersion;
    EventStatement.Context.Platform = Platform;

    EventStatement.Context.Extensions.AddString("device_id", DeviceId);
    EventStatement.Context.Extensions.AddString("device_model", DeviceModel);

    FSessionEventData Event;
    Event.DeviceId = DeviceId;
    Event.ModuleId = LoadedModuleId;
    Event.Uuid = CurrentSessionGuid.ToString();
    Event.EventType = Apex::EventTypes::PIXOVR_SESSION_EVENT;
    Event.JsonData = EventStatement;

    // Now we create our request and send it!
    UVaRestRequestJSON* Request = VaRestSubsystem->ConstructVaRestRequestExt(EVaRestRequestVerb::POST, EVaRestRequestContentType::json);
    FString RequestURL = URL + "/event";
    Request->SetURL(RequestURL);
    Request->SetHeader("Authorization: Bearer", CurrentActiveLogin.SessionToken);

    UVaRestJsonObject* CompleteSessionJson = VaRestSubsystem->ConstructVaRestJsonObject();
    CompleteSessionJson->SetRootObject(Event.ToJsonObject());

    Request->SetRequestObject(CompleteSessionJson);

    Request->OnStaticRequestComplete.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnSendSessionEventComplete(Request); });
    Request->OnStaticRequestFail.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnSendSessionEventFail(Request); });

    Request->ExecuteProcessRequest();

    return true;
}

void UApexAPI::OnSendSessionEventComplete(const UVaRestRequestJSON* Request)
{
    LogAPEX("Session event was sent successfully.");
    OnRequestComplete.Broadcast(EApexRequestType::SessionEvent, Request);
    OnStaticRequestComplete.Broadcast(EApexRequestType::SessionEvent, Request);
}

void UApexAPI::OnSendSessionEventFail(const UVaRestRequestJSON* Request)
{
    WarnAPEX("Failed to send session event. %d", Request->GetResponseCode());

    UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
    FAPEXRequestFailed APEXFailedRequest;

    APEXFailedRequest.FromJsonObject(ResponseObject->GetRootObject());

    OnRequestFail.Broadcast(EApexRequestType::SessionEvent, Request, APEXFailedRequest);
    OnStaticRequestFail.Broadcast(EApexRequestType::SessionEvent, Request);
}

bool UApexAPI::FetchTopTen(FString Scenario, int OrgUnit)
{
    return FetchTopTen(-1, -1.f, Scenario, OrgUnit);
}

bool UApexAPI::FetchTopTenByCurrentUser(FString Scenario)
{
    return FetchTopTen(CurrentActiveLogin.ID, -1.f, Scenario, CurrentActiveLogin.OrgUnit);
}

bool UApexAPI::FetchTopTenByUser(int UserId, FString Scenario, int OrgUnit)
{
    return FetchTopTen(UserId, -1.f, Scenario, OrgUnit);
}

bool UApexAPI::FetchTopTenByTimeRange(float TimePriorToToday, FString Scenario, int OrgUnit)
{
    return FetchTopTen(-1, TimePriorToToday, Scenario, OrgUnit);
}

bool UApexAPI::FetchTopTen(int UserId, float TimePriorToToday, FString Scenario, int OrgUnit)
{
    if (!CurrentActiveLogin.IsSet())
    {
        ErrorAPEX("Cannot complete session with no active login.");
        return false;
    }

    UVaRestRequestJSON* Request = VaRestSubsystem->ConstructVaRestRequestExt(EVaRestRequestVerb::GET, EVaRestRequestContentType::none);
    FString URLFormat = "{url}/analytics/top10/module/{moduleId}";
    FStringFormatNamedArguments FormatArguments;

    if (UserId >= 0)
    {
        URLFormat += "/user/{userId}";
    }

    FString DataQueryFormat = "?";
    
    if (!Scenario.IsEmpty())
    {
        DataQueryFormat += "scenario={moduleId}/{scenario}&";
    }

    if (TimePriorToToday > 0.0f)
    {
        DataQueryFormat += "dateRange={dateRange}&";
    }

    if (OrgUnit >= 0)
    {
        DataQueryFormat += "orgUnit={orgUnit}&";
    }

    if (DataQueryFormat.EndsWith("&"))
    {
        DataQueryFormat.RemoveFromEnd("&");
    }

    URLFormat += DataQueryFormat;

    FormatArguments.Add("url", FStringFormatArg(URL));
    FormatArguments.Add("moduleId", FStringFormatArg(LoadedModuleId));
    FormatArguments.Add("userId", FStringFormatArg(UserId));
    FormatArguments.Add("dateRange", FStringFormatArg(TimePriorToToday));
    FormatArguments.Add("scenario", FStringFormatArg(Scenario));
    FormatArguments.Add("orgUnit", FStringFormatArg(OrgUnit));
    FString AnalyticsURL = FString::Format(*URLFormat, FormatArguments);
    Request->SetURL(AnalyticsURL);
    Request->SetHeader("Authorization: Bearer", CurrentActiveLogin.SessionToken);

    Request->OnStaticRequestComplete.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnTopTenComplete(Request); });
    Request->OnStaticRequestFail.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnTopTenFail(Request); });

    Request->ExecuteProcessRequest();

    return true;
}

void UApexAPI::OnTopTenComplete(const UVaRestRequestJSON* Request)
{
    bool FailedResponse = true;
    if (Request->GetResponseCode() == 200)
    {
        UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
        if (ResponseObject->HasField("Error"))
        {
            if (!ResponseObject->GetBoolField("Error"))
            {
                LogAPEX("Successfully fetched top ten data!");

                // It's not a failed response!
                FailedResponse = false;

                FTopTenData FetchedTopTenData;
                FetchedTopTenData.FromJsonObject(ResponseObject->GetRootObject());

                OnFetchTopTenComplete.Broadcast(Request, FetchedTopTenData);
                OnStaticFetchTopTenComplete.Broadcast(Request, FetchedTopTenData);
            }
        }
    }

    if (FailedResponse)
    {
        OnTopTenFail(Request);
    }
}

void UApexAPI::OnTopTenFail(const UVaRestRequestJSON* Request)
{
    LogAPEX("Fetch Top Ten failed with response code %i", Request->GetResponseCode());

    UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
    FAPEXRequestFailed APEXFailedRequest;

    APEXFailedRequest.FromJsonObject(ResponseObject->GetRootObject());

    OnFetchTopTenFail.Broadcast(Request, APEXFailedRequest);
    OnStaticFetchTopTenFail.Broadcast(Request, APEXFailedRequest);
}

#undef FatalAPEX
#undef ErrorAPEX
#undef WarnAPEX
#undef LogAPEX
