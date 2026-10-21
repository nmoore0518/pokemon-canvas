@ ExoticSafari_GalarForest (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/galar.py)
ExoticSafari_GalarForest_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_GalarForest_OnTransition
	.byte 0

ExoticSafari_GalarForest_OnTransition::
	special ExoticSafari_AreaOnTransition
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

ExoticSafari_GalarForest_EventScript_LegendEnamorus::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_ENAMORUS
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_ENAMORUS
	goto ExoticSafari_EventScript_Release

ExoticSafari_GalarForest_EventScript_ItemChippedPot::
	finditem ITEM_CHIPPED_POT
	end

ExoticSafari_GalarForest_EventScript_ItemStrawberrySweet::
	finditem ITEM_STRAWBERRY_SWEET
	end

ExoticSafari_GalarForest_EventScript_ItemGalaricaWreath::
	finditem ITEM_GALARICA_WREATH
	end
