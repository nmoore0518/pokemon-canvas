@ ExoticSafari_UnovaDragonspiral (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/unova.py)
ExoticSafari_UnovaDragonspiral_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_UnovaDragonspiral_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_UnovaDragonspiral_OnLoad
	.byte 0

ExoticSafari_UnovaDragonspiral_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_UnovaDragonspiral_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_UnovaDragonspiral_EventScript_LegendReshiram::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_RESHIRAM
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_RESHIRAM
	goto ExoticSafari_EventScript_Release

ExoticSafari_UnovaDragonspiral_EventScript_SealReshiram::
	lockall
	setvar VAR_0x8004, 30
	goto ExoticSafari_EventScript_Seal
