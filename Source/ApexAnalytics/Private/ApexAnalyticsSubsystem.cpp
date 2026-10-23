// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "ApexAnalyticsSubsystem.h"

#include "ApexSDKSettings.h"
#include "ApexSubsystem.h"
#include "ApexTrackedObjectComponent.h"
#include "PixoVRAnalyticsProvider.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CoreDelegates.h"

DEFINE_LOG_CATEGORY_STATIC(LogApexAnalytics, Log, All);

UApexAnalyticsSubsystem* UApexAnalyticsSubsystem::Get(const UObject* worldContextObject)
{
	UWorld* world = GEngine != nullptr
						? GEngine->GetWorldFromContextObject(worldContextObject, EGetWorldErrorMode::ReturnNull)
						: nullptr;
	UGameInstance* gameInstance = world != nullptr ? world->GetGameInstance() : nullptr;
	return gameInstance != nullptr ? gameInstance->GetSubsystem<UApexAnalyticsSubsystem>() : nullptr;
}

void UApexAnalyticsSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);
	// InitializeDependency's return type differs between engine versions; ask the game instance instead.
	collection.InitializeDependency(UApexSubsystem::StaticClass());
	UApexSubsystem* apexSubsystem = GetGameInstance()->GetSubsystem<UApexSubsystem>();
	m_apexSubsystem = apexSubsystem;
	if (apexSubsystem != nullptr)
	{
		apexSubsystem->OnUserIdentifiedNative.AddUObject(this, &UApexAnalyticsSubsystem::HandleUserIdentified);
		apexSubsystem->OnSessionJoinedNative.AddUObject(this, &UApexAnalyticsSubsystem::HandleSessionJoined);
		apexSubsystem->OnSessionEventNative.AddUObject(this, &UApexAnalyticsSubsystem::HandleSessionEvent);
		apexSubsystem->OnSessionCompletedNative.AddUObject(this, &UApexAnalyticsSubsystem::HandleSessionCompleted);
	}

	m_backgroundDelegateHandle = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(
		this, &UApexAnalyticsSubsystem::HandleApplicationBackground);

	if (GetDefault<UApexSDKSettings>()->IsPixoVRProviderEnabled() && apexSubsystem != nullptr)
	{
		RegisterProvider(MakeShared<FPixoVRAnalyticsProvider>(*apexSubsystem, *this));
	}
}

void UApexAnalyticsSubsystem::Deinitialize()
{
	Flush();
	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(m_backgroundDelegateHandle);
	if (UApexSubsystem* apexSubsystem = m_apexSubsystem.Get())
	{
		apexSubsystem->OnUserIdentifiedNative.RemoveAll(this);
		apexSubsystem->OnSessionJoinedNative.RemoveAll(this);
		apexSubsystem->OnSessionEventNative.RemoveAll(this);
		apexSubsystem->OnSessionCompletedNative.RemoveAll(this);
	}

	m_providers.Empty();
	m_trackedObjects.Empty();
	Super::Deinitialize();
}

void UApexAnalyticsSubsystem::Tick(float deltaSeconds)
{
	Dispatch([deltaSeconds](IApexAnalyticsProvider& provider)
	{
		provider.Tick(deltaSeconds);
	});
}

TStatId UApexAnalyticsSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UApexAnalyticsSubsystem, STATGROUP_Tickables);
}

bool UApexAnalyticsSubsystem::IsTickable() const
{
	return !IsTemplate() && m_providers.Num() > 0;
}

bool UApexAnalyticsSubsystem::IsTickableWhenPaused() const
{
	return false;
}

// ---------------------------------------------------------------- Providers

bool UApexAnalyticsSubsystem::RegisterProvider(const TSharedRef<IApexAnalyticsProvider>& provider)
{
	const FString providerName = provider->GetName();
	for (const TSharedRef<IApexAnalyticsProvider>& registeredProvider : m_providers)
	{
		if (registeredProvider == provider || registeredProvider->GetName() == providerName)
		{
			UE_LOG(LogApexAnalytics, Warning, TEXT("Provider '%s' is already registered."), *providerName);
			return false;
		}
	}

	const UApexSDKSettings* settings = GetDefault<UApexSDKSettings>();
	if (!settings->bAnalyticsEnabled)
	{
		UE_LOG(LogApexAnalytics, Log, TEXT("Analytics are disabled by the settings."));
		return false;
	}

	if (settings->DisabledProviders.Contains(providerName))
	{
		UE_LOG(LogApexAnalytics, Log, TEXT("Provider '%s' is disabled by the settings."), *providerName);
		return false;
	}

	m_providers.Add(provider);
	for (UApexTrackedObjectComponent* trackedObject : GetTrackedObjects())
	{
		provider->OnTrackedObjectRegistered(*trackedObject);
	}

	UE_LOG(LogApexAnalytics, Log, TEXT("Provider '%s' registered."), *providerName);
	return true;
}

bool UApexAnalyticsSubsystem::UnregisterProvider(const TSharedRef<IApexAnalyticsProvider>& provider)
{
	return m_providers.Remove(provider) > 0;
}

const TArray<TSharedRef<IApexAnalyticsProvider>>& UApexAnalyticsSubsystem::GetProviders() const
{
	return m_providers;
}

void UApexAnalyticsSubsystem::Dispatch(TFunctionRef<void(IApexAnalyticsProvider& provider)> providerCall)
{
	// A copy: a provider may register or unregister providers while it is called.
	const TArray<TSharedRef<IApexAnalyticsProvider>> providers = m_providers;
	for (const TSharedRef<IApexAnalyticsProvider>& provider : providers)
	{
		providerCall(*provider);
	}
}

FApexAnalyticsContext UApexAnalyticsSubsystem::MakeContext() const
{
	FApexAnalyticsContext context;
	const UApexSubsystem* apexSubsystem = m_apexSubsystem.Get();
	if (apexSubsystem == nullptr)
	{
		return context;
	}

	const FApexUser user = apexSubsystem->GetCurrentUser();
	context.UserId = user.UserId;
	context.OrganizationId = user.OrganizationId;
	context.UserEmail = user.Email;
	context.ModuleId = apexSubsystem->GetModuleId();
	context.ModuleName = apexSubsystem->GetModuleName();
	context.ModuleVersion = apexSubsystem->GetModuleVersion();
	context.ScenarioId = apexSubsystem->GetScenarioId();
	context.SessionId = apexSubsystem->GetSessionId();
	context.DeviceId = apexSubsystem->GetDeviceId();
	context.DeviceModel = apexSubsystem->GetDeviceModel();
	context.DeviceSerial = apexSubsystem->GetDeviceSerialNumber();
	context.Platform = apexSubsystem->GetPlatformName();
	context.SdkVersion = UApexSubsystem::GetSdkVersion();
	return context;
}

void UApexAnalyticsSubsystem::Flush()
{
	Dispatch([](IApexAnalyticsProvider& provider)
	{
		provider.Flush();
	});
}

// ---------------------------------------------------------------- Steps

bool UApexAnalyticsSubsystem::IsStepNameValid(const FString& stepName, const TCHAR* actionName) const
{
	if (stepName.IsEmpty())
	{
		UE_LOG(LogApexAnalytics, Warning, TEXT("Cannot %s an empty step."), actionName);
		return false;
	}

	return true;
}

void UApexAnalyticsSubsystem::BeginStep(const FString& stepName)
{
	if (!IsStepNameValid(stepName, TEXT("begin")))
	{
		return;
	}

	const FApexAnalyticsContext context = MakeContext();
	Dispatch([&context, &stepName](IApexAnalyticsProvider& provider)
	{
		provider.OnStepBegin(context, stepName);
	});
	OnStepBegan.Broadcast(stepName);
}

void UApexAnalyticsSubsystem::CompleteStep(const FString& stepName, float score)
{
	if (!IsStepNameValid(stepName, TEXT("complete")))
	{
		return;
	}

	const FApexAnalyticsContext context = MakeContext();
	Dispatch([&context, &stepName, score](IApexAnalyticsProvider& provider)
	{
		provider.OnStepEnd(context, stepName, true, score);
	});
	OnStepEnded.Broadcast(stepName, true, score);
}

void UApexAnalyticsSubsystem::FailStep(const FString& stepName, float score)
{
	if (!IsStepNameValid(stepName, TEXT("fail")))
	{
		return;
	}

	const FApexAnalyticsContext context = MakeContext();
	Dispatch([&context, &stepName, score](IApexAnalyticsProvider& provider)
	{
		provider.OnStepEnd(context, stepName, false, score);
	});
	OnStepEnded.Broadcast(stepName, false, score);
}

// ---------------------------------------------------------------- Tracked objects and the head

TArray<UApexTrackedObjectComponent*> UApexAnalyticsSubsystem::GetTrackedObjects() const
{
	TArray<UApexTrackedObjectComponent*> trackedObjects;
	for (const TWeakObjectPtr<UApexTrackedObjectComponent>& trackedObject : m_trackedObjects)
	{
		if (trackedObject.IsValid())
		{
			trackedObjects.Add(trackedObject.Get());
		}
	}

	return trackedObjects;
}

UApexTrackedObjectComponent* UApexAnalyticsSubsystem::FindTrackedObject(const FString& trackedId) const
{
	for (UApexTrackedObjectComponent* trackedObject : GetTrackedObjects())
	{
		if (trackedObject->GetTrackedId() == trackedId)
		{
			return trackedObject;
		}
	}

	return nullptr;
}

void UApexAnalyticsSubsystem::RegisterTrackedObject(UApexTrackedObjectComponent& trackedObject)
{
	if (m_trackedObjects.Contains(&trackedObject))
	{
		return;
	}

	m_trackedObjects.Add(&trackedObject);
	Dispatch([&trackedObject](IApexAnalyticsProvider& provider)
	{
		provider.OnTrackedObjectRegistered(trackedObject);
	});
	OnTrackedObjectRegistered.Broadcast(&trackedObject);
}

void UApexAnalyticsSubsystem::UnregisterTrackedObject(UApexTrackedObjectComponent& trackedObject)
{
	if (m_trackedObjects.Remove(&trackedObject) == 0)
	{
		return;
	}

	Dispatch([&trackedObject](IApexAnalyticsProvider& provider)
	{
		provider.OnTrackedObjectUnregistered(trackedObject);
	});
	OnTrackedObjectUnregistered.Broadcast(&trackedObject);
}

void UApexAnalyticsSubsystem::ReportEngagementBegin(UApexTrackedObjectComponent& trackedObject,
													const FString& engagement)
{
	Dispatch([&trackedObject, &engagement](IApexAnalyticsProvider& provider)
	{
		provider.OnEngagementBegin(trackedObject, engagement);
	});
	OnEngagementBegan.Broadcast(&trackedObject, engagement);
}

void UApexAnalyticsSubsystem::ReportEngagementEnd(UApexTrackedObjectComponent& trackedObject, const FString& engagement)
{
	Dispatch([&trackedObject, &engagement](IApexAnalyticsProvider& provider)
	{
		provider.OnEngagementEnd(trackedObject, engagement);
	});
	OnEngagementEnded.Broadcast(&trackedObject, engagement);
}

void UApexAnalyticsSubsystem::ReportInteraction(UApexTrackedObjectComponent& trackedObject, const FString& action)
{
	Dispatch([&trackedObject, &action](IApexAnalyticsProvider& provider)
	{
		provider.OnInteraction(trackedObject, action);
	});
	OnInteraction.Broadcast(&trackedObject, action);
}

void UApexAnalyticsSubsystem::SetHeadComponent(USceneComponent* headComponent)
{
	m_headComponent = headComponent;
}

USceneComponent* UApexAnalyticsSubsystem::GetHeadComponent() const
{
	return m_headComponent.Get();
}

bool UApexAnalyticsSubsystem::GetHeadTransform(FTransform& headTransform) const
{
	if (const USceneComponent* headComponent = m_headComponent.Get())
	{
		headTransform = headComponent->GetComponentTransform();
		return true;
	}

	const UGameInstance* gameInstance = GetGameInstance();
	const UWorld* world = gameInstance != nullptr ? gameInstance->GetWorld() : nullptr;
	const APlayerController* playerController = world != nullptr ? world->GetFirstPlayerController() : nullptr;
	if (playerController != nullptr && playerController->PlayerCameraManager != nullptr)
	{
		headTransform = FTransform(playerController->PlayerCameraManager->GetCameraRotation(),
								   playerController->PlayerCameraManager->GetCameraLocation());
		return true;
	}

	return false;
}

// ---------------------------------------------------------------- Apex events

void UApexAnalyticsSubsystem::HandleUserIdentified(const FApexUser& user)
{
	const FApexAnalyticsContext context = MakeContext();
	Dispatch([&context, &user](IApexAnalyticsProvider& provider)
	{
		provider.OnUserIdentified(context, user);
	});
}

void UApexAnalyticsSubsystem::HandleSessionJoined(int64 sessionId)
{
	const FApexAnalyticsContext context = MakeContext();
	Dispatch([&context, sessionId](IApexAnalyticsProvider& provider)
	{
		provider.OnSessionJoined(context, sessionId);
	});
}

void UApexAnalyticsSubsystem::HandleSessionEvent(const FString& action, const FString& targetObject,
												 const FString& eventDataJson)
{
	const FApexAnalyticsContext context = MakeContext();
	Dispatch([&](IApexAnalyticsProvider& provider)
	{
		provider.OnSessionEvent(context, action, targetObject, eventDataJson);
	});
}

void UApexAnalyticsSubsystem::HandleSessionCompleted(const FApexSessionResult& sessionResult)
{
	const FApexAnalyticsContext context = MakeContext();
	Dispatch([&context, &sessionResult](IApexAnalyticsProvider& provider)
	{
		provider.OnSessionCompleted(context, sessionResult);
	});
}

void UApexAnalyticsSubsystem::HandleApplicationBackground()
{
	Flush();
}
