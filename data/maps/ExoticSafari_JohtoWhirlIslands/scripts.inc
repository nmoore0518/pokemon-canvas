@ ExoticSafari_JohtoWhirlIslands (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/johto_safari.py)
ExoticSafari_JohtoWhirlIslands_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_JohtoWhirlIslands_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_JohtoWhirlIslands_OnLoad
	.byte 0

ExoticSafari_JohtoWhirlIslands_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_JohtoWhirlIslands_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_JohtoWhirlIslands_EventScript_LegendLugia::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_LUGIA
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_LUGIA
	goto ExoticSafari_EventScript_Release
