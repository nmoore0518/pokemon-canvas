@ ExoticSafari_JohtoNewBark (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/johto_safari.py)
ExoticSafari_JohtoNewBark_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_JohtoNewBark_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_JohtoNewBark_OnLoad
	.byte 0

ExoticSafari_JohtoNewBark_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_JohtoNewBark_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_JohtoNewBark_EventScript_ToRuins::
	warpsilent MAP_EXOTIC_SAFARI_JOHTO_RUINS_OF_ALPH, 0
	waitstate
	end
