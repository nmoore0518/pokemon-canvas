@ ExoticSafari_HoennMtChimney (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/hoenn.py)
ExoticSafari_HoennMtChimney_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_HoennMtChimney_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_HoennMtChimney_OnLoad
	.byte 0

ExoticSafari_HoennMtChimney_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_HoennMtChimney_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_HoennMtChimney_EventScript_LegendDeoxys::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_DEOXYS
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_LEGEND_DEOXYS
	goto ExoticSafari_EventScript_Release

ExoticSafari_HoennMtChimney_EventScript_SealDeoxys::
	lockall
	setvar VAR_0x8004, 44
	goto ExoticSafari_EventScript_Seal

ExoticSafari_HoennMtChimney_EventScript_ItemMeteorite::
	finditem ITEM_METEORITE
	end
