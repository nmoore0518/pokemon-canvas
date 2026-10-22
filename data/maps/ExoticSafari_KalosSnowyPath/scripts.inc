@ ExoticSafari_KalosSnowyPath (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/kalos.py)
ExoticSafari_KalosSnowyPath_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_KalosSnowyPath_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_KalosSnowyPath_OnLoad
	.byte 0

ExoticSafari_KalosSnowyPath_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_KalosSnowyPath_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_KalosSnowyPath_EventScript_ToWoods::
	warpsilent MAP_EXOTIC_SAFARI_KALOS_WOODS, 1
	waitstate
	end

ExoticSafari_KalosSnowyPath_EventScript_ItemShinyStone::
	finditem ITEM_SHINY_STONE
	end

ExoticSafari_KalosSnowyPath_EventScript_Cell0::
	lock
	setvar VAR_0x8004, 3
	goto ExoticSafari_EventScript_Cell

ExoticSafari_KalosSnowyPath_EventScript_Cell1::
	lock
	setvar VAR_0x8004, 8
	goto ExoticSafari_EventScript_Cell
