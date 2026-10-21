@ ExoticSafari_AlolaVolcano (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/alola.py)
ExoticSafari_AlolaVolcano_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_AlolaVolcano_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_AlolaVolcano_OnLoad
	.byte 0

ExoticSafari_AlolaVolcano_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_AlolaVolcano_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_AlolaVolcano_EventScript_ToResort::
	warpsilent MAP_EXOTIC_SAFARI_ALOLA_RESORT, 4
	waitstate
	end

ExoticSafari_AlolaVolcano_EventScript_LegendSolgaleo::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_SOLGALEO
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_SOLGALEO
	goto ExoticSafari_EventScript_Release

ExoticSafari_AlolaVolcano_EventScript_LegendNecrozma::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_NECROZMA
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_NECROZMA
	goto ExoticSafari_EventScript_Release

ExoticSafari_AlolaVolcano_EventScript_ItemSunStone::
	finditem ITEM_SUN_STONE
	end
