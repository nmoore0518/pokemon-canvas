@ ExoticSafari_AlolaOcean (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/alola.py)
ExoticSafari_AlolaOcean_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_AlolaOcean_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_AlolaOcean_OnLoad
	.byte 0

ExoticSafari_AlolaOcean_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_AlolaOcean_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_AlolaOcean_EventScript_ToResort::
	warpsilent MAP_EXOTIC_SAFARI_ALOLA_RESORT, 1
	waitstate
	end

ExoticSafari_AlolaOcean_EventScript_ToRuins::
	warpsilent MAP_EXOTIC_SAFARI_ALOLA_RUINS, 0
	waitstate
	end

ExoticSafari_AlolaOcean_EventScript_LegendZeraora::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_ZERAORA
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_ZERAORA
	goto ExoticSafari_EventScript_Release

ExoticSafari_AlolaOcean_EventScript_SealZeraora::
	lockall
	setvar VAR_0x8004, 19
	goto ExoticSafari_EventScript_Seal

ExoticSafari_AlolaOcean_EventScript_Relic1::
	lock
	setvar VAR_0x8004, 1
	goto ExoticSafari_EventScript_Relic
