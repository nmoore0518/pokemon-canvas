@ ExoticSafari_HoennSootopolis (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/hoenn.py)
ExoticSafari_HoennSootopolis_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_HoennSootopolis_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_HoennSootopolis_OnLoad
	.byte 0

ExoticSafari_HoennSootopolis_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_HoennSootopolis_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_HoennSootopolis_EventScript_LegendJirachi::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_JIRACHI
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_JIRACHI
	goto ExoticSafari_EventScript_Release
