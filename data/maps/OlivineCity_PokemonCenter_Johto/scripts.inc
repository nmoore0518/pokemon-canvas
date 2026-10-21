OlivineCity_PokemonCenter_Johto_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, OlivineCity_PokemonCenter_Johto_OnTransition
	.byte 0

OlivineCity_PokemonCenter_Johto_OnTransition::
	setrespawn HEAL_LOCATION_OLIVINE_CITY
	end

OlivineCity_PokemonCenter_Johto_EventScript_ConventionFan0::
	msgbox OlivineCity_PokemonCenter_Johto_EventScript_ConventionFan0_Text, MSGBOX_NPC
	end

OlivineCity_PokemonCenter_Johto_EventScript_ConventionFan0_Text:
	.string "My favorite LEADER is CLAIR.\pShe's proud, but nobody battles\nwith dragons like her!$"
