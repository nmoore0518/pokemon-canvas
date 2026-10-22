@ ExoticSafari_SinnohHallOfOrigin (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/sinnoh.py)
ExoticSafari_SinnohHallOfOrigin_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_SinnohHallOfOrigin_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_SinnohHallOfOrigin_OnLoad
	.byte 0

ExoticSafari_SinnohHallOfOrigin_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_SinnohHallOfOrigin_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_SinnohHallOfOrigin_EventScript_LegendArceus::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_ARCEUS
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_ARCEUS
	goto ExoticSafari_EventScript_Release

ExoticSafari_SinnohHallOfOrigin_EventScript_SealArceus::
	lockall
	setvar VAR_0x8004, 40
	goto ExoticSafari_EventScript_Seal
