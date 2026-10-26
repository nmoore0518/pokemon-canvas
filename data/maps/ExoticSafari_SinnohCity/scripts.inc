@ ExoticSafari_SinnohCity (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/sinnoh.py)
ExoticSafari_SinnohCity_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_SinnohCity_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_SinnohCity_OnLoad
	.byte 0

ExoticSafari_SinnohCity_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_SinnohCity_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_SinnohCity_EventScript_LegendRegigigas::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_REGIGIGAS
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_REGIGIGAS
	goto ExoticSafari_EventScript_Release

ExoticSafari_SinnohCity_EventScript_SealRegigigas::
	lockall
	setvar VAR_0x8004, 40
	goto ExoticSafari_EventScript_Seal
