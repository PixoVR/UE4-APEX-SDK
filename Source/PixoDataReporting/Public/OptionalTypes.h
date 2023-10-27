// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OptionalTypes.generated.h"

USTRUCT(BlueprintType)
struct PIXODATAREPORTING_API FOptionalInt
{
    GENERATED_BODY()

    TOptional<int> OptionalValue;
public:

    virtual ~FOptionalInt() {}

    int Get() const
    {
        if (OptionalValue.IsSet())
        {
            return *OptionalValue;
        }
        return 0;
    }

    int Get(bool& IsSet)
    {
        IsSet = OptionalValue.IsSet();
        if (IsSet)
        {
            return *OptionalValue;
        }
        return 0;
    }

    void Set(int Value)
    {
        OptionalValue = Value;
    }

    bool IsSet() const
    {
        return OptionalValue.IsSet();
    }

    virtual int operator*()
    {
        bool IsSet;
        return Get(IsSet);
    }

    virtual void operator=(const int& Value)
    {
        OptionalValue = Value;
    }
};

USTRUCT(BlueprintType)
struct PIXODATAREPORTING_API FOptionalFloat
{
    GENERATED_BODY()

    TOptional<float> OptionalValue;
public:

    virtual ~FOptionalFloat() {}

    float Get() const
    {
        if (OptionalValue.IsSet())
        {
            return *OptionalValue;
        }
        return 0;
    }

    float Get(bool& IsSet)
    {
        IsSet = OptionalValue.IsSet();
        if (IsSet)
        {
            return *OptionalValue;
        }
        return 0;
    }

    void Set(float Value)
    {
        OptionalValue = Value;
    }

    bool IsSet() const
    {
        return OptionalValue.IsSet();
    }

    virtual float operator*()
    {
        bool IsSet;
        return Get(IsSet);
    }

    virtual void operator=(const float& Value)
    {
        OptionalValue = Value;
    }
};

USTRUCT(BlueprintType)
struct PIXODATAREPORTING_API FOptionalBool
{
    GENERATED_BODY()

    TOptional<bool> OptionalValue;
public:

    virtual ~FOptionalBool() {}

    bool Get() const
    {
        if (OptionalValue.IsSet())
        {
            return *OptionalValue;
        }
        return 0;
    }

    bool Get(bool& IsSet)
    {
        IsSet = OptionalValue.IsSet();
        if (IsSet)
        {
            return *OptionalValue;
        }
        return 0;
    }

    void Set(bool Value)
    {
        OptionalValue = Value;
    }

    bool IsSet() const
    {
        return OptionalValue.IsSet();
    }

    virtual bool operator*()
    {
        bool IsSet;
        return Get(IsSet);
    }

    virtual void operator=(const bool& Value)
    {
        OptionalValue = Value;
    }
};