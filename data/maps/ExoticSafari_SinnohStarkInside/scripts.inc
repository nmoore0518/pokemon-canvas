@ ExoticSafari_SinnohStarkInside (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/sinnoh.py)
ExoticSafari_SinnohStarkInside_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_SinnohStarkInside_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_SinnohStarkInside_OnLoad
	.byte 0

ExoticSafari_SinnohStarkInside_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_SinnohStarkInside_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_SinnohStarkInside_EventScript_LegendHeatran::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_HEATRAN
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_HEATRAN
	goto ExoticSafari_EventScript_Release
