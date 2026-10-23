// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "ApexTypes.h"

FApexResponse FApexResponse::MakeFailure(EApexErrorCode error, const FString& message)
{
	FApexResponse response;
	response.Error = error;
	response.Message = message;
	return response;
}

float FApexSessionResult::GetEffectiveScaledScore() const
{
	// As the Unity SDK: a missing scaled score is worked out from the score and the maximum.
	const bool computeScaledScore =
		ScaledScore < KINDA_SMALL_NUMBER && Score >= KINDA_SMALL_NUMBER && MaximumScore > KINDA_SMALL_NUMBER;
	return computeScaledScore ? Score / MaximumScore * 100.0f : ScaledScore;
}
