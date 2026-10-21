@ ExoticSafari_AlolaRuins (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/alola.py)
ExoticSafari_AlolaRuins_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_AlolaRuins_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_AlolaRuins_OnLoad
	.byte 0

ExoticSafari_AlolaRuins_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_AlolaRuins_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_AlolaRuins_EventScript_ToOcean::
	warpsilent MAP_EXOTIC_SAFARI_ALOLA_OCEAN, 1
	waitstate
	end

ExoticSafari_AlolaRuins_EventScript_LegendMarshadow::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_MARSHADOW
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_MARSHADOW
	goto ExoticSafari_EventScript_Release

ExoticSafari_AlolaRuins_EventScript_SealMarshadow::
	lockall
	setvar VAR_0x8004, 18
	goto ExoticSafari_EventScript_Seal

ExoticSafari_AlolaRuins_EventScript_ItemDuskStone::
	finditem ITEM_DUSK_STONE
	end

ExoticSafari_AlolaRuins_EventScript_Relic0::
	lock
	setvar VAR_0x8004, 0
	goto ExoticSafari_EventScript_Relic

ExoticSafari_AlolaRuins_EventScript_Relic3::
	lock
	setvar VAR_0x8004, 3
	goto ExoticSafari_EventScript_Relic
