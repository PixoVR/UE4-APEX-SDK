// Copyright 2022 PixoVR Corp. All Rights Reserved.

#include "PixoDataReportingFunctionLibrary.h"

#include "Interfaces/IPluginManager.h"
#include "XAPITypes.h"

FXAPIExtension UPDataReportingFunctionLibrary::CreateExtensionData(const TMap<FString, UVaRestJsonValue*>& Data)
{
    FXAPIExtension Extension;

    for (const TTuple<FString, UVaRestJsonValue*>& ValuePair : Data)
    {
        Extension.Add(ValuePair.Key, ValuePair.Value->GetRootValue());
    }

    return Extension;
}

FString UPDataReportingFunctionLibrary::GetDataReportingVersion()
{
    const auto PluginRef = IPluginManager::Get().FindPlugin("PixoDataReporting");

    return !PluginRef.IsValid() ? FString("invalid") : PluginRef->GetDescriptor().VersionName;
}