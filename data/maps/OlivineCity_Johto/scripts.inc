OlivineCity_Johto_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, OlivineCity_Johto_OnTransition
	.byte 0

OlivineCity_Johto_OnTransition::
	setflag FLAG_WORLD_MAP_OLIVINE_CITY
	end

OlivineCity_Johto_EventScript_ConventionFan0::
	msgbox OlivineCity_Johto_EventScript_ConventionFan0_Text, MSGBOX_NPC
	end

OlivineCity_Johto_EventScript_ConventionFan0_Text:
	.string "The ferry's been packed with\nLEADERS all week.\pMISTY got seasick... Don't tell\nher I told you!$"

OlivineCity_Johto_EventScript_ConventionFan1::
	msgbox OlivineCity_Johto_EventScript_ConventionFan1_Text, MSGBOX_NPC
	end

OlivineCity_Johto_EventScript_ConventionFan1_Text:
	.string "JASMINE's so shy, but her\nSTEELIX is huge!\pI hope she does well at the\nCONVENTION.$"
