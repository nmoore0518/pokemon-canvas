@ ExoticSafari_GalarCity (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/galar.py)
ExoticSafari_GalarCity_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_GalarCity_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_GalarCity_OnLoad
	.byte 0

ExoticSafari_GalarCity_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_GalarCity_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_GalarCity_EventScript_ToLake::
	warpsilent MAP_EXOTIC_SAFARI_GALAR_LAKE, 2
	waitstate
	end

ExoticSafari_GalarCity_EventScript_ToStadium::
	warpsilent MAP_EXOTIC_SAFARI_GALAR_STADIUM, 0
	waitstate
	end

ExoticSafari_GalarCity_EventScript_LegendZacian::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_ZACIAN
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_ZACIAN
	goto ExoticSafari_EventScript_Release

ExoticSafari_GalarCity_EventScript_SealZacian::
	lockall
	setvar VAR_0x8004, 11
	goto ExoticSafari_EventScript_Seal

ExoticSafari_GalarCity_EventScript_LegendZamazenta::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_ZAMAZENTA
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_ZAMAZENTA
	goto ExoticSafari_EventScript_Release

ExoticSafari_GalarCity_EventScript_SealZamazenta::
	lockall
	setvar VAR_0x8004, 12
	goto ExoticSafari_EventScript_Seal

ExoticSafari_GalarCity_EventScript_ItemRazorClaw::
	finditem ITEM_RAZOR_CLAW
	end

ExoticSafari_GalarCity_EventScript_ItemReinsOfUnity::
	finditem ITEM_REINS_OF_UNITY
	end
