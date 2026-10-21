@ ExoticSafari_GalarMeadow (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/galar.py)
ExoticSafari_GalarMeadow_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_GalarMeadow_OnTransition
	.byte 0

ExoticSafari_GalarMeadow_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_GalarMeadow_EventScript_ToHall::
	warpsilent MAP_EXOTIC_SAFARI_HALL, 3
	waitstate
	end

ExoticSafari_GalarMeadow_EventScript_ToLake::
	warpsilent MAP_EXOTIC_SAFARI_GALAR_LAKE, 0
	waitstate
	end

ExoticSafari_GalarMeadow_EventScript_LegendRegieleki::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_REGIELEKI
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_REGIELEKI
	goto ExoticSafari_EventScript_Release

ExoticSafari_GalarMeadow_EventScript_ItemTartApple::
	finditem ITEM_TART_APPLE
	end

ExoticSafari_GalarMeadow_EventScript_ItemSweetApple::
	finditem ITEM_SWEET_APPLE
	end
