// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ApexTypes.h"
#include "Components/ActorComponent.h"
#include "ApexSessionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FApexSessionComponentJoinedDelegate, int64, SessionId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FApexSessionComponentFailedDelegate, const FApexResponse&, Response);

/**
 * A session with no wiring: on BeginPlay it logs in with the launcher's token if no one is logged in,
 * then joins a session for its scenario; on EndPlay it completes the session if it is still running.
 * Put it on an actor in the level the scenario plays in (or on the game mode). Call CompleteSession
 * yourself to report a score; EndPlay then has nothing left to do.
 */
UCLASS(ClassGroup = "Apex", meta = (BlueprintSpawnableComponent, DisplayName = "Apex Session"))
class APEXANALYTICS_API UApexSessionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UApexSessionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	/** Logs in if needed, then joins. BeginPlay does this when Join On Begin Play is set. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Sessions")
	void StartSession();

	/** Completes the session with a result; resultDataJson is a JSON object or empty. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Sessions")
	void CompleteSession(const FApexSessionResult& sessionResult, const FString& resultDataJson);

	UPROPERTY(BlueprintAssignable, Category = "Apex|Sessions")
	FApexSessionComponentJoinedDelegate OnSessionJoined;

	/** The login, the join or the completion failed. */
	UPROPERTY(BlueprintAssignable, Category = "Apex|Sessions")
	FApexSessionComponentFailedDelegate OnSessionFailed;

private:
	void Join();

	/** Empty keeps the scenario already set (Default Scenario ID in the settings). */
	UPROPERTY(EditAnywhere, Category = "Apex", meta = (DisplayName = "Scenario ID"))
	FString m_scenarioId;

	/** A JSON object sent with the join, e.g. {"difficulty":"hard"}; may be empty. */
	UPROPERTY(EditAnywhere, Category = "Apex", meta = (DisplayName = "Session Data JSON"))
	FString m_sessionDataJson;

	UPROPERTY(EditAnywhere, Category = "Apex", meta = (DisplayName = "Join On Begin Play"))
	bool m_joinOnBeginPlay;

	/** Logs in with the launcher's token ("pixotoken") when no one is logged in. */
	UPROPERTY(EditAnywhere, Category = "Apex", meta = (DisplayName = "Login With Launch Token"))
	bool m_loginWithLaunchToken;

	/** Completes a running session at EndPlay with Result At End Play. */
	UPROPERTY(EditAnywhere, Category = "Apex", meta = (DisplayName = "Complete On End Play"))
	bool m_completeOnEndPlay;

	UPROPERTY(EditAnywhere, Category = "Apex", meta = (DisplayName = "Result At End Play"))
	FApexSessionResult m_resultAtEndPlay;
};
