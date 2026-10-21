@ ExoticSafari_GalarForest (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/galar.py)
ExoticSafari_GalarForest_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_GalarForest_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_GalarForest_OnLoad
	.byte 0

ExoticSafari_GalarForest_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_GalarForest_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_GalarForest_EventScript_ToLake::
	warpsilent MAP_EXOTIC_SAFARI_GALAR_LAKE, 1
	waitstate
	end

ExoticSafari_GalarForest_EventScript_LegendZarude::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_ZARUDE
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_ZARUDE
	goto ExoticSafari_EventScript_Release

ExoticSafari_GalarForest_EventScript_SealZarude::
	lockall
	setvar VAR_0x8004, 12
	goto ExoticSafari_EventScript_Seal

ExoticSafari_GalarForest_EventScript_LegendEnamorus::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_ENAMORUS
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_ENAMORUS
	goto ExoticSafari_EventScript_Release

ExoticSafari_GalarForest_EventScript_SealEnamorus::
	lockall
	setvar VAR_0x8004, 13
	goto ExoticSafari_EventScript_Seal

ExoticSafari_GalarForest_EventScript_ItemChippedPot::
	finditem ITEM_CHIPPED_POT
	end

ExoticSafari_GalarForest_EventScript_ItemStrawberrySweet::
	finditem ITEM_STRAWBERRY_SWEET
	end

ExoticSafari_GalarForest_EventScript_ItemGalaricaWreath::
	finditem ITEM_GALARICA_WREATH
	end

ExoticSafari_GalarForest_EventScript_ItemRustedSword::
	finditem ITEM_RUSTED_SWORD
	end

ExoticSafari_GalarForest_EventScript_Lamp0::
	setvar VAR_0x8004, 0
	goto ExoticSafari_EventScript_Lamp

ExoticSafari_GalarForest_EventScript_Lamp1::
	setvar VAR_0x8004, 1
	goto ExoticSafari_EventScript_Lamp

ExoticSafari_GalarForest_EventScript_Lamp2::
	setvar VAR_0x8004, 2
	goto ExoticSafari_EventScript_Lamp

ExoticSafari_GalarForest_EventScript_Lamp3::
	setvar VAR_0x8004, 3
	goto ExoticSafari_EventScript_Lamp

ExoticSafari_GalarForest_EventScript_Lamp4::
	setvar VAR_0x8004, 4
	goto ExoticSafari_EventScript_Lamp

ExoticSafari_GalarForest_EventScript_Lamp5::
	setvar VAR_0x8004, 5
	goto ExoticSafari_EventScript_Lamp

ExoticSafari_GalarForest_EventScript_Lamp6::
	setvar VAR_0x8004, 6
	goto ExoticSafari_EventScript_Lamp

ExoticSafari_GalarForest_EventScript_Lamp7::
	setvar VAR_0x8004, 7
	goto ExoticSafari_EventScript_Lamp
