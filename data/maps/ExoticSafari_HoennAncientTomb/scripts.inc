@ ExoticSafari_HoennAncientTomb (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/hoenn.py)
ExoticSafari_HoennAncientTomb_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_HoennAncientTomb_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_HoennAncientTomb_OnLoad
	.byte 0

ExoticSafari_HoennAncientTomb_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_HoennAncientTomb_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_HoennAncientTomb_EventScript_LegendRegisteel::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_REGISTEEL
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_REGISTEEL
	goto ExoticSafari_EventScript_Release
