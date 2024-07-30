// Copyright 2022 PixoVR Corp. All Rights Reserved.

#include "ApexLibrary.h"
#include "ApexAPI.h"
#include "Interfaces/IPluginManager.h"

#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#endif

FXAPIExtension UApexLibrary::CreateExtensionData(const TMap<FString, UVaRestJsonValue*>& Data)
{
    FXAPIExtension Extension;

    for (const TTuple<FString, UVaRestJsonValue*>& ValuePair : Data)
    {
        Extension.Add(ValuePair.Key, ValuePair.Value->GetRootValue());
    }

    return Extension;
}

UVaRestJsonValue* UApexLibrary::JsonValueFromString(FString String)
{
    TSharedPtr<FJsonValue> SharedValue = MakeShared<FJsonValueString>(String);
    UVaRestJsonValue* RestJsonValue = NewObject<UVaRestJsonValue>();
    RestJsonValue->SetRootValue(SharedValue);

    return RestJsonValue;
}

FString UApexLibrary::GetApexSDKVersion()
{
    const auto PluginRef = IPluginManager::Get().FindPlugin("ApexSDK");

    return !PluginRef.IsValid() ? FString("invalid") : PluginRef->GetDescriptor().VersionName;
}