@ ExoticSafari_UnovaGiantChasm (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/unova.py)
ExoticSafari_UnovaGiantChasm_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_UnovaGiantChasm_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_UnovaGiantChasm_OnLoad
	.byte 0

ExoticSafari_UnovaGiantChasm_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_UnovaGiantChasm_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_UnovaGiantChasm_EventScript_LegendCobalion::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_COBALION
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_COBALION
	goto ExoticSafari_EventScript_Release

ExoticSafari_UnovaGiantChasm_EventScript_SealCobalion::
	lockall
	setvar VAR_0x8004, 28
	goto ExoticSafari_EventScript_Seal

ExoticSafari_UnovaGiantChasm_EventScript_LegendKyurem::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_KYUREM
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_KYUREM
	goto ExoticSafari_EventScript_Release

ExoticSafari_UnovaGiantChasm_EventScript_SealKyurem::
	lockall
	setvar VAR_0x8004, 29
	goto ExoticSafari_EventScript_Seal
