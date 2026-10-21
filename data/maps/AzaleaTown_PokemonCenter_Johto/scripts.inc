AzaleaTown_PokemonCenter_Johto_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, AzaleaTown_PokemonCenter_Johto_OnTransition
	.byte 0

AzaleaTown_PokemonCenter_Johto_OnTransition::
	setrespawn HEAL_LOCATION_AZALEA_TOWN
	end

AzaleaTown_PokemonCenter_Johto_EventScript_ConventionFan0::
	msgbox AzaleaTown_PokemonCenter_Johto_EventScript_ConventionFan0_Text, MSGBOX_NPC
	end

AzaleaTown_PokemonCenter_Johto_EventScript_ConventionFan0_Text:
	.string "BROCK and ROXANNE argued about\nrock POKéMON all night.\pThen they went fossil hunting\ntogether!$"
