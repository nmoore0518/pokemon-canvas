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

AzaleaTown_Johto_EventScript_ConventionFan0::
	msgbox AzaleaTown_Johto_EventScript_ConventionFan0_Text, MSGBOX_NPC
	end

AzaleaTown_Johto_EventScript_ConventionFan0_Text:
	.string "KURT says he's making special\nPOKé BALLS for the LEADERS.\pMostly he's been napping.$"

AzaleaTown_Johto_EventScript_ConventionFan1::
	msgbox AzaleaTown_Johto_EventScript_ConventionFan1_Text, MSGBOX_NPC
	end

AzaleaTown_Johto_EventScript_ConventionFan1_Text:
	.string "I trained my bugs all year for\nthe CONVENTION!\p...Turns out only LEADERS get to\nbattle there.$"
