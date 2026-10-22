@ ExoticSafari_SinnohMeadow (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/sinnoh.py)
ExoticSafari_SinnohMeadow_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_SinnohMeadow_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_SinnohMeadow_OnLoad
	.byte 0

ExoticSafari_SinnohMeadow_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_SinnohMeadow_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_SinnohMeadow_EventScript_ToLake::
	warpsilent MAP_EXOTIC_SAFARI_SINNOH_LAKE, 3
	waitstate
	end
