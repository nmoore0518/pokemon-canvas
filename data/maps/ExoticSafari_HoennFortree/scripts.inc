@ ExoticSafari_HoennFortree (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/hoenn.py)
ExoticSafari_HoennFortree_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_HoennFortree_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_HoennFortree_OnLoad
	.byte 0

ExoticSafari_HoennFortree_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_HoennFortree_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_HoennFortree_EventScript_ToOcean::
	warpsilent MAP_EXOTIC_SAFARI_HOENN_PACIFIDLOG, 0
	waitstate
	end

ExoticSafari_HoennFortree_EventScript_ToVolcano::
	warpsilent MAP_EXOTIC_SAFARI_HOENN_MT_CHIMNEY, 0
	waitstate
	end
