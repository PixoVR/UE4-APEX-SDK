// Copyright 2022 PixoVR Corp. All Rights Reserved.

#include "PixoDataReportingModule.h"
#include "PixoDataReportingFunctionLibrary.h"
#include "ApexSDKDefines.h"

#include "UObject/Package.h"

#define LOCTEXT_NAMESPACE "FPixoDataReportingModule"

DEFINE_LOG_CATEGORY_STATIC(LogPDataReportingModule, Log, All);

#define Log(pmt, ...) UE_LOG(LogPDataReportingModule, Log, TEXT(pmt), ##__VA_ARGS__)
#define Warn(pmt, ...) UE_LOG(LogPDataReportingModule, Warning, TEXT(pmt), ##__VA_ARGS__)
#define Fatal(pmt, ...) UE_LOG(LogPDataReportingModule, Fatal, TEXT(pmt), ##__VA_ARGS__)

void FPixoDataReportingModule::StartupModule()
{
	Log("%s: Pixo Data Reporting (%s) module started", *APEX_FUNC_LINE, *UPDataReportingFunctionLibrary::GetDataReportingVersion());
}

void FPixoDataReportingModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FPixoDataReportingModule, PixoDataReporting)

#undef Log
#undef Warn
#undef Fatal

#undef LOCTEXT_NAMESPACE