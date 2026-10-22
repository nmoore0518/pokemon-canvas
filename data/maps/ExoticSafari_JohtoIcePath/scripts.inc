@ ExoticSafari_JohtoIcePath (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/johto_safari.py)
ExoticSafari_JohtoIcePath_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_JohtoIcePath_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_JohtoIcePath_OnLoad
	.byte 0

ExoticSafari_JohtoIcePath_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_JohtoIcePath_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_JohtoIcePath_EventScript_SilverWing::
	lock
	setflag FLAG_EXOTIC_SILVER_WING
	bufferstring STR_VAR_1, ExoticSafari_JohtoIcePath_EventScript_SilverWing_Name
	goto ExoticSafari_EventScript_JohtoKeepsake

ExoticSafari_JohtoIcePath_EventScript_SilverWing_Name:
	.string "SILVER WING$"
