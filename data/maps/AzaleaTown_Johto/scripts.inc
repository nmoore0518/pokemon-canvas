AzaleaTown_Johto_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, AzaleaTown_Johto_OnTransition
	.byte 0

AzaleaTown_Johto_OnTransition::
	setflag FLAG_WORLD_MAP_AZALEA_TOWN
	end

AzaleaTown_Johto_EventScript_Border0::
	setvar VAR_0x8004, LOCALID_AZALEA_TOWN_GUARD_0
	setvar VAR_0x8005, DIR_WEST
	goto Johto_EventScript_BorderGuard
