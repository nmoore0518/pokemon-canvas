@ ExoticSafari_JohtoLakeOfRage (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/johto_safari.py)
ExoticSafari_JohtoLakeOfRage_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_JohtoLakeOfRage_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_JohtoLakeOfRage_OnLoad
	.byte 0

ExoticSafari_JohtoLakeOfRage_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_JohtoLakeOfRage_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_JohtoLakeOfRage_EventScript_ToGrassland::
	warpsilent MAP_EXOTIC_SAFARI_JOHTO_NEW_BARK, 2
	waitstate
	end

ExoticSafari_JohtoLakeOfRage_EventScript_ToIce::
	warpsilent MAP_EXOTIC_SAFARI_JOHTO_ICE_PATH, 0
	waitstate
	end
