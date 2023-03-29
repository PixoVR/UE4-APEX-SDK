// Fill out your copyright notice in the Description page of Project Settings.


#include "ApexLibrary.h"
#include "XAPITypes.h"

FXAPIExtension UApexLibrary::CreateExtensionData(const TMap<FString, UVaRestJsonValue*>& Data)
{
    FXAPIExtension Extension;

    for (const TTuple<FString, UVaRestJsonValue*> ValuePair : Data)
    {
        Extension.Add(ValuePair.Key, ValuePair.Value->GetRootValue());
    }

    return Extension;
}
