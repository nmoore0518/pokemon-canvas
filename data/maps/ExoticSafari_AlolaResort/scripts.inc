@ ExoticSafari_AlolaResort (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/alola.py)
ExoticSafari_AlolaResort_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_AlolaResort_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_AlolaResort_OnLoad
	.byte 0

ExoticSafari_AlolaResort_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_AlolaResort_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_AlolaResort_EventScript_ToOcean::
	warpsilent MAP_EXOTIC_SAFARI_ALOLA_OCEAN, 0
	waitstate
	end

ExoticSafari_AlolaResort_EventScript_ToBeach::
	warpsilent MAP_EXOTIC_SAFARI_ALOLA_BEACH, 0
	waitstate
	end

ExoticSafari_AlolaResort_EventScript_ToSnow::
	warpsilent MAP_EXOTIC_SAFARI_ALOLA_LANAKILA, 0
	waitstate
	end

ExoticSafari_AlolaResort_EventScript_ToVolcano::
	warpsilent MAP_EXOTIC_SAFARI_ALOLA_VOLCANO, 0
	waitstate
	end

ExoticSafari_AlolaResort_EventScript_LegendMagearna::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_MAGEARNA
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_MAGEARNA
	goto ExoticSafari_EventScript_Release

ExoticSafari_AlolaResort_EventScript_SealMagearna::
	lockall
	setvar VAR_0x8004, 14
	goto ExoticSafari_EventScript_Seal

ExoticSafari_AlolaResort_EventScript_LegendMeltan::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_MELTAN
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_MELTAN
	goto ExoticSafari_EventScript_Release

ExoticSafari_AlolaResort_EventScript_SealMeltan::
	lockall
	setvar VAR_0x8004, 15
	goto ExoticSafari_EventScript_Seal
