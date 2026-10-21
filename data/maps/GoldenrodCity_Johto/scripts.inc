GoldenrodCity_Johto_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, GoldenrodCity_Johto_OnTransition
	.byte 0

GoldenrodCity_Johto_OnTransition::
	setflag FLAG_WORLD_MAP_GOLDENROD_CITY
	end

GoldenrodCity_Johto_EventScript_ConventionFan0::
	msgbox GoldenrodCity_Johto_EventScript_ConventionFan0_Text, MSGBOX_NPC
	end

GoldenrodCity_Johto_EventScript_ConventionFan0_Text:
	.string "The CHAMPIONS are all here!\nCYNTHIA, STEVEN, LANCE, LEON...\pGoldenrod has never been this\nbusy!$"

GoldenrodCity_Johto_EventScript_ConventionFan1::
	msgbox GoldenrodCity_Johto_EventScript_ConventionFan1_Text, MSGBOX_NPC
	end

GoldenrodCity_Johto_EventScript_ConventionFan1_Text:
	.string "I heard there's a TROPHY for\nbeating everyone in a hall.\pClear every hall and you get a\nGRAND TROPHY!$"

GoldenrodCity_Johto_EventScript_ConventionFan2::
	msgbox GoldenrodCity_Johto_EventScript_ConventionFan2_Text, MSGBOX_NPC
	end

GoldenrodCity_Johto_EventScript_ConventionFan2_Text:
	.string "In my day there were only eight\nLEADERS.\pNow there's a whole hall for\nevery type!$"

GoldenrodCity_Johto_EventScript_ConventionFan3::
	msgbox GoldenrodCity_Johto_EventScript_ConventionFan3_Text, MSGBOX_NPC
	end

GoldenrodCity_Johto_EventScript_ConventionFan3_Text:
	.string "BUGSY's going to show those\nGalar LEADERS what bug POKéMON\pcan do!$"

GoldenrodCity_Johto_EventScript_ConventionFan4::
	msgbox GoldenrodCity_Johto_EventScript_ConventionFan4_Text, MSGBOX_NPC
	end

GoldenrodCity_Johto_EventScript_ConventionFan4_Text:
	.string "I came all the way from Hoenn to\nsee WALLACE.\pHe's so elegant, even when he\nloses!$"
