@ ExoticSafari_HoennDesertRuins (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/hoenn.py)
ExoticSafari_HoennDesertRuins_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_HoennDesertRuins_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_HoennDesertRuins_OnLoad
	.byte 0

ExoticSafari_HoennDesertRuins_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_HoennDesertRuins_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_HoennDesertRuins_EventScript_LegendRegirock::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_REGIROCK
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_REGIROCK
	goto ExoticSafari_EventScript_Release

ExoticSafari_HoennDesertRuins_EventScript_RegiWall::
	lockall
	setvar VAR_0x8004, 0
	goto ExoticSafari_EventScript_RegiWall
