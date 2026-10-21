@ ExoticSafari_PaldeaCrater (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/paldea.py)
ExoticSafari_PaldeaCrater_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_PaldeaCrater_OnTransition
	.byte 0

ExoticSafari_PaldeaCrater_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_PaldeaCrater_EventScript_ItemMaliciousArmor::
	finditem ITEM_MALICIOUS_ARMOR
	end
