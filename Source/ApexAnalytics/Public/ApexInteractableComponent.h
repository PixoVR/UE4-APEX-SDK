// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ApexTrackedObjectComponent.h"
#include "ApexInteractableComponent.generated.h"

/**
 * A tracked object the player handles. Call Grab/Release, HoverEnter/HoverExit and Use from your
 * interaction system (motion controller overlaps, a grab component's events, input actions); each
 * becomes an engagement or interaction, and optionally a session event.
 */
UCLASS(ClassGroup = "Apex", meta = (BlueprintSpawnableComponent, DisplayName = "Apex Interactable"))
class APEXANALYTICS_API UApexInteractableComponent : public UApexTrackedObjectComponent
{
	GENERATED_BODY()

public:
	UApexInteractableComponent();

	UFUNCTION(BlueprintCallable, Category = "Apex|Analytics")
	void Grab();

	UFUNCTION(BlueprintCallable, Category = "Apex|Analytics")
	void Release();

	UFUNCTION(BlueprintCallable, Category = "Apex|Analytics")
	void HoverEnter();

	UFUNCTION(BlueprintCallable, Category = "Apex|Analytics")
	void HoverExit();

	UFUNCTION(BlueprintCallable, Category = "Apex|Analytics")
	void Use();

	UFUNCTION(BlueprintPure, Category = "Apex|Analytics")
	bool IsGrabbed() const;

	UFUNCTION(BlueprintPure, Category = "Apex|Analytics")
	bool IsHovered() const;

	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

private:
	void SendSessionEvent(const FString& action);

	/** Also sends "grab", "release" and "use" as session events while a session is active. */
	UPROPERTY(EditAnywhere, Category = "Apex", meta = (DisplayName = "Send Session Events"))
	bool m_sendSessionEvents;

	bool m_isGrabbed;
	bool m_isHovered;
};
