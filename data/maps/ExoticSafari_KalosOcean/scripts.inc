@ ExoticSafari_KalosOcean (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/kalos.py)
ExoticSafari_KalosOcean_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_KalosOcean_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_KalosOcean_OnLoad
	.byte 0

ExoticSafari_KalosOcean_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_KalosOcean_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_KalosOcean_EventScript_ToDesert::
	warpsilent MAP_EXOTIC_SAFARI_KALOS_BADLANDS, 1
	waitstate
	end

ExoticSafari_KalosOcean_EventScript_LegendVolcanion::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_VOLCANION
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_VOLCANION
	goto ExoticSafari_EventScript_Release

ExoticSafari_KalosOcean_EventScript_SealVolcanion::
	lockall
	setvar VAR_0x8004, 27
	goto ExoticSafari_EventScript_Seal

ExoticSafari_KalosOcean_EventScript_Cell0::
	lock
	setvar VAR_0x8004, 7
	goto ExoticSafari_EventScript_Cell

ExoticSafari_KalosOcean_EventScript_Cell1::
	lock
	setvar VAR_0x8004, 11
	goto ExoticSafari_EventScript_Cell
