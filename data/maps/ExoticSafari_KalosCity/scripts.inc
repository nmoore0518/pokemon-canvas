@ ExoticSafari_KalosCity (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/kalos.py)
ExoticSafari_KalosCity_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_KalosCity_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_KalosCity_OnLoad
	.byte 0

ExoticSafari_KalosCity_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_KalosCity_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_KalosCity_EventScript_LegendZygarde::
	lock
	faceplayer
	call ExoticSafari_EventScript_ZygardePre
	setvar VAR_0x8004, SPECIES_ZYGARDE
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_ZYGARDE
	goto ExoticSafari_EventScript_Release

ExoticSafari_KalosCity_EventScript_SealZygarde::
	lockall
	setvar VAR_0x8004, 22
	goto ExoticSafari_EventScript_Seal

ExoticSafari_KalosCity_EventScript_ItemSachet::
	finditem ITEM_SACHET
	end

ExoticSafari_KalosCity_EventScript_ItemWhippedDream::
	finditem ITEM_WHIPPED_DREAM
	end

ExoticSafari_KalosCity_EventScript_Cell0::
	lock
	setvar VAR_0x8004, 0
	goto ExoticSafari_EventScript_Cell
