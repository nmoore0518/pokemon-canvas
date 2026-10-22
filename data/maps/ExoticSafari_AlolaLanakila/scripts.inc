@ ExoticSafari_AlolaLanakila (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/alola.py)
ExoticSafari_AlolaLanakila_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_AlolaLanakila_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_AlolaLanakila_OnLoad
	.byte 0

ExoticSafari_AlolaLanakila_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_AlolaLanakila_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_AlolaLanakila_EventScript_ToResort::
	warpsilent MAP_EXOTIC_SAFARI_ALOLA_RESORT, 3
	waitstate
	end

ExoticSafari_AlolaLanakila_EventScript_LegendLunala::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_LUNALA
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_LUNALA
	goto ExoticSafari_EventScript_Release

ExoticSafari_AlolaLanakila_EventScript_SealLunala::
	lockall
	setvar VAR_0x8004, 21
	goto ExoticSafari_EventScript_Seal

ExoticSafari_AlolaLanakila_EventScript_ItemMoonStone::
	finditem ITEM_MOON_STONE
	end

ExoticSafari_AlolaLanakila_EventScript_ItemNLunarizer::
	finditem ITEM_N_LUNARIZER
	end

ExoticSafari_AlolaLanakila_EventScript_Relic5::
	lock
	setvar VAR_0x8004, 5
	goto ExoticSafari_EventScript_Relic
