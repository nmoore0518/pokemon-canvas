@ ExoticSafari_GalarMeadow (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/galar.py)
ExoticSafari_GalarMeadow_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_GalarMeadow_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_GalarMeadow_OnLoad
	.byte 0

ExoticSafari_GalarMeadow_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_GalarMeadow_OnLoad::
	special ExoticSafari_AreaOnLoad
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

ExoticSafari_GalarMeadow_EventScript_SealRegieleki::
	lockall
	setvar VAR_0x8004, 7
	goto ExoticSafari_EventScript_Seal

ExoticSafari_GalarMeadow_EventScript_ItemTartApple::
	finditem ITEM_TART_APPLE
	end

ExoticSafari_GalarMeadow_EventScript_ItemSweetApple::
	finditem ITEM_SWEET_APPLE
	end

ExoticSafari_GalarMeadow_EventScript_Stone0::
	setvar VAR_0x8004, 0
	goto ExoticSafari_EventScript_Stone

ExoticSafari_GalarMeadow_EventScript_Stone1::
	setvar VAR_0x8004, 1
	goto ExoticSafari_EventScript_Stone

ExoticSafari_GalarMeadow_EventScript_Stone2::
	setvar VAR_0x8004, 2
	goto ExoticSafari_EventScript_Stone

ExoticSafari_GalarMeadow_EventScript_Stone3::
	setvar VAR_0x8004, 3
	goto ExoticSafari_EventScript_Stone

ExoticSafari_GalarMeadow_EventScript_Stone4::
	setvar VAR_0x8004, 4
	goto ExoticSafari_EventScript_Stone

ExoticSafari_GalarMeadow_EventScript_Stone5::
	setvar VAR_0x8004, 5
	goto ExoticSafari_EventScript_Stone

ExoticSafari_GalarMeadow_EventScript_Stone6::
	setvar VAR_0x8004, 6
	goto ExoticSafari_EventScript_Stone

ExoticSafari_GalarMeadow_EventScript_StonesSign::
	msgbox ExoticSafari_Text_StonesSign, MSGBOX_SIGN
	end
