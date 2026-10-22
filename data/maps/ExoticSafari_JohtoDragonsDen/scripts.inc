@ ExoticSafari_JohtoDragonsDen (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/johto_safari.py)
ExoticSafari_JohtoDragonsDen_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_JohtoDragonsDen_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_JohtoDragonsDen_OnLoad
	.byte 0

ExoticSafari_JohtoDragonsDen_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_JohtoDragonsDen_OnLoad::
	special ExoticSafari_AreaOnLoad
	end
