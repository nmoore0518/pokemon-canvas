@ ExoticSafari_SinnohMarsh (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/sinnoh.py)
ExoticSafari_SinnohMarsh_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_SinnohMarsh_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_SinnohMarsh_OnLoad
	.byte 0

ExoticSafari_SinnohMarsh_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_SinnohMarsh_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_SinnohMarsh_EventScript_ToLake::
	warpsilent MAP_EXOTIC_SAFARI_SINNOH_LAKE, 6
	waitstate
	end

ExoticSafari_SinnohMarsh_EventScript_LegendPhione::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_PHIONE
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_PHIONE
	goto ExoticSafari_EventScript_Release
