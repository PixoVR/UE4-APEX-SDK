# Apex SDK for Unreal Engine 4 — 2.0

An Unreal Engine plugin for the Apex platform, for Unreal Engine **4.26** and
**4.27**. Since 2.0 it is a thin Unreal layer over the native
[apex-sdk-core](https://github.com/PixoVR/apex-sdk-core) library, the same core
the Unity SDK 2.0 uses: every platform call, session event and telemetry
packet goes through the core's C interface.

The 2.0 plugin brings together:

- **apex-sdk-core 0.2.0**: login (password, token, Quick ID, login codes),
  module access, module lists and pages with lifecycles, sessions (data or
  xAPI), heartbeat, telemetry and the launcher hand-off.
- **The UE5 plugins** `pixo-unreal-core` (launcher: install, update, uninstall,
  launch, intents, shared files, device serial),
  `pixo-unreal-auth` (login, authorization-code WebSocket, launching another
  module with the login) and `pixo-unreal-datareporting` (sessions).
- **The Unity SDK 2.0's analytics components**: tracked objects, interactables,
  gaze, steps, poses and analytics providers, as Unreal components.

Upgrading from 1.x? Read [docs/MIGRATING.md](docs/MIGRATING.md).

## Modules

| Module | What it holds |
| --- | --- |
| `ApexCore` | Compiles apex-sdk-core from `Source/ThirdParty/ApexCore` (copied in by `Tools/update_core.py`). Exposes `apex/Apex.h`. |
| `ApexSDK` | `UApexSubsystem` (one per game instance), the result types, Blueprint async nodes, settings. |
| `ApexLauncher` | `UApexLauncherSubsystem` and `UApexDownloadManagerSubsystem`, and the Android Java and UPL that do the work. |
| `ApexAnalytics` | `UApexAnalyticsSubsystem`, analytics providers, and the components you add to actors. |

## Setting up

1. Copy the plugin into your project's `Plugins/ApexSDK` folder and enable it.
2. **Project Settings > Plugins > Apex SDK**: set the **Module ID**, **Module
   Name**, **Module Version** (`MAJOR.MINOR.PATCH`) and the **Environment**.
3. Android only, **Project Settings > Plugins > Apex Launcher**: pick the
   **Headset Type** (it chooses how packages are installed). Builds need
   Android SDK API level 29 or later for shared-storage files. Turn off **Use
   System Shared User ID** for builds that are not signed with the platform
   key.

The subsystem starts with the game instance, reads the launcher's arguments
(`pixotoken`, `returntarget`, `optional` and so on, from the command line, a
`pixovr://` link or the Android intent) and polls the core every frame.

## Using it from Blueprints

Every platform call is an async node with **Success** and **Failure** pins,
under *Apex*:

| Node | Result |
| --- | --- |
| Apex Login, Apex Login With Token, Apex Quick ID Login | `FApexUser` |
| Apex Login With Authorization Code | the code to show (Code Received), then `FApexUser` |
| Apex Get Quick ID Users | `FApexQuickIdUsers` |
| Apex Generate Login Code | `FApexLoginCode` |
| Apex Check Module Access | `FApexModuleAccess` |
| Apex Get User Modules | module IDs per user |
| Apex Get Module List, Apex Get Module Page | `FApexModule` array, `FApexModulePage` |
| Apex Join Session | the session ID |
| Apex Send Session Event, Apex Send Session Statement, Apex Complete Session, Apex Ping | success or failure |

Each result carries an `FApexResponse`: `Error` (`EApexErrorCode`),
`HttpStatus` and `Message`. **Get Apex** returns the subsystem for everything
else: the current user, session state, `Exit To Launcher`, `Launch Module`,
and events such as **On Login Succeeded** and **On Session Joined**.

A typical module:

1. **Apex Login With Token** with an empty token. That uses the launcher's
   `pixotoken`, then checks the user's access to this module.
2. **Apex Join Session** with a scenario ID and, optionally, session data as
   JSON. **Make Json Object** and the **Add Json ...** nodes build it.
3. **Apex Send Session Event** for what happens: an action, a target object,
   optional JSON data.
4. **Apex Complete Session** with an `FApexSessionResult`: score, minimum,
   maximum, duration, complete and success.
5. **Exit To Launcher** to hand back to the launcher, passing the login on.

Or add an **Apex Session** component to an actor in the scenario's level. It
logs in with the launcher's token, joins on BeginPlay and completes on
EndPlay; call its **Complete Session** to report a score.

## Using it from C++

```cpp
#include "ApexSubsystem.h"

UApexSubsystem* apex = UApexSubsystem::Get(this);
apex->LoginWithToken(FString(), [apex](const FApexResponse& response, const FApexUser& user)
{
	if (!response.IsSuccess())
	{
		UE_LOG(LogTemp, Warning, TEXT("Login failed: %s"), *response.Message);
		return;
	}

	apex->JoinSession(TEXT("Forklift-Basics"), TEXT("{\"difficulty\":\"easy\"}"),
		[](const FApexResponse& joinResponse, int64 sessionId)
		{
			// ...
		});
});
```

Results arrive on a later frame, on the game thread.

## Telemetry and data tracking

Add components to actors; the built-in **PixoVR** provider records everything
in the core's telemetry format (the same packets the Unity SDK 2.0 sends):

| Component | Add it to | It records |
| --- | --- | --- |
| **Apex Tracked Object** | anything worth tracking | registration, engagements and interactions you report, its pose when Track Pose is on |
| **Apex Interactable** | grabbable or usable objects | Grab/Release and HoverEnter/HoverExit as engagements, Use as an interaction, optionally as session events |
| **Apex Gaze Tracker** | the player pawn | a gaze engagement on a tracked object looked at for Dwell Seconds |
| **Apex Head Gaze Source**, **Apex Eye Gaze Source** | the player pawn | where the gaze comes from: the head, or the eye tracker (falls back to the head) |
| **Apex Spatial Sampler** | the VR camera | marks the head; otherwise the player's camera is used |
| **Apex Step Tracker** | a step's actor | Begin, Complete (score) and Fail (score) of a named step |
| **Apex Session** | the scenario's level | the session itself (see above) |

**Apex Analytics** (`UApexAnalyticsSubsystem`) has `BeginStep`,
`CompleteStep` and `FailStep` for steps without a component, and Blueprint
events for every recorded moment. Bind them to forward data to another
service.

In C++, implement `IApexAnalyticsProvider` and register it with
`UApexAnalyticsSubsystem::RegisterProvider`.

**Project Settings > Plugins > Apex SDK > Telemetry** sets where packets go:

| Sink | Where packets go |
| --- | --- |
| `LOG` | the log (default) |
| `FILE` | `Saved/ApexTelemetry` |
| `HTTP` | POSTed to the telemetry URL with the user's token |
| `NONE` | nowhere: nothing is recorded |

The same section sets how often packets go out and how often poses are
sampled. Packets also go out when the app goes to the background.

Poses are converted from Unreal's axes (centimeters, X forward, Z up) to the
core's (meters, Unity's axes) in `UApexSubsystem::MakeApexPose`.

## Launching, installing and updating apps

`UApexLauncherSubsystem` (an engine subsystem; Blueprint: *Get Apex Launcher*)
does the following. Each item is Android only unless it says otherwise.

| What | Calls |
| --- | --- |
| Launch an app with extras, or open a URL (URLs everywhere) | `LaunchApplication`, `LaunchUrl` |
| Read how this app was launched | `GetIntentData`, `GetApplicationExtra` |
| Install or update from a URL to an APK or a zip holding one | `InstallApplication`, `UpdateApplication` |
| Compare installed and available versions | `GetApplicationVersionName`, `IsUpdateAvailable`, `CompareVersions` |
| Uninstall | `UninstallApplication` |
| Text files in the shared Download folder (Android 10+) | `DoesSharedFileExist`, `WriteSharedFile`, `ReadSharedFile`, `DeleteSharedFile` |
| Device details | `GetDeviceSerialNumber` (ManageXR's, else the system's), `GetDeviceMacAddress`, `IsDeviceManaged` |

Installs report progress through `OnPackageEvent`.
`UApexDownloadManagerSubsystem` follows each install, update or uninstall
through its states (downloading, extracting, installing, completed and so on)
for a UI. Off Android, **Set Mock Downloader Class** spawns an actor in place
of a download, so the UI can be tested in the editor.

`UApexSubsystem::LaunchModule` launches another module with the current login,
as `PixoAuth::AuthenticatedLaunch` did. `ExitToLauncher` returns to the
launcher or the app it named.

## Environments

| Setting | Headset API | Platform API |
| --- | --- | --- |
| North America - Production | `https://modules.apex.pixovr.com` | `https://apex.pixovr.com` |
| North America - Staging | `https://modules.apex.stage.pixovr.com` | `https://apex.stage.pixovr.com` |
| North America - Development | `https://modules.apex.dev.pixovr.com` | `https://apex.dev.pixovr.com` |
| Saudi Arabia - Production | `https://saudi.modules.apex.pixovr.com` | `https://saudi.apex.pixovr.com` |
| Local | `http://localhost:8001` | `http://localhost:8000` |
| Custom | Headset API URL setting | Platform API URL setting |

On Android, a `pixoconfig.cnf` file in shared storage
(`{"Platform": "NA Stage"}`) selects the environment, as in the Unity SDK.
`ChangeEnvironment` switches at runtime and starts logged out.

HTTP goes through the core's own backends on Windows (WinHTTP) and Android
(`HttpURLConnection`). Elsewhere, or with **Always Use Unreal HTTP**, it goes
through Unreal's HTTP module.

## Sessions: data or xAPI

**Send xAPI Session Events** (on by default, like the Unity SDK) makes session
calls send the xAPI statements the platform reads today. The core builds them
from the session JSON. `SendSessionStatement` sends a statement you built.
Turn it off once the platform's new session route is live. The data format
then sends the module's JSON as an opaque payload. See
[apex-sdk-core's spec](https://github.com/PixoVR/apex-sdk-core/blob/main/spec/README.md).

## Updating the core

```sh
python3 Tools/update_core.py --source ../apex-sdk-core           # a checkout, e.g. at a release tag
python3 Tools/update_core.py --zip apex-sdk-core-v0.2.0.zip      # or a release zip
```

The script replaces `Source/ThirdParty/ApexCore` and records the version in
its `VERSION.txt`. When the core's ABI changes (`APEX_ABI_MINOR` in
`Apex.h`), update `UApexSubsystem` to match.

## Not supported (yet)

- **Reporting queries** (1.x `FetchTopTen`): the core leaves them out on
  purpose.
- **PixoConfig's remote regional configuration** (multiplayer and voice URLs):
  environments come from the settings and `pixoconfig.cnf`.
- **Launching apps by package name** on Windows.

## Code style

The company style, as in apex-sdk-core and the Unity SDK. The rules and the
checks to run are in [AGENTS.md](AGENTS.md).

## License

See [LICENSE.md](LICENSE.md) and [ThirdpartyNotice.md](ThirdpartyNotice.md).
