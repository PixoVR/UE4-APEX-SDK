// Copyright 2022 PixoVR Corp. All Rights Reserved.
#pragma once

#define APEX_FUNC (FString(__FUNCTION__))              // Current Class Name + Function Name where this is called
#define APEX_LINE (FString::FromInt(__LINE__))         // Current Line Number in the code where this is called
#define APEX_FUNC_LINE (APEX_FUNC + "(" + APEX_LINE + ")") // Current Class and Line Number where this is called!

// Create a set of delegates and functions for a request
#define CREATE_REQUEST_DELEGATE(NAME) \
	FScriptDelegate On##NAME##CompleteDelegate; \
	FScriptDelegate On##NAME##FailDelegate; \
	UFUNCTION(BlueprintCallable, Category = "Apex|API")\
	void On##NAME##Complete(class UVaRestRequestJSON* Request); \
	UFUNCTION(BlueprintCallable, Category = "Apex|API")\
	void On##NAME##Fail(class UVaRestRequestJSON* Request);

// Easy to use to bind previous added functions to the appropriate script delegates
#define BIND_REQUEST_DELEGATE(NAME) \
	On##NAME##CompleteDelegate.BindUFunction(this, STRING(On##NAME##Complete)); \
	On##NAME##FailDelegate.BindUFunction(this, STRING(On##NAME##Fail)); \

#define STRING(NAME) #NAME

namespace Apex
{
	namespace Verbs
	{
		static const FString JOINED_SESSION = TEXT("https://pixovr.com/xapi/verbs/joined_session");
		static const FString SESSION_EVENT = TEXT("https://pixovr.com/xapi/verbs/session_event");
		static const FString COMPLETED_SESSION = TEXT("https://pixovr.com/xapi/verbs/completed_session");
	}

	namespace Extensions
	{
		static const FString MODULE_IDS = TEXT("https://pixovr.com/xapi/extension/moduleIds");
	}

	namespace EventTypes
	{
		static const FString PIXOVR_SESSION_JOINED = "PIXOVR_SESSION_JOINED";
		static const FString PIXOVR_SESSION_EVENT = "PIXOVR_SESSION_EVENT";
		static const FString PIXOVR_SESSION_COMPLETE = "PIXOVR_SESSION_COMPLETE";
	}

	const FString ISODateTimeFormat = TEXT("o");
}