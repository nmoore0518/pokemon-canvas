@ ExoticSafari_PaldeaForest (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/paldea.py)
ExoticSafari_PaldeaForest_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_PaldeaForest_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_PaldeaForest_OnLoad
	.byte 0

ExoticSafari_PaldeaForest_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_PaldeaForest_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_PaldeaForest_EventScript_ToMeadow::
	warpsilent MAP_EXOTIC_SAFARI_PALDEA_MEADOW, 3
	waitstate
	end

ExoticSafari_PaldeaForest_EventScript_ToDesert::
	warpsilent MAP_EXOTIC_SAFARI_PALDEA_DESERT, 0
	waitstate
	end

ExoticSafari_PaldeaForest_EventScript_LegendWoChien::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_WO_CHIEN
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_WO_CHIEN
	goto ExoticSafari_EventScript_Release

ExoticSafari_PaldeaForest_EventScript_SealWoChien::
	lockall
	setvar VAR_0x8004, 1
	goto ExoticSafari_EventScript_Seal
