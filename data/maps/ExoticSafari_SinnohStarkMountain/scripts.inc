@ ExoticSafari_SinnohStarkMountain (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/sinnoh.py)
ExoticSafari_SinnohStarkMountain_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_SinnohStarkMountain_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_SinnohStarkMountain_OnLoad
	.byte 0

ExoticSafari_SinnohStarkMountain_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_SinnohStarkMountain_OnLoad::
	special ExoticSafari_AreaOnLoad
	end
