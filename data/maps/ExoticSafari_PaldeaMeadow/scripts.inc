@ ExoticSafari_PaldeaMeadow (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/paldea.py)
ExoticSafari_PaldeaMeadow_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_PaldeaMeadow_OnTransition
	.byte 0

ExoticSafari_PaldeaMeadow_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_PaldeaMeadow_EventScript_ToForest::
	warpsilent MAP_EXOTIC_SAFARI_PALDEA_FOREST, 0
	waitstate
	end

ExoticSafari_PaldeaMeadow_EventScript_ToLake::
	warpsilent MAP_EXOTIC_SAFARI_PALDEA_LAKE, 0
	waitstate
	end

ExoticSafari_PaldeaMeadow_EventScript_ToAcademy::
	warpsilent MAP_EXOTIC_SAFARI_PALDEA_ACADEMY, 1
	waitstate
	end

ExoticSafari_PaldeaMeadow_EventScript_LegendOgerpon::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_OGERPON
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_OGERPON
	goto ExoticSafari_EventScript_Release

ExoticSafari_PaldeaMeadow_EventScript_SealOgerpon::
	lockall
	setvar VAR_0x8004, 0
	goto ExoticSafari_EventScript_Seal
