@ ExoticSafari_KalosBadlands (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/kalos.py)
ExoticSafari_KalosBadlands_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_KalosBadlands_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_KalosBadlands_OnLoad
	.byte 0

ExoticSafari_KalosBadlands_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_KalosBadlands_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_KalosBadlands_EventScript_ToCity::
	warpsilent MAP_EXOTIC_SAFARI_KALOS_CITY, 5
	waitstate
	end

ExoticSafari_KalosBadlands_EventScript_ToOcean::
	warpsilent MAP_EXOTIC_SAFARI_KALOS_OCEAN, 0
	waitstate
	end

ExoticSafari_KalosBadlands_EventScript_LegendYveltal::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_YVELTAL
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_YVELTAL
	goto ExoticSafari_EventScript_Release

ExoticSafari_KalosBadlands_EventScript_SealYveltal::
	lockall
	setvar VAR_0x8004, 26
	goto ExoticSafari_EventScript_Seal

ExoticSafari_KalosBadlands_EventScript_Cell0::
	lock
	setvar VAR_0x8004, 6
	goto ExoticSafari_EventScript_Cell

ExoticSafari_KalosBadlands_EventScript_Cell1::
	lock
	setvar VAR_0x8004, 10
	goto ExoticSafari_EventScript_Cell
