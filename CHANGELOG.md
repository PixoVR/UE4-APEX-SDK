# Changelog

All notable changes to this package will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2022-08-09
### This is the first release of *UE4 APEX SDK*.

## [1.0.1] - 2022-08-26
### Add FAPEXRequestFail struct to the OnApexRequestFail event to capture simple error data returned from the APEX API.
### Change ModuleID to ModuleId across the whole code base.
### Added module access verification to the login process.

## [1.1.0] - 2022-09-01
### Adds the ability to query top ten data by module and other inforation.
### Fixes an incorrect field in FXAPIActivityDefinition when converting to a JSON Object.
### Adds simplified functionality to try to fill various field types from the json object.
### Adds a TopTenData type exposed for displaying users Top Ten score data.

## [1.1.1] - 2022-09-14
### Fixes up TopTenData fetching for newly structured data.
### Adds ability to get Top Ten for the current user.
### Adds scenario and org unit parameters to Fetch Top Ten.

## [1.1.2] - 2022-11-28
### Creates Optional types that wrap TOptional types needed for XAPI.
### Adds library function support for Optional types.
### Exposes all Optional types as properties.

## [1.1.3] - 2023-02-06
### Adds a plugin Property that syncs Module version with the Project version and Android Release version.

## [1.1.4] - 2023-02-17
### Changes if LessonStatus is 'passed' or 'failed' based on Result::Status instead of Result::Completion.

## [1.1.5] - 2023-02-21
### Changed to bSyncVersion to synchronize the full module version instead of the shortened version.
### Added extended property fields to be synchronized, currently for Android Store Version and the PixoVR plugin Module Version.

## [1.1.6] - 2023-03-02
### Added editor only build code to wrap settings and prevent Windows build error when Android isn't configured.
### Removed the "EngineVersion" from the uplugin to prevent the warning.

## [1.2.0] - 2023-04-11
### Adds a hook-up to the websocket system allowing access to Single Sign On (SSO) responses.
### Adds a function to retrieve SSO Pin.
### Adds callback when the SSO Pin has been successfully entered and the user is logged in.
### Adds the function for Logout. Currently only clears internal information for the current logged in user.
### Adds a flag to Login to logout the previous user someone else logs in.
### Changes the Version to a single string, with the PixoVR Versioning enforced.
### Updates Failure Responses to add the HttpCode string field.
### Adds the documentation system.

## [1.3.0] - 2023-11-10
### Changes the Login function to take a username and password, instead of FApexLoginInfo.
### Removed the FApexLoginInfo structure.
### Adds the ApexSDKLibrary that exposes variables from the ApexSDK without the need to access the subsystem directly.
### Cleans up internal dependencies.

## [1.3.1] - 2024-08-12
### Fixes a bug with handling platform errors.
### Integrates the ManageXR plugin to get headset information when a headset is registered with ManageXR.
### Passes the devices information in the user access check when available.
### Adds a heartbeat process to end sessions when a user closes the app without ending the session.

## [2.0.0] - Unreleased
### Rebuilt on apex-sdk-core 0.2.0, compiled from source in the new ApexCore module. Every platform call, session event and telemetry packet goes through the core; VaRest and the xAPI types are gone.
### UApexSubsystem replaces UApexAPI. It adds token, Quick ID and login-code logins, module lists and pages with lifecycles, data or xAPI sessions, environments (NA production, staging and development, Saudi production, local, custom), launch arguments from the command line, pixovr:// links and Android intents, and the launcher hand-off.
### Blueprint async nodes for every call, with Success and Failure pins and typed results.
### Brings in the UE5 plugins: the launcher (install, update, uninstall, launch, intents, shared files, ManageXR serial number, Quest/Pico/HTC installers) from pixo-unreal-core, the authorization-code login and module launch from pixo-unreal-auth, and sessions from pixo-unreal-datareporting.
### Adds the ApexAnalytics module: tracked objects, interactables, gaze tracking (head and eye tracker), spatial sampling, steps, a drop-in session component, and analytics providers, with the built-in PixoVR provider sending the Unity SDK 2.0's telemetry format.
### Removes the reporting queries (FetchTopTen); see docs/MIGRATING.md.

