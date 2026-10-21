@ ExoticSafari_GalarStadium (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/galar.py)
ExoticSafari_GalarStadium_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_GalarStadium_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_GalarStadium_OnLoad
	.byte 0

ExoticSafari_GalarStadium_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_GalarStadium_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_GalarStadium_EventScript_ToPitch::
	warpsilent MAP_EXOTIC_SAFARI_GALAR_STADIUM_PITCH, 0
	waitstate
	end

ExoticSafari_GalarStadium_EventScript_ItemCrackedPot::
	finditem ITEM_CRACKED_POT
	end
