#pragma once
#include "FortPlayerState.h"

class AFortPlayerStateZone : public AFortPlayerState
{
public:

	static UClass* StaticClass()
	{
		static auto Class = FindObject<UClass>(L"/Script/FortniteGame.FortPlayerStateZone");
		return Class;
	}
};