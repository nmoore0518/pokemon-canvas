@ ExoticSafari_GalarMineDepths (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/galar.py)
ExoticSafari_GalarMineDepths_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_GalarMineDepths_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_GalarMineDepths_OnLoad
	.byte 0

ExoticSafari_GalarMineDepths_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_GalarMineDepths_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_GalarMineDepths_EventScript_LegendRegidrago::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_REGIDRAGO
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_REGIDRAGO
	goto ExoticSafari_EventScript_Release

ExoticSafari_GalarMineDepths_EventScript_SealRegidrago::
	lockall
	setvar VAR_0x8004, 8
	goto ExoticSafari_EventScript_Seal

ExoticSafari_GalarMineDepths_EventScript_ItemPeatBlock::
	finditem ITEM_PEAT_BLOCK
	end

ExoticSafari_GalarMineDepths_EventScript_ItemRustedShield::
	finditem ITEM_RUSTED_SHIELD
	end
