// Copyright 2022 PixoVR Corp. All Rights Reserved.

#include "PixoDataReporterLibrary.h"
#include "Interfaces/IPluginManager.h"

bool UPDataReporterLibrary::JoinSession(FString InScenarioId, const FXAPIExtension& InContextExtension, bool bIsMultiplayer)
{
    return GetDataReporter()->JoinSession(InScenarioId, InContextExtension, bIsMultiplayer);
}

bool UPDataReporterLibrary::CompleteSession(const FSessionData& InSessionData)
{
    return GetDataReporter()->CompleteSession(InSessionData);
}

bool UPDataReporterLibrary::SendSessionEvent(const FXAPIStatement& InStatement)
{
    return GetDataReporter()->SendSessionEvent(InStatement);
}

bool UPDataReporterLibrary::FetchTopTen(FString Scenario, int OrgUnit)
{
    return GetDataReporter()->FetchTopTen(Scenario, OrgUnit);
}

bool UPDataReporterLibrary::FetchTopTenByUser(FString Scenario, int UserId, int OrgUnit)
{
    return GetDataReporter()->FetchTopTenByUser(Scenario, UserId, OrgUnit);
}

bool UPDataReporterLibrary::FetchTopTenByTimeRange(float TimePriorToToday, FString Scenario, int OrgUnit)
{
    return GetDataReporter()->FetchTopTenByTimeRange(TimePriorToToday, Scenario, OrgUnit);
}