@ ExoticSafari_GalarMine (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/galar.py)
ExoticSafari_GalarMine_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_GalarMine_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_GalarMine_OnLoad
	.byte 0

ExoticSafari_GalarMine_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_GalarMine_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_GalarMine_EventScript_ItemBlackAugurite::
	finditem ITEM_BLACK_AUGURITE
	end

ExoticSafari_GalarMine_EventScript_ItemDragonFang::
	finditem ITEM_DRAGON_FANG
	end
