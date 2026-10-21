GoldenrodCity_PokemonCenter_Johto_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, GoldenrodCity_PokemonCenter_Johto_OnTransition
	.byte 0

GoldenrodCity_PokemonCenter_Johto_OnTransition::
	setrespawn HEAL_LOCATION_GOLDENROD_CITY
	end

GoldenrodCity_PokemonCenter_Johto_EventScript_ConventionFan0::
	msgbox GoldenrodCity_PokemonCenter_Johto_EventScript_ConventionFan0_Text, MSGBOX_NPC
	end

GoldenrodCity_PokemonCenter_Johto_EventScript_ConventionFan0_Text:
	.string "The NURSE says she's healed more\nLEADER POKéMON this week than in\pher whole career!$"
