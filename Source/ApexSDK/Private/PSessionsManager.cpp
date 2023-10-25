// Copyright 2022 PixoVR Corp. All Rights Reserved.


#include "PSessionsManager.h"
#include "PPlatformAuth.h"
#include "VaRest/Public/VaRestSubsystem.h"
#include "ApexSDK.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "GenericPlatform/GenericPlatformHttp.h"

DEFINE_LOG_CATEGORY_STATIC(LogApexAPI, Log, All);

#define LogAPEX(pmt, ...) UE_LOG(LogApexAPI, Log, TEXT(pmt), ##__VA_ARGS__)
#define WarnAPEX(pmt, ...) UE_LOG(LogApexAPI, Warning, TEXT(pmt), ##__VA_ARGS__)
#define ErrorAPEX(pmt, ...) UE_LOG(LogApexAPI, Error, TEXT(pmt), ##__VA_ARGS__)
#define FatalAPEX(pmt, ...) UE_LOG(LogApexAPI, Fatal, TEXT(pmt), ##__VA_ARGS__)

UPSessionsManager::UPSessionsManager()
    : UEngineSubsystem()
{
}

void UPSessionsManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    VaRestSubsystem = GEngine->GetEngineSubsystem<UVaRestSubsystem>();
    AuthorizationSubsystem = GEngine->GetEngineSubsystem<UPPlatformAuth>();

    AuthorizationSubsystem->OnStaticAuthorizationCompleted.AddLambda(
        [&](const FUserSessionData& AuthorizedUser, const class UVaRestRequestJSON* Request) -> void { OnAuthComplete(AuthorizedUser, Request); }
    );

    AuthorizationSubsystem->OnStaticLogoutCompleted.AddLambda(
        [&]() -> void { OnLogoutComplete(); }
    );

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
}

void UPSessionsManager::OnAuthComplete(const FUserSessionData& AuthorizedUser, const class UVaRestRequestJSON* Request)
{
    CurrentUser = AuthorizedUser;
    URL = AuthorizationSubsystem->URL;
    ModuleId = AuthorizationSubsystem->ModuleId;
    ModuleVersion = AuthorizationSubsystem->ModuleVersion;
}

void UPSessionsManager::OnLogoutComplete()
{
    CurrentUser.Invalidate();
}

bool UPSessionsManager::JoinSession(FString InScenarioId, const FXAPIExtension& InContextExtension)
{
    if (!AuthorizationSubsystem->IsLoggedIn())
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
    Actor.MBox = CurrentUser.Email;
    Actor.Name = FString::Printf(TEXT("%s %s"), *CurrentUser.FirstName, *CurrentUser.LastName);

    FXAPIVerb Verb;
    Verb.ID = Apex::Verbs::JOINED_SESSION;
    Verb.Display.Add("en", "Joined Session");

    FXAPIActivity Activity;
    FStringFormatOrderedArguments IdFormat({ ModuleId, ScenarioId });
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
    Context.Extensions.AddCustomString(Apex::Extensions::MODULE_IDS, FString::FromInt(ModuleId));
    Context.Extensions.AddString("device_id", DeviceId);
    Context.Extensions.AddString("device_model", DeviceModel);

    Statement.Actor = Actor;
    Statement.Verb = Verb;
    Statement.Target = Activity;
    Statement.Context = Context;

    FJoinSessionData JoinData;
    JoinData.DeviceId = DeviceId;
    JoinData.IpAddress = "0.0.0.0"; // We should get rid of this as there isn't a good way to get external IP addresses
    JoinData.ModuleId = ModuleId;
    JoinData.Uuid = CurrentSessionGuid.ToString();
    JoinData.EventType = Apex::EventTypes::PIXOVR_SESSION_JOINED;
    JoinData.JsonData = Statement;

    UVaRestRequestJSON* Request = VaRestSubsystem->ConstructVaRestRequestExt(EVaRestRequestVerb::POST, EVaRestRequestContentType::json);
    FString RequestURL = URL + "/event";
    Request->SetURL(RequestURL);
    Request->SetHeader("Authorization: Bearer", CurrentUser.SessionToken);

    UVaRestJsonObject* JoinSessionJson = VaRestSubsystem->ConstructVaRestJsonObject();
    JoinSessionJson->SetRootObject(JoinData.ToJsonObject());

    Request->SetRequestObject(JoinSessionJson);

    Request->OnStaticRequestComplete.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnJoinSessionComplete(Request); });
    Request->OnStaticRequestFail.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnJoinSessionFail(Request); });

    Request->ExecuteProcessRequest();

    return true;
}

void UPSessionsManager::OnJoinSessionComplete(const UVaRestRequestJSON* Request)
{
    LogAPEX("Session was successfully joined.");
    SessionInProgress = true;
    OnRequestComplete.Broadcast(EApexRequestType::JoinSession, Request);
    OnStaticRequestComplete.Broadcast(EApexRequestType::JoinSession, Request);
}

void UPSessionsManager::OnJoinSessionFail(const UVaRestRequestJSON* Request)
{
    LogAPEX("Failed to join session. %d", Request->GetResponseCode());
    SessionInProgress = false;
    CurrentSessionGuid.Invalidate();

    UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
    FAPEXRequestFailed APEXFailedRequest;

    APEXFailedRequest.FromJsonObject(ResponseObject->GetRootObject());

    OnRequestFail.Broadcast(EApexRequestType::JoinSession, APEXFailedRequest, Request);
    OnStaticRequestFail.Broadcast(EApexRequestType::JoinSession, APEXFailedRequest, Request);
}

bool UPSessionsManager::CompleteSession(const FSessionData& InSessionData)
{
    if (!CurrentUser.IsSet())
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
    Actor.MBox = CurrentUser.Email;
    Actor.Name = FString::Printf(TEXT("%s %s"), *CurrentUser.FirstName, *CurrentUser.LastName);

    // Create our verb
    FXAPIVerb Verb;
    Verb.ID = Apex::Verbs::COMPLETED_SESSION;
    Verb.Display.Add("en", "Completed Session");

    // Create the session activity
    FXAPIActivity Activity;
    Activity.ID = FString::Printf(TEXT("https://pixovr.com/xapi/objects/%d/%s"), ModuleId, *ScenarioId);

    // Create our context
    FXAPIContext Context;
    Context.Registration = CurrentSessionGuid;
    Context.Revision = ModuleVersion;
    Context.Platform = Platform;

    // Build an extension for the context
    Context.Extensions = InSessionData.AdditionalContextData;
    Context.Extensions.AddCustomString(Apex::Extensions::MODULE_IDS, FString::FromInt(ModuleId));
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
    SessionData.ModuleId = ModuleId;
    SessionData.Uuid = CurrentSessionGuid.ToString();
    SessionData.EventType = Apex::EventTypes::PIXOVR_SESSION_COMPLETE;
    SessionData.JsonData = Statement;
    SessionData.SessionData = InSessionData;

    // Now we create our request and send it!
    UVaRestRequestJSON* Request = VaRestSubsystem->ConstructVaRestRequestExt(EVaRestRequestVerb::POST, EVaRestRequestContentType::json);
    FString RequestURL = URL + "/event";
    Request->SetURL(RequestURL);
    Request->SetHeader("Authorization: Bearer", CurrentUser.SessionToken);

    UVaRestJsonObject* CompleteSessionJson = VaRestSubsystem->ConstructVaRestJsonObject();
    CompleteSessionJson->SetRootObject(SessionData.ToJsonObject());

    Request->SetRequestObject(CompleteSessionJson);

    Request->OnStaticRequestComplete.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnCompleteSessionComplete(Request); });
    Request->OnStaticRequestFail.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnCompleteSessionFail(Request); });

    Request->ExecuteProcessRequest();

    return true;
}

void UPSessionsManager::OnCompleteSessionComplete(const UVaRestRequestJSON* Request)
{
    LogAPEX("Session complete!");
    SessionInProgress = false;
    CurrentSessionGuid.Invalidate();
    OnRequestComplete.Broadcast(EApexRequestType::CompleteSession, Request);
    OnStaticRequestComplete.Broadcast(EApexRequestType::CompleteSession, Request);
}

void UPSessionsManager::OnCompleteSessionFail(const UVaRestRequestJSON* Request)
{
    WarnAPEX("Failed to complete session. %d", Request->GetResponseCode());

    UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
    FAPEXRequestFailed APEXFailedRequest;

    APEXFailedRequest.FromJsonObject(ResponseObject->GetRootObject());

    OnRequestFail.Broadcast(EApexRequestType::CompleteSession, APEXFailedRequest, Request);
    OnStaticRequestFail.Broadcast(EApexRequestType::CompleteSession, APEXFailedRequest, Request);
}

bool UPSessionsManager::SendSessionEvent(const FXAPIStatement& InStatement)
{
    FXAPIStatement EventStatement = InStatement;
    if (!CurrentUser.IsSet())
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

    EventStatement.Actor.MBox = CurrentUser.Email;
    EventStatement.Actor.Name = FString::Printf(TEXT("%s %s"), *CurrentUser.FirstName, *CurrentUser.LastName);

    EventStatement.Context.Registration = CurrentSessionGuid;
    EventStatement.Context.Revision = ModuleVersion;
    EventStatement.Context.Platform = Platform;

    EventStatement.Context.Extensions.AddString("device_id", DeviceId);
    EventStatement.Context.Extensions.AddString("device_model", DeviceModel);

    FSessionEventData Event;
    Event.DeviceId = DeviceId;
    Event.ModuleId = ModuleId;
    Event.Uuid = CurrentSessionGuid.ToString();
    Event.EventType = Apex::EventTypes::PIXOVR_SESSION_EVENT;
    Event.JsonData = EventStatement;

    // Now we create our request and send it!
    UVaRestRequestJSON* Request = VaRestSubsystem->ConstructVaRestRequestExt(EVaRestRequestVerb::POST, EVaRestRequestContentType::json);
    FString RequestURL = URL + "/event";
    Request->SetURL(RequestURL);
    Request->SetHeader("Authorization: Bearer", CurrentUser.SessionToken);

    UVaRestJsonObject* CompleteSessionJson = VaRestSubsystem->ConstructVaRestJsonObject();
    CompleteSessionJson->SetRootObject(Event.ToJsonObject());

    Request->SetRequestObject(CompleteSessionJson);

    Request->OnStaticRequestComplete.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnSendSessionEventComplete(Request); });
    Request->OnStaticRequestFail.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnSendSessionEventFail(Request); });

    Request->ExecuteProcessRequest();

    return true;
}

void UPSessionsManager::OnSendSessionEventComplete(const UVaRestRequestJSON* Request)
{
    LogAPEX("Session event was sent successfully.");
    OnRequestComplete.Broadcast(EApexRequestType::SessionEvent, Request);
    OnStaticRequestComplete.Broadcast(EApexRequestType::SessionEvent, Request);
}

void UPSessionsManager::OnSendSessionEventFail(const UVaRestRequestJSON* Request)
{
    WarnAPEX("Failed to send session event. %d", Request->GetResponseCode());

    UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
    FAPEXRequestFailed APEXFailedRequest;

    APEXFailedRequest.FromJsonObject(ResponseObject->GetRootObject());

    OnRequestFail.Broadcast(EApexRequestType::SessionEvent, APEXFailedRequest, Request);
    OnStaticRequestFail.Broadcast(EApexRequestType::SessionEvent, APEXFailedRequest, Request);
}

bool UPSessionsManager::FetchTopTen(FString Scenario, int OrgUnit)
{
    return FetchTopTen(-1, -1.f, Scenario, OrgUnit);
}

bool UPSessionsManager::FetchTopTenByCurrentUser(FString Scenario)
{
    return FetchTopTen(CurrentUser.ID, -1.f, Scenario, CurrentUser.OrgUnit);
}

bool UPSessionsManager::FetchTopTenByUser(int UserId, FString Scenario, int OrgUnit)
{
    return FetchTopTen(UserId, -1.f, Scenario, OrgUnit);
}

bool UPSessionsManager::FetchTopTenByTimeRange(float TimePriorToToday, FString Scenario, int OrgUnit)
{
    return FetchTopTen(-1, TimePriorToToday, Scenario, OrgUnit);
}

bool UPSessionsManager::FetchTopTen(int UserId, float TimePriorToToday, FString Scenario, int OrgUnit)
{
    if (!CurrentUser.IsSet())
    {
        ErrorAPEX("Cannot complete session with no active login.");
        return false;
    }

    UVaRestRequestJSON* Request = VaRestSubsystem->ConstructVaRestRequestExt(EVaRestRequestVerb::GET, EVaRestRequestContentType::x_www_form_urlencoded_url);
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
    FormatArguments.Add("moduleId", FStringFormatArg(ModuleId));
    FormatArguments.Add("userId", FStringFormatArg(UserId));
    FormatArguments.Add("dateRange", FStringFormatArg(TimePriorToToday));
    FormatArguments.Add("scenario", FStringFormatArg(FGenericPlatformHttp::UrlEncode(Scenario)));
    FormatArguments.Add("orgUnit", FStringFormatArg(OrgUnit));
    FString AnalyticsURL = FString::Format(*URLFormat, FormatArguments);
    Request->SetURL(AnalyticsURL);
    Request->SetHeader("Authorization: Bearer", CurrentUser.SessionToken);

    Request->OnStaticRequestComplete.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnTopTenComplete(Request); });
    Request->OnStaticRequestFail.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnTopTenFail(Request); });

    Request->ExecuteProcessRequest();

    return true;
}

void UPSessionsManager::OnTopTenComplete(const UVaRestRequestJSON* Request)
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

                OnFetchTopTenComplete.Broadcast(FetchedTopTenData, Request);
                OnStaticFetchTopTenComplete.Broadcast(FetchedTopTenData, Request);
            }
        }
    }

    if (FailedResponse)
    {
        OnTopTenFail(Request);
    }
}

void UPSessionsManager::OnTopTenFail(const UVaRestRequestJSON* Request)
{
    LogAPEX("Fetch Top Ten failed with response code %i", Request->GetResponseCode());

    UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
    FAPEXRequestFailed APEXFailedRequest;

    APEXFailedRequest.FromJsonObject(ResponseObject->GetRootObject());

    OnFetchTopTenFail.Broadcast(APEXFailedRequest, Request);
    OnStaticFetchTopTenFail.Broadcast(APEXFailedRequest, Request);
}

#undef FatalAPEX
#undef ErrorAPEX
#undef WarnAPEX
#undef LogAPEX
