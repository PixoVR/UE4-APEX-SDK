// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ApexTypes.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "ApexAsyncActions.generated.h"

class UApexSubsystem;

/** Shared plumbing: finds the subsystem, keeps the node alive until its call has a result. */
UCLASS(Abstract)
class APEXSDK_API UApexAsyncActionBase : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

protected:
	void Prepare(UObject* worldContextObject);
	/** The subsystem, or nullptr after reporting the failure through ReportMissingSubsystem. */
	UApexSubsystem* GetSubsystemOrFail();
	virtual void ReportMissingSubsystem(const FApexResponse& response)
	{
	}

	TWeakObjectPtr<UObject> m_worldContextObject;
};

enum class EApexLoginKind : uint8
{
	PASSWORD,
	TOKEN,
	QUICK_ID,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FApexAsyncUserPin, const FApexResponse&, Response, const FApexUser&, User);

/** Logs in; with "check module access on login" on, succeeds only after the access check. */
UCLASS()
class APEXSDK_API UApexLoginAsyncAction : public UApexAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Apex|Login",
			  meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject",
					  DisplayName = "Apex Login"))
	static UApexLoginAsyncAction* Login(UObject* worldContextObject, const FString& username, const FString& password);

	/** An empty token uses the launcher's ("pixotoken"). */
	UFUNCTION(BlueprintCallable, Category = "Apex|Login",
			  meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject",
					  DisplayName = "Apex Login With Token"))
	static UApexLoginAsyncAction* LoginWithToken(UObject* worldContextObject, const FString& userToken);

	/** An empty serial number uses this device's. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Login",
			  meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject",
					  DisplayName = "Apex Quick ID Login"))
	static UApexLoginAsyncAction* QuickIdLogin(UObject* worldContextObject, const FString& username,
											   const FString& serialNumber);

	virtual void Activate() override;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncUserPin OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncUserPin OnFailure;

protected:
	virtual void ReportMissingSubsystem(const FApexResponse& response) override;

private:
	void Finish(const FApexResponse& response, const FApexUser& user);

	EApexLoginKind m_loginKind = EApexLoginKind::PASSWORD;
	FString m_firstText;
	FString m_secondText;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FApexAsyncCodePin, const FString&, AuthorizationCode);

/** Login by code: Code Received gives the code to show, Success follows once the user has entered it. */
UCLASS()
class APEXSDK_API UApexAuthorizationCodeAsyncAction : public UApexAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Apex|Login",
			  meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject",
					  DisplayName = "Apex Login With Authorization Code"))
	static UApexAuthorizationCodeAsyncAction* RequestAuthorizationCode(UObject* worldContextObject);

	virtual void Activate() override;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncCodePin OnCodeReceived;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncUserPin OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncUserPin OnFailure;

protected:
	virtual void ReportMissingSubsystem(const FApexResponse& response) override;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FApexAsyncResponsePin, const FApexResponse&, Response);

/** Calls whose result is only success or failure: ping, session events and completion. */
UCLASS()
class APEXSDK_API UApexRequestAsyncAction : public UApexAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Apex",
			  meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject",
					  DisplayName = "Apex Ping"))
	static UApexRequestAsyncAction* Ping(UObject* worldContextObject);

	/** eventDataJson: a JSON object with the module's data, or empty. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Sessions",
			  meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject",
					  DisplayName = "Apex Send Session Event"))
	static UApexRequestAsyncAction* SendSessionEvent(UObject* worldContextObject, const FString& action,
													 const FString& targetObject, const FString& eventDataJson);

	/** An xAPI statement (JSON with a verb and an object); needs xAPI session events. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Sessions",
			  meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject",
					  DisplayName = "Apex Send Session Statement"))
	static UApexRequestAsyncAction* SendSessionStatement(UObject* worldContextObject, const FString& statementJson);

	/** resultDataJson: a JSON object sent with the result, or empty. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Sessions",
			  meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject",
					  DisplayName = "Apex Complete Session"))
	static UApexRequestAsyncAction* CompleteSession(UObject* worldContextObject,
													const FApexSessionResult& sessionResult,
													const FString& resultDataJson);

	virtual void Activate() override;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncResponsePin OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncResponsePin OnFailure;

protected:
	virtual void ReportMissingSubsystem(const FApexResponse& response) override;

private:
	TFunction<void(UApexSubsystem&, TFunction<void(const FApexResponse&)>)> m_startCall;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FApexAsyncSessionPin, const FApexResponse&, Response, int64, SessionId);

UCLASS()
class APEXSDK_API UApexJoinSessionAsyncAction : public UApexAsyncActionBase
{
	GENERATED_BODY()

public:
	/** scenarioId (when not empty) becomes the session's scenario; sessionDataJson is a JSON object or empty. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Sessions",
			  meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject",
					  DisplayName = "Apex Join Session"))
	static UApexJoinSessionAsyncAction* JoinSession(UObject* worldContextObject, const FString& scenarioId,
													const FString& sessionDataJson);

	virtual void Activate() override;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncSessionPin OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncSessionPin OnFailure;

protected:
	virtual void ReportMissingSubsystem(const FApexResponse& response) override;

private:
	FString m_scenarioId;
	FString m_sessionDataJson;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FApexAsyncModuleAccessPin, const FApexResponse&, Response,
											 const FApexModuleAccess&, ModuleAccess);

UCLASS()
class APEXSDK_API UApexModuleAccessAsyncAction : public UApexAsyncActionBase
{
	GENERATED_BODY()

public:
	/** moduleId < 0 is this module. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Modules",
			  meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject",
					  DisplayName = "Apex Check Module Access"))
	static UApexModuleAccessAsyncAction* CheckModuleAccess(UObject* worldContextObject, int32 moduleId = -1);

	virtual void Activate() override;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncModuleAccessPin OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncModuleAccessPin OnFailure;

protected:
	virtual void ReportMissingSubsystem(const FApexResponse& response) override;

private:
	int32 m_moduleId = -1;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FApexAsyncModuleListPin, const FApexResponse&, Response,
											 const TArray<FApexModule>&, Modules);

UCLASS()
class APEXSDK_API UApexModuleListAsyncAction : public UApexAsyncActionBase
{
	GENERATED_BODY()

public:
	/** The user's modules, with versions built for platformName, Archived left out. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Modules",
			  meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject",
					  DisplayName = "Apex Get Module List"))
	static UApexModuleListAsyncAction* GetModuleList(UObject* worldContextObject, const FString& platformName);

	virtual void Activate() override;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncModuleListPin OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncModuleListPin OnFailure;

protected:
	virtual void ReportMissingSubsystem(const FApexResponse& response) override;

private:
	FString m_platformName;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FApexAsyncModulePagePin, const FApexResponse&, Response,
											 const FApexModulePage&, ModulePage);

UCLASS()
class APEXSDK_API UApexModulePageAsyncAction : public UApexAsyncActionBase
{
	GENERATED_BODY()

public:
	/**
	 * One page of the user's modules. platformName empty: any platform. No lifecycle set: Released.
	 * pageIndex starts at 0. Later pages reuse the first call's list until refresh is set.
	 */
	UFUNCTION(BlueprintCallable, Category = "Apex|Modules",
			  meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject",
					  DisplayName = "Apex Get Module Page"))
	static UApexModulePageAsyncAction* GetModulePage(UObject* worldContextObject, const FString& platformName,
													 UPARAM(meta = (Bitmask, BitmaskEnum = "EApexModuleLifecycle"))
														 int32 lifecycles,
													 EApexModuleSortOrder sortOrder, int32 pageIndex = 0,
													 int32 pageSize = 20, bool refresh = false);

	virtual void Activate() override;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncModulePagePin OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncModulePagePin OnFailure;

protected:
	virtual void ReportMissingSubsystem(const FApexResponse& response) override;

private:
	FString m_platformName;
	int32 m_lifecycles = 0;
	EApexModuleSortOrder m_sortOrder = EApexModuleSortOrder::PLATFORM;
	int32 m_pageIndex = 0;
	int32 m_pageSize = 20;
	bool m_refresh = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FApexAsyncUserModulesPin, const FApexResponse&, Response,
											 const TArray<FApexUserModules>&, UsersModules);

UCLASS()
class APEXSDK_API UApexUserModulesAsyncAction : public UApexAsyncActionBase
{
	GENERATED_BODY()

public:
	/** The module IDs a user can access; userId < 0 is the current user. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Modules",
			  meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject",
					  DisplayName = "Apex Get User Modules"))
	static UApexUserModulesAsyncAction* GetUserModules(UObject* worldContextObject, int32 userId = -1);

	virtual void Activate() override;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncUserModulesPin OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncUserModulesPin OnFailure;

protected:
	virtual void ReportMissingSubsystem(const FApexResponse& response) override;

private:
	int32 m_userId = -1;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FApexAsyncQuickIdUsersPin, const FApexResponse&, Response,
											 const FApexQuickIdUsers&, QuickIdUsers);

UCLASS()
class APEXSDK_API UApexQuickIdUsersAsyncAction : public UApexAsyncActionBase
{
	GENERATED_BODY()

public:
	/** The users enrolled for Quick ID on a device; an empty serial number is this device. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Login",
			  meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject",
					  DisplayName = "Apex Get Quick ID Users"))
	static UApexQuickIdUsersAsyncAction* GetQuickIdUsers(UObject* worldContextObject, const FString& serialNumber);

	virtual void Activate() override;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncQuickIdUsersPin OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncQuickIdUsersPin OnFailure;

protected:
	virtual void ReportMissingSubsystem(const FApexResponse& response) override;

private:
	FString m_serialNumber;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FApexAsyncLoginCodePin, const FApexResponse&, Response,
											 const FApexLoginCode&, LoginCode);

UCLASS()
class APEXSDK_API UApexLoginCodeAsyncAction : public UApexAsyncActionBase
{
	GENERATED_BODY()

public:
	/** A one-time login code; userId < 0 is the current user. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Login",
			  meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject",
					  DisplayName = "Apex Generate Login Code"))
	static UApexLoginCodeAsyncAction* GenerateLoginCode(UObject* worldContextObject, int32 userId = -1);

	virtual void Activate() override;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncLoginCodePin OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FApexAsyncLoginCodePin OnFailure;

protected:
	virtual void ReportMissingSubsystem(const FApexResponse& response) override;

private:
	int32 m_userId = -1;
};
