# Working in UE4-APEX-SDK

The Apex SDK for Unreal Engine 4.26 and 4.27. Since 2.0 it is a thin Unreal
layer over the native [apex-sdk-core](https://github.com/PixoVR/apex-sdk-core)
library: every platform call, session event and telemetry packet goes through
the core's C interface (`Source/ThirdParty/ApexCore/include/apex/Apex.h`).
Read `README.md` for how the pieces fit, and `docs/MIGRATING.md` for what
changed from 1.x.

## Branches

- The 2.0 line lives on `2.0`; 1.x lives on `main`. Feature work goes on its
  own branch and reaches `2.0` through a pull request.
- Never force-push a shared branch.

## The native core

- `Source/ThirdParty/ApexCore` is a copy of apex-sdk-core's sources. Update it
  with `python3 Tools/update_core.py --source <checkout>` (or `--zip`), never
  by hand; the script also generates `GraphqlQueries.hpp` and records the
  version in `VERSION.txt`.
- The `ApexCore` module compiles it: each file in `Source/ApexCore/Private`
  includes one core source file, and `CoreBuiltinTransport.cpp` picks the HTTP
  backend for the platform. When the core gains or loses a source file, add or
  remove its wrapper.
- When `Apex.h` changes (`APEX_ABI_MINOR`), update `UApexSubsystem` and the
  types in `ApexTypes.h` to match. Unreal enums mirroring core enums keep the
  core's values.
- No HTTP in the plugin except `FApexUnrealHttp`, which hands the core's own
  requests to Unreal's HTTP module where the core has no backend, and the
  authorization-code WebSocket, which the core does not have.

## Android

`Source/ApexLauncher/ApexLauncherAndroid_UPL.xml` adds the launcher's methods
to `com.epicgames.ue4.GameActivity`; the Java classes under
`Source/ApexLauncher/Java` (plus `Other/`, `Pico/` or `WaveVR/` for the
headset) are copied into the build. The JNI method names in
`Private/Android/ApexLauncherJni.cpp` must match `PixoEventNotifier.java`'s
native methods, and the signatures in `ApexLauncherSubsystem.cpp` must match
the UPL's `AndroidThunkJava_*` methods.

## Code style

Every C++ file we own (`Source/`, except `Source/ThirdParty/`) follows the
company style of apex-sdk-core and the Unity SDK, with the exceptions Unreal
forces.

### Naming

| What | Style | Example |
| --- | --- | --- |
| Functions and methods | PascalCase | `JoinSession`, `GetTrackedId` |
| Types | Unreal's prefix + PascalCase (UnrealHeaderTool requires it) | `UApexSubsystem`, `FApexUser`, `IApexAnalyticsProvider` |
| Member variables of classes | private, `m_` + camelCase | `m_sessionInProgress`, `m_trackedId` |
| Fields of `USTRUCT`s and `UDeveloperSettings` | public PascalCase, `b` prefix for booleans | `FApexUser::FirstName`, `bSyncVersion` |
| Parameters and local variables | camelCase | `moduleData`, `sessionId` |
| Constants | `k` + PascalCase | `kTokenArgument` |
| Enums | `E` + PascalCase, values UPPER_CASE | `EApexErrorCode::NOT_LOGGED_IN` |
| Files | named after the main type they hold, without its prefix | `ApexSubsystem.cpp` |

Structs and settings are the exception to private members for the same
reason C structs are in apex-sdk-core: Blueprints, the details panel and
config files read their fields by name. Settings keys from 1.x keep their
names so projects upgrade without losing values.

Editable properties of components are private `m_` members with a
`DisplayName` meta, and functions to read and change them.

Unreal types must not reuse names from `Apex.h` (`EApexError`,
`EApexEnvironment`, `ApexPose` and so on): the C header is included
everywhere the subsystem is.

**Names must say what the thing is.** No single letters, no abbreviations, and
no vague words on their own: `serialNumber` not `sn`, `javaEnvironment` not
`env`. Units go in the name when a number has one (`DwellSeconds`,
`m_maximumDistanceCentimeters`). No name shorter than three characters.

### Formatting

- Tabs for indentation, 4 columns wide. Lines up to 120 columns.
- Every opening and closing brace on its own line.
- Every `if`, `else`, `for`, `while` and `do` uses braces.
- After the closing brace of a conditional or loop, leave a blank line before
  more code, unless the next line closes the enclosing block or continues the
  statement.
- `CoreMinimal.h` first and `*.generated.h` last in headers; `.clang-format`
  sorts them so.

## Checking

Run both before you push:

```sh
clang-format --dry-run --Werror $(git ls-files 'Source/*.h' 'Source/*.cpp' | grep -v '^Source/ThirdParty/')
python3 Tools/style/check_style.py      # blank lines after blocks, tabs, file names; --fix inserts blank lines
```

Nothing here builds without Unreal Engine: compile the plugin in a 4.26 or
4.27 project (Win64 and Android) before merging.
