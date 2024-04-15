#include "FortGameStateZone.h"
#include "reboot.h"

UClass* AFortGameStateZone::StaticClass()
{
	static auto Class = FindObject<UClass>(L"/Script/FortniteGame.FortGameStateZone");
	return Class;
}