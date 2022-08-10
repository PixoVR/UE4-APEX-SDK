// Fill out your copyright notice in the Description page of Project Settings.


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

void UApexAPI::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    VaRestSubsystem = GEngine->GetEngineSubsystem<UVaRestSubsystem>();

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

        if (!URL.Contains("https://"))
        {
            if (URL.Contains("http://"))
            {
                URL = URL.Replace(TEXT("http://"), TEXT(""));
            }

            URL.InsertAt(0, "https://");
        }

        LoadedModuleId = ApexSettings->ModuleID;

        ModuleVersion = FString::Printf(TEXT("%i.%.2i.%.2i"), ApexSettings->MajorVerison, ApexSettings->MinorVersion, ApexSettings->PatchVersion);
        LogAPEX("Launching module %d version %s", LoadedModuleId, *ModuleVersion);
    }

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

    BIND_REQUEST_DELEGATE(Ping);
    BIND_REQUEST_DELEGATE(Login);
    BIND_REQUEST_DELEGATE(JoinSession);
    BIND_REQUEST_DELEGATE(CompleteSession);
    BIND_REQUEST_DELEGATE(SendSessionEvent);
}

void UApexAPI::Ping()
{
    UVaRestRequestJSON* Request = VaRestSubsystem->ConstructVaRestRequestExt(EVaRestRequestVerb::GET, EVaRestRequestContentType::none);
    FString PingURL = URL + "/ping";
    Request->SetURL(PingURL);

    // Add call backs from our lovely generated delegates
    Request->OnRequestComplete.Add(OnPingCompleteDelegate);
    Request->OnRequestFail.Add(OnPingFailDelegate);

    Request->ExecuteProcessRequest();
}

void UApexAPI::OnPingComplete(UVaRestRequestJSON* Request)
{
    LogAPEX("On Ping Completed!");
    OnRequestComplete.Broadcast(ApexRequestType::Ping, Request);
    OnStaticRequestComplete.Broadcast(ApexRequestType::Ping, Request);
}

void UApexAPI::OnPingFail(UVaRestRequestJSON* Request)
{
    LogAPEX("On Ping Failed!");
    OnRequestFail.Broadcast(ApexRequestType::Ping, Request);
    OnStaticRequestFail.Broadcast(ApexRequestType::Ping, Request);
}

void UApexAPI::Login(const FApexLoginInfo& InLoginInfo)
{
    UVaRestRequestJSON* Request = VaRestSubsystem->ConstructVaRestRequestExt(EVaRestRequestVerb::POST, EVaRestRequestContentType::json);
    FString PingURL = URL + "/login";
    Request->SetURL(PingURL);

    UVaRestJsonObject* LoginJson = VaRestSubsystem->ConstructVaRestJsonObject();
    LoginJson->SetField("login", VaRestSubsystem->ConstructJsonValueString(InLoginInfo.Username));
    LoginJson->SetField("password", VaRestSubsystem->ConstructJsonValueString(InLoginInfo.Password));

    Request->SetRequestObject(LoginJson);

    // Add call backs from our lovely generated delegates
    Request->OnRequestComplete.Add(OnLoginCompleteDelegate);
    Request->OnRequestFail.Add(OnLoginFailDelegate);

    Request->ExecuteProcessRequest();
}

void UApexAPI::OnLoginComplete(UVaRestRequestJSON* Request)
{
    LogAPEX("Login Completed!");
    if (Request->GetResponseCode() == 200)
    {
        if (CurrentActiveLogin.IsSet())
        {
            WarnAPEX("Replacing the current login.");
            CurrentActiveLogin.Invalidate();
        }

        CurrentActiveLogin.FromJsonObject(Request->GetResponseObject()->GetRootObject());
    }

    OnRequestComplete.Broadcast(ApexRequestType::Login, Request);
    OnStaticRequestComplete.Broadcast(ApexRequestType::Login, Request);
}

void UApexAPI::OnLoginFail(UVaRestRequestJSON* Request)
{
    LogAPEX("Login failed with response code %i", Request->GetResponseCode());
    OnRequestFail.Broadcast(ApexRequestType::Login, Request);
    OnStaticRequestFail.Broadcast(ApexRequestType::Login, Request);
}

bool UApexAPI::JoinSession(FString InScenarioID, const FXAPIExtension& InContextExtension)
{
    if (!CurrentActiveLogin.IsSet())
    {
        ErrorAPEX("Cannot join session with no active login.");
        return false;
    }

    if (!InScenarioID.IsEmpty())
    {
        ScenarioId = InScenarioID;
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
    JoinData.ModuleID = LoadedModuleId;
    JoinData.Uuid = CurrentSessionGuid.ToString();
    JoinData.EventType = Apex::EventTypes::PIXOVR_SESSION_JOINED;
    JoinData.JsonData = Statement;

    UVaRestRequestJSON* Request = VaRestSubsystem->ConstructVaRestRequestExt(EVaRestRequestVerb::POST, EVaRestRequestContentType::json);
    FString PingURL = URL + "/event";
    Request->SetURL(PingURL);
    Request->SetHeader("Authorization: Bearer", CurrentActiveLogin.SessionToken);

    UVaRestJsonObject* JoinSessionJson = VaRestSubsystem->ConstructVaRestJsonObject();
    JoinSessionJson->SetRootObject(JoinData.ToJsonObject());

    Request->SetRequestObject(JoinSessionJson);

    // Add call backs from our lovely generated delegates
    Request->OnRequestComplete.Add(OnJoinSessionCompleteDelegate);
    Request->OnRequestFail.Add(OnJoinSessionFailDelegate);

    Request->ExecuteProcessRequest();

    return true;
}

void UApexAPI::OnJoinSessionComplete(UVaRestRequestJSON* Request)
{
    LogAPEX("Session was successfully joined.");
    SessionInProgress = true;
    OnRequestComplete.Broadcast(ApexRequestType::JoinSession, Request);
    OnStaticRequestComplete.Broadcast(ApexRequestType::JoinSession, Request);
}

void UApexAPI::OnJoinSessionFail(UVaRestRequestJSON* Request)
{
    LogAPEX("Failed to join session. %d", Request->GetResponseCode());
    SessionInProgress = false;
    CurrentSessionGuid.Invalidate();
    OnRequestFail.Broadcast(ApexRequestType::JoinSession, Request);
    OnStaticRequestFail.Broadcast(ApexRequestType::JoinSession, Request);
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
    SessionData.ModuleID = LoadedModuleId;
    SessionData.Uuid = CurrentSessionGuid.ToString();
    SessionData.EventType = Apex::EventTypes::PIXOVR_SESSION_COMPLETE;
    SessionData.JsonData = Statement;
    SessionData.SessionData = InSessionData;

    // Now we create our request and send it!
    UVaRestRequestJSON* Request = VaRestSubsystem->ConstructVaRestRequestExt(EVaRestRequestVerb::POST, EVaRestRequestContentType::json);
    FString PingURL = URL + "/event";
    Request->SetURL(PingURL);
    Request->SetHeader("Authorization: Bearer", CurrentActiveLogin.SessionToken);

    UVaRestJsonObject* CompleteSessionJson = VaRestSubsystem->ConstructVaRestJsonObject();
    CompleteSessionJson->SetRootObject(SessionData.ToJsonObject());

    Request->SetRequestObject(CompleteSessionJson);

    Request->OnRequestComplete.Add(OnCompleteSessionCompleteDelegate);
    Request->OnRequestFail.Add(OnCompleteSessionFailDelegate);

    Request->ExecuteProcessRequest();

    return true;
}

void UApexAPI::OnCompleteSessionComplete(UVaRestRequestJSON* Request)
{
    LogAPEX("Session complete!");
    SessionInProgress = false;
    CurrentSessionGuid.Invalidate();
    OnRequestComplete.Broadcast(ApexRequestType::CompleteSession, Request);
    OnStaticRequestComplete.Broadcast(ApexRequestType::CompleteSession, Request);
}

void UApexAPI::OnCompleteSessionFail(UVaRestRequestJSON* Request)
{
    WarnAPEX("Failed to complete session. %d", Request->GetResponseCode());
    OnRequestFail.Broadcast(ApexRequestType::CompleteSession, Request);
    OnStaticRequestFail.Broadcast(ApexRequestType::CompleteSession, Request);
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
    Event.ModuleID = LoadedModuleId;
    Event.Uuid = CurrentSessionGuid.ToString();
    Event.EventType = Apex::EventTypes::PIXOVR_SESSION_EVENT;
    Event.JsonData = InStatement;

    // Now we create our request and send it!
    UVaRestRequestJSON* Request = VaRestSubsystem->ConstructVaRestRequestExt(EVaRestRequestVerb::POST, EVaRestRequestContentType::json);
    FString PingURL = URL + "/event";
    Request->SetURL(PingURL);
    Request->SetHeader("Authorization: Bearer", CurrentActiveLogin.SessionToken);

    UVaRestJsonObject* CompleteSessionJson = VaRestSubsystem->ConstructVaRestJsonObject();
    CompleteSessionJson->SetRootObject(Event.ToJsonObject());

    Request->SetRequestObject(CompleteSessionJson);

    Request->OnRequestComplete.Add(OnSendSessionEventCompleteDelegate);
    Request->OnRequestFail.Add(OnSendSessionEventFailDelegate);

    Request->ExecuteProcessRequest();

    return true;
}

void UApexAPI::OnSendSessionEventComplete(class UVaRestRequestJSON* Request)
{
    LogAPEX("Session event was sent successfully.");
    OnRequestComplete.Broadcast(ApexRequestType::SessionEvent, Request);
    OnStaticRequestComplete.Broadcast(ApexRequestType::SessionEvent, Request);
}

void UApexAPI::OnSendSessionEventFail(class UVaRestRequestJSON* Request)
{
    WarnAPEX("Failed to send session event. %d", Request->GetResponseCode());
    OnRequestFail.Broadcast(ApexRequestType::SessionEvent, Request);
    OnStaticRequestFail.Broadcast(ApexRequestType::SessionEvent, Request);
}

#undef FatalAPEX
#undef ErrorAPEX
#undef WarnAPEX
#undef LogAPEX