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

## [1.1.2] - 2022-
### Creates Optional types that wrap TOptional types needed for XAPI.
### Adds library function support for Optional types.
### Exposes all Optional types as properties.