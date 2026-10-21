@ ExoticSafari_GalarStadiumPitch (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/galar.py)
ExoticSafari_GalarStadiumPitch_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_GalarStadiumPitch_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_GalarStadiumPitch_OnLoad
	.byte 0

ExoticSafari_GalarStadiumPitch_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_GalarStadiumPitch_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_GalarStadiumPitch_EventScript_ToLobby::
	warpsilent MAP_EXOTIC_SAFARI_GALAR_STADIUM, 2
	waitstate
	end

ExoticSafari_GalarStadiumPitch_EventScript_LegendKubfu::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_KUBFU
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_KUBFU
	goto ExoticSafari_EventScript_Release

ExoticSafari_GalarStadiumPitch_EventScript_SealKubfu::
	lockall
	setvar VAR_0x8004, 9
	goto ExoticSafari_EventScript_Seal

ExoticSafari_GalarStadiumPitch_EventScript_ItemScrollOfDarkness::
	finditem ITEM_SCROLL_OF_DARKNESS
	end

ExoticSafari_GalarStadiumPitch_EventScript_ItemScrollOfWaters::
	finditem ITEM_SCROLL_OF_WATERS
	end
