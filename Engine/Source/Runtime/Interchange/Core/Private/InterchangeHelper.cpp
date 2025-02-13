// Copyright Epic Games, Inc. All Rights Reserved.

#include "InterchangeHelper.h"

namespace UE::Interchange
{
	void SanitizeName(FString& OutName, bool bIsJoint)
	{
		const TCHAR* InvalidChar = bIsJoint ? INVALID_OBJECTNAME_CHARACTERS TEXT("+ ") : INVALID_OBJECTNAME_CHARACTERS;

		while (*InvalidChar)
		{
			if (bIsJoint)
			{
				//For joint we want to replace any space by a dash
				OutName.ReplaceCharInline(TCHAR(' '), TCHAR('-'), ESearchCase::CaseSensitive);
			}
			OutName.ReplaceCharInline(*InvalidChar, TCHAR('_'), ESearchCase::CaseSensitive);
			++InvalidChar;
		}
	}

	FString MakeName(const FString& InName, bool bIsJoint)
	{
		FString TmpName = InName;
		SanitizeName(TmpName, bIsJoint);
		return TmpName;
	}
};