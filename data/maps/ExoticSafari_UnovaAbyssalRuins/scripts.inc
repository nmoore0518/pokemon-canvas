@ ExoticSafari_UnovaAbyssalRuins (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/unova.py)
ExoticSafari_UnovaAbyssalRuins_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_UnovaAbyssalRuins_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_UnovaAbyssalRuins_OnLoad
	.byte 0

ExoticSafari_UnovaAbyssalRuins_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_UnovaAbyssalRuins_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_UnovaAbyssalRuins_EventScript_LegendKeldeo::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_KELDEO
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_KELDEO
	goto ExoticSafari_EventScript_Release

ExoticSafari_UnovaAbyssalRuins_EventScript_SealKeldeo::
	lockall
	setvar VAR_0x8004, 33
	goto ExoticSafari_EventScript_Seal

ExoticSafari_UnovaAbyssalRuins_EventScript_Key1::
	lock
	setvar VAR_0x8004, 1
	goto ExoticSafari_EventScript_Key
