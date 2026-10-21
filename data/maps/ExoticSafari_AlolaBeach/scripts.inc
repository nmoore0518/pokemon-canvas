@ ExoticSafari_AlolaBeach (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/alola.py)
ExoticSafari_AlolaBeach_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_AlolaBeach_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_AlolaBeach_OnLoad
	.byte 0

ExoticSafari_AlolaBeach_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_AlolaBeach_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_AlolaBeach_EventScript_ToResort::
	warpsilent MAP_EXOTIC_SAFARI_ALOLA_RESORT, 2
	waitstate
	end

ExoticSafari_AlolaBeach_EventScript_LegendCosmog::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_COSMOG
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_COSMOG
	goto ExoticSafari_EventScript_Release
