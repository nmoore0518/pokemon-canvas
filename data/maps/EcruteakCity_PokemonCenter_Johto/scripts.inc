EcruteakCity_PokemonCenter_Johto_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, EcruteakCity_PokemonCenter_Johto_OnTransition
	.byte 0

EcruteakCity_PokemonCenter_Johto_OnTransition::
	setrespawn HEAL_LOCATION_ECRUTEAK_CITY
	end

EcruteakCity_PokemonCenter_Johto_EventScript_ConventionFan0::
	msgbox EcruteakCity_PokemonCenter_Johto_EventScript_ConventionFan0_Text, MSGBOX_NPC
	end

EcruteakCity_PokemonCenter_Johto_EventScript_ConventionFan0_Text:
	.string "SABRINA knew I'd walk in here\nbefore I even did.\pCreepy... but cool!$"
