@ ExoticSafari_HoennPacifidlog (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/hoenn.py)
ExoticSafari_HoennPacifidlog_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_HoennPacifidlog_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_HoennPacifidlog_OnLoad
	.byte 0

ExoticSafari_HoennPacifidlog_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_HoennPacifidlog_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_HoennPacifidlog_EventScript_ToForest::
	warpsilent MAP_EXOTIC_SAFARI_HOENN_FORTREE, 1
	waitstate
	end

ExoticSafari_HoennPacifidlog_EventScript_LegendKyogre::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_KYOGRE
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_KYOGRE
	goto ExoticSafari_EventScript_Release

ExoticSafari_HoennPacifidlog_EventScript_SealKyogre::
	lockall
	setvar VAR_0x8004, 43
	goto ExoticSafari_EventScript_Seal

ExoticSafari_HoennPacifidlog_EventScript_ItemSoulDew::
	finditem ITEM_SOUL_DEW
	end
