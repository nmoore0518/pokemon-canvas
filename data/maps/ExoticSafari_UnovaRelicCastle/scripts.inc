@ ExoticSafari_UnovaRelicCastle (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/unova.py)
ExoticSafari_UnovaRelicCastle_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_UnovaRelicCastle_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_UnovaRelicCastle_OnLoad
	.byte 0

ExoticSafari_UnovaRelicCastle_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_UnovaRelicCastle_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_UnovaRelicCastle_EventScript_LegendVictini::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_VICTINI
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_VICTINI
	goto ExoticSafari_EventScript_Release
