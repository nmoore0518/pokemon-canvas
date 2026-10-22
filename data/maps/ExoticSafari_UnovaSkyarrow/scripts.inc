@ ExoticSafari_UnovaSkyarrow (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/unova.py)
ExoticSafari_UnovaSkyarrow_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_UnovaSkyarrow_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_UnovaSkyarrow_OnLoad
	.byte 0

ExoticSafari_UnovaSkyarrow_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_UnovaSkyarrow_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_UnovaSkyarrow_EventScript_ToHall::
	warpsilent MAP_EXOTIC_SAFARI_HALL, 5
	waitstate
	end

ExoticSafari_UnovaSkyarrow_EventScript_LegendTerrakion::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_TERRAKION
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_TERRAKION
	goto ExoticSafari_EventScript_Release

ExoticSafari_UnovaSkyarrow_EventScript_LegendVirizion::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_VIRIZION
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_VIRIZION
	goto ExoticSafari_EventScript_Release

ExoticSafari_UnovaSkyarrow_EventScript_Key2::
	lock
	setvar VAR_0x8004, 2
	goto ExoticSafari_EventScript_Key
