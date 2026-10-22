@ ExoticSafari_JohtoRuinsOfAlph (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/johto_safari.py)
ExoticSafari_JohtoRuinsOfAlph_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_JohtoRuinsOfAlph_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_JohtoRuinsOfAlph_OnLoad
	.byte 0

ExoticSafari_JohtoRuinsOfAlph_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_JohtoRuinsOfAlph_OnLoad::
	special ExoticSafari_AreaOnLoad
	end
