@ ExoticSafari_SinnohLake (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/sinnoh.py)
ExoticSafari_SinnohLake_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_SinnohLake_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_SinnohLake_OnLoad
	.byte 0

ExoticSafari_SinnohLake_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_SinnohLake_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_SinnohLake_EventScript_ToMeadow::
	warpsilent MAP_EXOTIC_SAFARI_SINNOH_MEADOW, 0
	waitstate
	end

ExoticSafari_SinnohLake_EventScript_ToVolcano::
	warpsilent MAP_EXOTIC_SAFARI_SINNOH_STARK_MOUNTAIN, 0
	waitstate
	end

ExoticSafari_SinnohLake_EventScript_ToCity::
	warpsilent MAP_EXOTIC_SAFARI_SINNOH_CITY, 0
	waitstate
	end

ExoticSafari_SinnohLake_EventScript_ToMarsh::
	warpsilent MAP_EXOTIC_SAFARI_SINNOH_MARSH, 0
	waitstate
	end

ExoticSafari_SinnohLake_EventScript_LegendUxie::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_UXIE
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_UXIE
	goto ExoticSafari_EventScript_Release

ExoticSafari_SinnohLake_EventScript_SealUxie::
	lockall
	setvar VAR_0x8004, 34
	goto ExoticSafari_EventScript_Seal

ExoticSafari_SinnohLake_EventScript_LegendAzelf::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_AZELF
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_AZELF
	goto ExoticSafari_EventScript_Release

ExoticSafari_SinnohLake_EventScript_SealAzelf::
	lockall
	setvar VAR_0x8004, 35
	goto ExoticSafari_EventScript_Seal

ExoticSafari_SinnohLake_EventScript_LegendMesprit::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_MESPRIT
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_MESPRIT
	goto ExoticSafari_EventScript_Release

ExoticSafari_SinnohLake_EventScript_SealMesprit::
	lockall
	setvar VAR_0x8004, 36
	goto ExoticSafari_EventScript_Seal
