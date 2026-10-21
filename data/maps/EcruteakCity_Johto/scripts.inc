EcruteakCity_Johto_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, EcruteakCity_Johto_OnTransition
	.byte 0

EcruteakCity_Johto_OnTransition::
	setflag FLAG_WORLD_MAP_ECRUTEAK_CITY
	end

EcruteakCity_Johto_EventScript_ConventionFan0::
	msgbox EcruteakCity_Johto_EventScript_ConventionFan0_Text, MSGBOX_NPC
	end

EcruteakCity_Johto_EventScript_ConventionFan0_Text:
	.string "MORTY says the GHOST HALL looks\njust like our BURNED TOWER.\pIt gives me the chills...$"

EcruteakCity_Johto_EventScript_ConventionFan1::
	msgbox EcruteakCity_Johto_EventScript_ConventionFan1_Text, MSGBOX_NPC
	end

EcruteakCity_Johto_EventScript_ConventionFan1_Text:
	.string "The FAIRY HALL is a copy of our\nDANCE THEATER.\pVALERIE fits right in. She moves\nlike a dancer!$"
