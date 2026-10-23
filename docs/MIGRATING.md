# Migrating from 1.x to 2.0

2.0 replaces the 1.x plugin's own HTTP code (VaRest requests, xAPI types) with
apex-sdk-core, and brings in the UE5 plugins' launcher, auth and data-reporting
systems. Most calls have a direct replacement. Results now arrive through
async nodes or C++ callbacks, not through `OnRequestComplete` with a VaRest
request.

## Settings

The section stays `[/Script/ApexSDK.ApexSDKSettings]` in `DefaultEngine.ini`.
`ModuleId`, `ModuleVersion`, `bSyncVersion` and `AdditionalVersionFields` keep
their keys.

| 1.x | 2.0 |
| --- | --- |
| `ServerURI` | `Environment` (plus `HeadsetApiUrl`/`PlatformApiUrl` for custom hosts) |
| `EPVRVersionFields::AndroidStoreVersion` | `EApexVersionField::ANDROID_STORE_VERSION` |
| `EPVRVersionFields::PixoVRPluginVersion` | removed (it wrote another plugin's setting) |
| — | Module Name, login, session, telemetry settings (see the README) |

Re-select **Additional Synchronized Version Fields** after upgrading, since
the enum changed.

## Classes

| 1.x / UE5 plugins | 2.0 |
| --- | --- |
| `UApexAPI` (game instance subsystem) | `UApexSubsystem` |
| `UPixoAuth`, `UPixoDataReporter` (UE5) | `UApexSubsystem` |
| `UApexSDKLibrary` getters | `UApexSubsystem` getters, `UApexBlueprintLibrary::GetApex` |
| `UPixoLauncherSubsystem` (1.x and UE5) | `UApexLauncherSubsystem` |
| `UPixoDownloadManagerSubsystem` (UE5) | `UApexDownloadManagerSubsystem` |
| `UPixoConfigSettings::HeadsetType` (UE5) | `UApexLauncherSettings::HeadsetType` |
| VaRest module | removed |
| `FXAPIStatement`, `FXAPIExtension`, optional types | removed: session data is JSON; statements are JSON for `SendSessionStatement` |

## Calls

| 1.x | 2.0 |
| --- | --- |
| `Login(user, password, bShouldLogout)` | `Login(user, password, callback)` / **Apex Login** |
| `LoginWithAuthToken()` | `LoginWithToken("")` / **Apex Login With Token** |
| `RequestAuthorizationCode()` + `OnAuthorizationCodeRetrieved` | **Apex Login With Authorization Code** (connects the socket itself) |
| `ConnectWebSocketService()` | not needed |
| `VerifyModuleAccess(id)` | `CheckModuleAccess(id, callback)`; runs after every login by default |
| `JoinSession(scenario, FXAPIExtension, bIsMultiplayer)` | `JoinSession(scenario, sessionDataJson, callback)` |
| `SendSessionEvent(FXAPIStatement)` | `SendSessionEvent(action, target, eventDataJson)`, or `SendSessionStatement(statementJson)` |
| `CompleteSession(FSessionData)` | `CompleteSession(FApexSessionResult, resultDataJson)` |
| `FetchTopTen*` | removed (the core does not include reporting queries) |
| `ExitApplication(target, type)` | `ExitToLauncher(nextTarget)` |
| `SetExitTarget(target, type)` | `SetExitTarget(target)` (the type follows from `://`) |
| `SetExitParameters(params)` | `SetOptionalData(params)` |
| `HasLaunchAuthToken()` | `HasLaunchLoginToken()` |
| `PixoAuth::AuthenticatedLaunch(app, return)` (UE5) | `LaunchModule(app, return)` |
| `UPixoLauncherSubsystem::OpenURL` / `LaunchURL` | `UApexLauncherSubsystem::LaunchUrl(url, quitAfterLaunch)` |
| `DeleteSharedFileExist` (UE5) | `DeleteSharedFile` |
| 18 download/install/uninstall delegates (UE5) | `OnPackageEvent(package, EApexPackageEvent, progress, size)` |

## Behavior that changed

- **Platform errors are failures.** The core detects `{"Error":"true"}` bodies
  on module access and sessions, which older clients treated as success.
- **Session calls need a logged-in user whose module access was checked**,
  unless **Require Module Access For Sessions** is off.
- **Sessions send xAPI by default**, built by the core in the same shape 1.x
  sent. Turn off **Send xAPI Session Events** for the data format.
- **An active session is completed on shutdown** (unsuccessful, no score),
  unless **Complete Session On Shutdown** is off. The heartbeat keeps the
  session alive meanwhile.
- **Logs never contain tokens, passwords or response bodies.**
- **Android `sharedUserId`:** 1.x and PixoCore put `android:sharedUserId` on
  `<application>`, where Android ignores it. 2.0 puts it on `<manifest>` behind
  **Use System Shared User ID**, which is off by default: with it, only builds
  signed with the platform key install.
- **Installs are for Pico and HTC Focus only.** On Quest, HTC Business and
  other headsets, `InstallApplication`, `UpdateApplication` and
  `UninstallApplication` return false without downloading anything
  (`SupportsPackageInstalls` tells in advance). Those builds no longer request
  install permissions, and the HTC Business install-permission grant is gone.
  HTC Focus installs report the package name instead of the APK path, so the
  download manager finishes them.
- **Exit To Launcher** completes an active session before handing over,
  because on Android the app's process ends with the hand-off.
- **Changing environment** fails the calls still in flight with `CANCELLED`
  instead of dropping them.
