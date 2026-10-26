@ ExoticSafari_KalosCave (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/kalos.py)
ExoticSafari_KalosCave_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_KalosCave_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_KalosCave_OnLoad
	.byte 0

ExoticSafari_KalosCave_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_KalosCave_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_KalosCave_EventScript_LegendHoopa::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_HOOPA
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_HOOPA
	goto ExoticSafari_EventScript_Release

ExoticSafari_KalosCave_EventScript_SealHoopa::
	lockall
	setvar VAR_0x8004, 25
	goto ExoticSafari_EventScript_Seal

ExoticSafari_KalosCave_EventScript_Cell0::
	lock
	setvar VAR_0x8004, 4
	goto ExoticSafari_EventScript_Cell

ExoticSafari_KalosCave_EventScript_Cell1::
	lock
	setvar VAR_0x8004, 5
	goto ExoticSafari_EventScript_Cell

ExoticSafari_KalosCave_EventScript_Cell2::
	lock
	setvar VAR_0x8004, 9
	goto ExoticSafari_EventScript_Cell
