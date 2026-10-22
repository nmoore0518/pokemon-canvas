@ ExoticSafari_HoennIslandCave (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/hoenn.py)
ExoticSafari_HoennIslandCave_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_HoennIslandCave_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_HoennIslandCave_OnLoad
	.byte 0

ExoticSafari_HoennIslandCave_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_HoennIslandCave_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_HoennIslandCave_EventScript_LegendRegice::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_REGICE
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_REGICE
	goto ExoticSafari_EventScript_Release

ExoticSafari_HoennIslandCave_EventScript_RegiWall::
	lockall
	setvar VAR_0x8004, 1
	goto ExoticSafari_EventScript_RegiWall
