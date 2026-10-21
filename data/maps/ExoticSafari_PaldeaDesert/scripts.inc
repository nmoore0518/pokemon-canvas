@ ExoticSafari_PaldeaDesert (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/paldea.py)
ExoticSafari_PaldeaDesert_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_PaldeaDesert_OnTransition
	.byte 0

ExoticSafari_PaldeaDesert_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_PaldeaDesert_EventScript_ToForest::
	warpsilent MAP_EXOTIC_SAFARI_PALDEA_FOREST, 1
	waitstate
	end

ExoticSafari_PaldeaDesert_EventScript_LegendTingLu::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_TING_LU
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendStays
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_TING_LU
	goto ExoticSafari_EventScript_Release

ExoticSafari_PaldeaDesert_EventScript_ItemAuspiciousArmor::
	finditem ITEM_AUSPICIOUS_ARMOR
	end
