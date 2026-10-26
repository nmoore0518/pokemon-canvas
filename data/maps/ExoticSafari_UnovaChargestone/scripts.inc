@ ExoticSafari_UnovaChargestone (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/unova.py)
ExoticSafari_UnovaChargestone_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_UnovaChargestone_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_UnovaChargestone_OnLoad
	.byte 0

ExoticSafari_UnovaChargestone_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_UnovaChargestone_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_UnovaChargestone_EventScript_LegendZekrom::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_ZEKROM
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_ZEKROM
	goto ExoticSafari_EventScript_Release

ExoticSafari_UnovaChargestone_EventScript_SealZekrom::
	lockall
	setvar VAR_0x8004, 33
	goto ExoticSafari_EventScript_Seal
