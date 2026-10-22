@ ExoticSafari_KalosWoods (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/kalos.py)
ExoticSafari_KalosWoods_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_KalosWoods_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_KalosWoods_OnLoad
	.byte 0

ExoticSafari_KalosWoods_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_KalosWoods_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_KalosWoods_EventScript_ToCity::
	warpsilent MAP_EXOTIC_SAFARI_KALOS_CITY, 2
	waitstate
	end

ExoticSafari_KalosWoods_EventScript_ToSnow::
	warpsilent MAP_EXOTIC_SAFARI_KALOS_SNOWY_PATH, 0
	waitstate
	end

ExoticSafari_KalosWoods_EventScript_Cell0::
	lock
	setvar VAR_0x8004, 1
	goto ExoticSafari_EventScript_Cell

ExoticSafari_KalosWoods_EventScript_Cell1::
	lock
	setvar VAR_0x8004, 2
	goto ExoticSafari_EventScript_Cell
