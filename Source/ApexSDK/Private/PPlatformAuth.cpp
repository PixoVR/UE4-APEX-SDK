// Copyright 2022 PixoVR Corp. All Rights Reserved.


#include "PPlatformAuth.h"
#include "VaRest/Public/VaRestSubsystem.h"
#include "ApexSDK.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "PPlatformCore.h"
#include "PPlatformCoreSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogPlatformAuth, Log, All);

#define Log(pmt, ...) UE_LOG(LogPlatformAuth, Log, TEXT(pmt), ##__VA_ARGS__)
#define Warn(pmt, ...) UE_LOG(LogPlatformAuth, Warning, TEXT(pmt), ##__VA_ARGS__)
#define Error(pmt, ...) UE_LOG(LogPlatformAuth, Error, TEXT(pmt), ##__VA_ARGS__)
#define Fatal(pmt, ...) UE_LOG(LogPlatformAuth, Fatal, TEXT(pmt), ##__VA_ARGS__)

UPPlatformAuth::UPPlatformAuth()
    : UEngineSubsystem()
    , PlatformSettings(nullptr)
{
}

void UPPlatformAuth::SetupModuleSettings()
{
    if (FPPlatformCoreModule::IsAvailable())
    {
        PlatformSettings = FPPlatformCoreModule::Get().GetSettings();
    }

    UpdateSettings(PlatformSettings);
    Log("Launching module %d version %s", ModuleId, *ModuleVersion);
}

void UPPlatformAuth::UpdateSettings(UPPlatformCoreSettings* InSettings)
{
    check(InSettings != nullptr);
    PlatformSettings = InSettings;
    URL = PlatformSettings->PlatformURL;

    if (URL.IsEmpty())
    {
        Fatal("No URL available. Please set the Server URI in the settings.");
    }
    else
    {
        URL.RemoveFromEnd("/");
        if (!URL.Contains("https://"))
        {
            if (URL.Contains("http://"))
            {
                URL = URL.Replace(TEXT("http://"), TEXT(""));
            }

            URL.InsertAt(0, "https://");
        }
    }

    ModuleId = PlatformSettings->ModuleId;

    WebSocketURL = PlatformSettings->PlatformURL;
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

    check(PlatformSettings->CheckVersionString(PlatformSettings->ModuleVersion));
    ModuleVersion = PlatformSettings->ModuleVersion;
}

void UPPlatformAuth::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    VaRestSubsystem = GEngine->GetEngineSubsystem<UVaRestSubsystem>();

    SetupModuleSettings();

    if (PlatformSettings != nullptr)
    {
        PlatformSettings->OnStaticSettingsChanged.AddLambda([&](UPPlatformCoreSettings* UpdatedSettings) -> void {
            UpdateSettings(UpdatedSettings);
        });
    }

    if (WebSocketEnabled)
    {
        WebSocket = MakeUnique<ApexWebSocket>();

        WebSocket->OnApexWebSocketConnected().AddLambda([&]() -> void {OnWebSocketConnected.Broadcast(); OnStaticWebSocketConnected.Broadcast(); });
        WebSocket->OnApexWebSocketConnectFailed().AddLambda([&](const FString& Error) -> void {OnWebSocketConnectFailed.Broadcast(Error); OnStaticWebSocketConnectFailed.Broadcast(Error); });
        WebSocket->OnApexWebSocketDisconnected().AddLambda([&](int32 StatusCode, const FString& Reason, bool bWasClean) -> void {OnWebSocketDisconnected.Broadcast(StatusCode, Reason, bWasClean); OnStaticWebSocketDisconnected.Broadcast(StatusCode, Reason, bWasClean); });
        WebSocket->OnApexAuthCodeRetrieved().AddLambda([&](const FString& AuthorizationCode) -> void {OnAuthorizationCodeRetrieved.Broadcast(AuthorizationCode); OnStaticAuthorizationCodeRetrieved.Broadcast(AuthorizationCode); });
        WebSocket->OnApexAuthenticateComplete().AddLambda([&](UVaRestJsonObject* LoginJsonObject) -> void { HandleLogin(LoginJsonObject); });

        WebSocket->AttemptConnect(WebSocketURL);
    }
}

bool UPPlatformAuth::IsLoggedIn()
{
    return CurrentActiveLogin.IsSet();
}

bool UPPlatformAuth::IsWebSocketConnected()
{
    if (WebSocketEnabled)
    {
        return WebSocket->IsWebsocketConnected();
    }

    return false;
}


void UPPlatformAuth::ConnectWebSocketService()
{
    if (WebSocketEnabled)
    {
        if (!WebSocket->AttemptConnect(WebSocketURL))
        {
            Warn("Web Socket already connected.");
        }
    }
}

void UPPlatformAuth::RequestAuthorizationCode()
{
    if (WebSocketEnabled)
    {
        WebSocket->RequestAuthorizationCode();
    }
}

void UPPlatformAuth::Ping()
{
    UVaRestRequestJSON* Request = VaRestSubsystem->ConstructVaRestRequestExt(EVaRestRequestVerb::GET, EVaRestRequestContentType::none);
    FString RequestURL = URL + "/ping";
    Request->SetURL(RequestURL);

    Request->OnStaticRequestComplete.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnPingComplete(Request); });
    Request->OnStaticRequestFail.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnPingFail(Request); });

    Request->ExecuteProcessRequest();
}

void UPPlatformAuth::OnPingComplete(const UVaRestRequestJSON* Request)
{
    Log("On Ping Completed!");
    OnPingCompleted.Broadcast();
    OnStaticPingCompleted.Broadcast();
}

void UPPlatformAuth::OnPingFail(const UVaRestRequestJSON* Request)
{
    Log("On Ping Failed!");

    UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
    FAPEXRequestFailed APEXFailedRequest;

    APEXFailedRequest.FromJsonObject(ResponseObject->GetRootObject());

    OnPingFailed.Broadcast(APEXFailedRequest);
    OnStaticPingFailed.Broadcast(APEXFailedRequest);
}

void UPPlatformAuth::LoginUser(const FString& InUsername, const FString& InPassword, bool bShouldLogout)
{
    Login(FApexLoginInfo(InUsername, InPassword), bShouldLogout);
}

void UPPlatformAuth::Login(const FApexLoginInfo& InLoginInfo, bool bShouldLogout)
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

void UPPlatformAuth::OnLoginComplete(const UVaRestRequestJSON* Request)
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

void UPPlatformAuth::OnLoginFail(const UVaRestRequestJSON* Request)
{
    Log("Login failed with response code %i", Request->GetResponseCode());

    UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
    FAPEXRequestFailed APEXFailedRequest;
    
    APEXFailedRequest.FromJsonObject(ResponseObject->GetRootObject());

    OnAuthorizationFailed.Broadcast(APEXFailedRequest, Request);
    OnStaticAuthorizationFailed.Broadcast(APEXFailedRequest, Request);
}


void UPPlatformAuth::HandleLogin(UVaRestJsonObject* LoginJsonObject)
{
    Log("Login Completed!");
    if (CurrentActiveLogin.IsSet())
    {
        Warn("Replacing the current login.");
        CurrentActiveLogin.Invalidate();
    }

    CurrentActiveLogin.FromJsonObject(LoginJsonObject->GetRootObject());

    VerifyModuleAccess();
}

void UPPlatformAuth::Logout()
{
    const UVaRestRequestJSON* DummyRequestJSON = VaRestSubsystem->ConstructVaRestRequest();
    if (CurrentActiveLogin.IsSet())
    {
        CurrentActiveLogin.Invalidate();

        OnLogoutCompleted.Broadcast();
        OnStaticLogoutCompleted.Broadcast();
    }
    else
    {
        FAPEXRequestFailed APEXFailedRequest;
        APEXFailedRequest.Error = true;
        APEXFailedRequest.Message = "No user currently logged in.";

        UVaRestJsonObject* ResponseObject = DummyRequestJSON->GetResponseObject();
        ResponseObject->SetBoolField("Error", APEXFailedRequest.Error);
        ResponseObject->SetStringField("Message", APEXFailedRequest.Message);

        OnLogoutFailed.Broadcast(APEXFailedRequest);
        OnStaticLogoutFailed.Broadcast(APEXFailedRequest);
    }
}

void UPPlatformAuth::OnLogoutComplete(const UVaRestRequestJSON* Request)
{
    bool FailedResponse = true;
    if (Request->GetResponseCode() == 200)
    {
        if (!Request->GetResponseObject()->GetRootObject()->HasField("Error"))
        {
            OnLogoutCompleted.Broadcast();
            OnStaticLogoutCompleted.Broadcast();
        }
    }

    if (FailedResponse)
    {
        OnLogoutFail(Request);
    }
}

void UPPlatformAuth::OnLogoutFail(const UVaRestRequestJSON* Request)
{
    Log("Logout failed with response code %i", Request->GetResponseCode());

    UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
    FAPEXRequestFailed APEXFailedRequest;

    APEXFailedRequest.FromJsonObject(ResponseObject->GetRootObject());

    OnLogoutFailed.Broadcast(APEXFailedRequest);
    OnStaticLogoutFailed.Broadcast(APEXFailedRequest);
}

void UPPlatformAuth::VerifyModuleAccess()
{
    UVaRestRequestJSON* Request = VaRestSubsystem->ConstructVaRestRequestExt(EVaRestRequestVerb::GET, EVaRestRequestContentType::none);

    FStringFormatOrderedArguments FormatArguments;
    FormatArguments.Add(FStringFormatArg(URL));
    FormatArguments.Add(FStringFormatArg(CurrentActiveLogin.ID));
    FormatArguments.Add(FStringFormatArg(ModuleId));
    FString AccessURL = FString::Format(TEXT("{0}/access/user/{1}/module/{2}"), FormatArguments);
    Request->SetURL(AccessURL);
    Request->SetHeader("Authorization: Bearer", CurrentActiveLogin.SessionToken);

    Request->OnStaticRequestComplete.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnUserVerificationComplete(Request); });
    Request->OnStaticRequestFail.AddLambda([&](UVaRestRequestJSON* Request) -> void { OnUserVerificationFail(Request); });

    Request->ExecuteProcessRequest();
}

void UPPlatformAuth::OnUserVerificationComplete(const UVaRestRequestJSON* Request)
{
    bool FailedResponse = true;
    if (Request->GetResponseCode() == 200)
    {
        UVaRestJsonObject* ResponseObject = Request->GetResponseObject();
        if (!ResponseObject->GetRootObject()->HasField("Error"))
        {
            Log("User Verification Completed!");

            if (ResponseObject->HasField("access"))
            {
                FailedResponse = !ResponseObject->GetBoolField("access");
            }

            if (FailedResponse == false)
            {
                OnAuthorizationCompleted.Broadcast(CurrentActiveLogin, Request);
                OnStaticAuthorizationCompleted.Broadcast(CurrentActiveLogin, Request);
            }
        }
    }

    if (FailedResponse)
    {
        OnUserVerificationFail(Request);
    }
}

void UPPlatformAuth::OnUserVerificationFail(const UVaRestRequestJSON* Request)
{
    Log("User module access verification failed with response code %i", Request->GetResponseCode());

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

    OnAuthorizationFailed.Broadcast(APEXFailedRequest, Request);
    OnStaticAuthorizationFailed.Broadcast(APEXFailedRequest, Request);
}

#undef Fatal
#undef Error
#undef Warn
#undef Log
