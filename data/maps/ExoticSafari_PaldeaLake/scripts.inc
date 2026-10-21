@ ExoticSafari_PaldeaLake (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/paldea.py)
ExoticSafari_PaldeaLake_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_PaldeaLake_OnTransition
	.byte 0

ExoticSafari_PaldeaLake_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_PaldeaLake_EventScript_ToMeadow::
	warpsilent MAP_EXOTIC_SAFARI_PALDEA_MEADOW, 4
	waitstate
	end

ExoticSafari_PaldeaLake_EventScript_ToSnow::
	warpsilent MAP_EXOTIC_SAFARI_PALDEA_SNOWY_MOUNTAIN, 0
	waitstate
	end

ExoticSafari_PaldeaLake_EventScript_LegendChiYu::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_CHI_YU
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendStays
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_CHI_YU
	goto ExoticSafari_EventScript_Release
