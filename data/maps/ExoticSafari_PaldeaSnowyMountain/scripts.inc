@ ExoticSafari_PaldeaSnowyMountain (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/paldea.py)
ExoticSafari_PaldeaSnowyMountain_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_PaldeaSnowyMountain_OnTransition
	.byte 0

ExoticSafari_PaldeaSnowyMountain_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_PaldeaSnowyMountain_EventScript_ToLake::
	warpsilent MAP_EXOTIC_SAFARI_PALDEA_LAKE, 1
	waitstate
	end

ExoticSafari_PaldeaSnowyMountain_EventScript_LegendChienPao::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_CHIEN_PAO
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_CHIEN_PAO
	goto ExoticSafari_EventScript_Release

ExoticSafari_PaldeaSnowyMountain_EventScript_SealChienPao::
	lockall
	setvar VAR_0x8004, 6
	goto ExoticSafari_EventScript_Seal

ExoticSafari_PaldeaSnowyMountain_EventScript_ItemIceStone::
	finditem ITEM_ICE_STONE
	end
