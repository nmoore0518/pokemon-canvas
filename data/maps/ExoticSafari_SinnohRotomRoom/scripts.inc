@ ExoticSafari_SinnohRotomRoom (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/sinnoh.py)
ExoticSafari_SinnohRotomRoom_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_SinnohRotomRoom_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_SinnohRotomRoom_OnLoad
	.byte 0

ExoticSafari_SinnohRotomRoom_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_SinnohRotomRoom_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_SinnohRotomRoom_EventScript_Appliance0::
	setvar VAR_0x8006, 0
	bufferstring STR_VAR_3, ExoticSafari_SinnohRotomRoom_Text_Appliance0
	goto ExoticSafari_EventScript_Appliance

ExoticSafari_SinnohRotomRoom_Text_Appliance0:
	.string "MICROWAVE OVEN$"

ExoticSafari_SinnohRotomRoom_EventScript_Appliance1::
	setvar VAR_0x8006, 1
	bufferstring STR_VAR_3, ExoticSafari_SinnohRotomRoom_Text_Appliance1
	goto ExoticSafari_EventScript_Appliance

ExoticSafari_SinnohRotomRoom_Text_Appliance1:
	.string "WASHING MACHINE$"

ExoticSafari_SinnohRotomRoom_EventScript_Appliance2::
	setvar VAR_0x8006, 2
	bufferstring STR_VAR_3, ExoticSafari_SinnohRotomRoom_Text_Appliance2
	goto ExoticSafari_EventScript_Appliance

ExoticSafari_SinnohRotomRoom_Text_Appliance2:
	.string "REFRIGERATOR$"

ExoticSafari_SinnohRotomRoom_EventScript_Appliance3::
	setvar VAR_0x8006, 3
	bufferstring STR_VAR_3, ExoticSafari_SinnohRotomRoom_Text_Appliance3
	goto ExoticSafari_EventScript_Appliance

ExoticSafari_SinnohRotomRoom_Text_Appliance3:
	.string "ELECTRIC FAN$"

ExoticSafari_SinnohRotomRoom_EventScript_Appliance4::
	setvar VAR_0x8006, 4
	bufferstring STR_VAR_3, ExoticSafari_SinnohRotomRoom_Text_Appliance4
	goto ExoticSafari_EventScript_Appliance

ExoticSafari_SinnohRotomRoom_Text_Appliance4:
	.string "LAWN MOWER$"
