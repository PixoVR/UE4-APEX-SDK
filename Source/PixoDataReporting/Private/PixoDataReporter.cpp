// Copyright 2022 PixoVR Corp. All Rights Reserved.


#include "PixoDataReporter.h"
#include "PixoAuth.h"
#include "VaRest/Public/VaRestSubsystem.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "GenericPlatform/GenericPlatformHttp.h"

#include "PixoConfigSettings.h"
#include "PixoConfig.h"

DEFINE_LOG_CATEGORY_STATIC(LogPDataReporter, Log, All);

#define Log(pmt, ...) UE_LOG(LogPDataReporter, Log, TEXT(pmt), ##__VA_ARGS__)
#define Warn(pmt, ...) UE_LOG(LogPDataReporter, Warning, TEXT(pmt), ##__VA_ARGS__)
#define Error(pmt, ...) UE_LOG(LogPDataReporter, Error, TEXT(pmt), ##__VA_ARGS__)
#define Fatal(pmt, ...) UE_LOG(LogPDataReporter, Fatal, TEXT(pmt), ##__VA_ARGS__)

UPDataReporter::UPDataReporter()
    : UEngineSubsystem(),
    PlatformSettings(nullptr)
{
}

void UPDataReporter::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (FPixoConfigModule::IsAvailable())
    {
        PlatformSettings = FPixoConfigModule::Get().GetSettings();
        SetupSessionManagerSettings(PlatformSettings);
        PlatformSettings->OnStaticSettingsChanged.AddLambda([&](UPixoConfigSettings* UpdatedSettings) -> void {
            SetupSessionManagerSettings(UpdatedSettings);
        });
    }

    VaRestSubsystem = GEngine->GetEngineSubsystem<UVaRestSubsystem>();
    AuthorizationSubsystem = GEngine->GetEngineSubsystem<UPAuthorizer>();

    check(AuthorizationSubsystem);
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

void UPDataReporter::Deinitialize()
{
}

void UPDataReporter::OnAuthComplete(const FUserSessionData& AuthorizedUser, const class UVaRestRequestJSON* Request)
{
    CurrentUser = AuthorizedUser;

    // We have to wait for the user to be set before the session manager can be used.
    OnDataReporterReady.Broadcast();
    OnStaticDataReporterReady.Broadcast();
}

void UPDataReporter::SetupSessionManagerSettings(UPixoConfigSettings* InPlatformSettings)
{
    URL = PlatformSettings->PlatformURL;
    ModuleId = PlatformSettings->ModuleId;
    ModuleVersion = PlatformSettings->ModuleVersion;
}

void UPDataReporter::OnLogoutComplete()
{
    CurrentUser.Invalidate();
}

bool UPDataReporter::JoinSession(FString InScenarioId, const FXAPIExtension& InContextExtension, bool bIsMultiplayer)
{
    if (!AuthorizationSubsystem->IsLoggedIn())
    {
        Error("Cannot join session with no active login.");
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
        Error("Session is already in progress. The previous session didn't complete or a new session was started during an active session.");
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

    if (bIsMultiplayer)
    {
        Context.Extensions.AddString("multiplayer", "true");
    }

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

void UPDataReporter::OnJoinSessionComplete(const UVaRestRequestJSON* Request)
{
    Log("Session was successfully joined.");
    SessionInProgress = true;
    OnRequestComplete.Broadcast(EPixoDataReportType::JoinSession, Request);
    OnStaticRequestComplete.Broadcast(EPixoDataReportType::JoinSession, Request);
}

void UPDataReporter::OnJoinSessionFail(const UVaRestRequestJSON* Request)
{
    Log("Failed to join session. %d", Request->GetResponseCode());
    SessionInProgress = false;
    CurrentSessionGuid.Invalidate();

    UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
    FPRequestFailed FailedRequest;

    FailedRequest.FromJsonObject(ResponseObject->GetRootObject());

    OnRequestFail.Broadcast(EPixoDataReportType::JoinSession, FailedRequest, Request);
    OnStaticRequestFail.Broadcast(EPixoDataReportType::JoinSession, FailedRequest, Request);
}

bool UPDataReporter::CompleteSession(const FSessionData& InSessionData)
{
    if (!CurrentUser.IsSet())
    {
        Error("Cannot complete session with no active login.");
        return false;
    }

    if (SessionInProgress == false)
    {
        Error("No current session in progress.");
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

void UPDataReporter::OnCompleteSessionComplete(const UVaRestRequestJSON* Request)
{
    Log("Session complete!");
    SessionInProgress = false;
    CurrentSessionGuid.Invalidate();
    OnRequestComplete.Broadcast(EPixoDataReportType::CompleteSession, Request);
    OnStaticRequestComplete.Broadcast(EPixoDataReportType::CompleteSession, Request);
}

void UPDataReporter::OnCompleteSessionFail(const UVaRestRequestJSON* Request)
{
    Warn("Failed to complete session. %d", Request->GetResponseCode());

    UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
    FPRequestFailed FailedRequest;

    FailedRequest.FromJsonObject(ResponseObject->GetRootObject());

    OnRequestFail.Broadcast(EPixoDataReportType::CompleteSession, FailedRequest, Request);
    OnStaticRequestFail.Broadcast(EPixoDataReportType::CompleteSession, FailedRequest, Request);
}

bool UPDataReporter::SendSessionEvent(const FXAPIStatement& InStatement)
{
    FXAPIStatement EventStatement = InStatement;
    if (!CurrentUser.IsSet())
    {
        Error("Cannot complete session with no active login.");
        return false;
    }

    if (SessionInProgress == false)
    {
        Error("No current session in progress.");
        return false;
    }

    if (EventStatement.IsEmpty())
    {
        Error("No event data to send.");
        return false;
    }

    if (EventStatement.Verb.IsEmpty())
    {
        Error("Verb missing from eventStatement.");
        return false;
    }

    if (EventStatement.Verb.ID.IsEmpty())
    {
        Error("Verb ID missing from eventStatement.");
        return false;
    }

    if (EventStatement.Target.IsEmpty())
    {
        Error("Object (target) missing from eventStatement.");
        return false;
    }

    if (EventStatement.Actor.IsEmpty() || EventStatement.Actor.Name.IsEmpty())
    {
        Log("Actor is empty or missing a name, will default to the current user.");
        EventStatement.Actor.MBox = CurrentUser.Email;
        EventStatement.Actor.Name = FString::Printf(TEXT("%s %s"), *CurrentUser.FirstName, *CurrentUser.LastName);
    }

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

void UPDataReporter::OnSendSessionEventComplete(const UVaRestRequestJSON* Request)
{
    Log("Session event was sent successfully.");
    OnRequestComplete.Broadcast(EPixoDataReportType::SessionEvent, Request);
    OnStaticRequestComplete.Broadcast(EPixoDataReportType::SessionEvent, Request);
}

void UPDataReporter::OnSendSessionEventFail(const UVaRestRequestJSON* Request)
{
    Warn("Failed to send session event. %d", Request->GetResponseCode());

    UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
    FPRequestFailed FailedRequest;

    FailedRequest.FromJsonObject(ResponseObject->GetRootObject());

    OnRequestFail.Broadcast(EPixoDataReportType::SessionEvent, FailedRequest, Request);
    OnStaticRequestFail.Broadcast(EPixoDataReportType::SessionEvent, FailedRequest, Request);
}

bool UPDataReporter::FetchTopTen(FString Scenario, int OrgUnit)
{
    return FetchTopTen(-1, -1.f, Scenario, OrgUnit);
}

bool UPDataReporter::FetchTopTenByUser(FString Scenario, int UserId, int OrgUnit)
{
    if (UserId <= 0 || OrgUnit <= 0)
    {
        UserId = CurrentUser.ID;
        OrgUnit = CurrentUser.OrgUnit;
    }
    return FetchTopTen(UserId, -1.f, Scenario, OrgUnit);
}

bool UPDataReporter::FetchTopTenByTimeRange(float TimePriorToToday, FString Scenario, int OrgUnit)
{
    return FetchTopTen(-1, TimePriorToToday, Scenario, OrgUnit);
}

bool UPDataReporter::FetchTopTen(int UserId, float TimePriorToToday, FString Scenario, int OrgUnit)
{
    if (!CurrentUser.IsSet())
    {
        Error("Cannot complete session with no active login.");
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

void UPDataReporter::OnTopTenComplete(const UVaRestRequestJSON* Request)
{
    bool FailedResponse = true;
    if (Request->GetResponseCode() == 200)
    {
        UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
        if (ResponseObject->HasField("Error"))
        {
            if (!ResponseObject->GetBoolField("Error"))
            {
                Log("Successfully fetched top ten data!");

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

void UPDataReporter::OnTopTenFail(const UVaRestRequestJSON* Request)
{
    Log("Fetch Top Ten failed with response code %i", Request->GetResponseCode());

    UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
    FPRequestFailed FailedRequest;

    FailedRequest.FromJsonObject(ResponseObject->GetRootObject());

    OnFetchTopTenFail.Broadcast(FailedRequest, Request);
    OnStaticFetchTopTenFail.Broadcast(FailedRequest, Request);
}

#undef Fatal
#undef Error
#undef Warn
#undef Log
