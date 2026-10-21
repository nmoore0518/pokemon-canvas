@ ExoticSafari_GalarLake (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/galar.py)
ExoticSafari_GalarLake_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_GalarLake_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_GalarLake_OnLoad
	.byte 0

ExoticSafari_GalarLake_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_GalarLake_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_GalarLake_EventScript_ToMeadow::
	warpsilent MAP_EXOTIC_SAFARI_GALAR_MEADOW, 1
	waitstate
	end

ExoticSafari_GalarLake_EventScript_ToForest::
	warpsilent MAP_EXOTIC_SAFARI_GALAR_FOREST, 0
	waitstate
	end

ExoticSafari_GalarLake_EventScript_ToCity::
	warpsilent MAP_EXOTIC_SAFARI_GALAR_CITY, 0
	waitstate
	end

ExoticSafari_GalarLake_EventScript_ToMine::
	warpsilent MAP_EXOTIC_SAFARI_GALAR_MINE, 0
	waitstate
	end

ExoticSafari_GalarLake_EventScript_ItemGalaricaCuff::
	finditem ITEM_GALARICA_CUFF
	end
