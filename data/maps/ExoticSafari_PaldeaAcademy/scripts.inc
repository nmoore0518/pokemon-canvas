@ ExoticSafari_PaldeaAcademy (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/paldea.py)
ExoticSafari_PaldeaAcademy_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_PaldeaAcademy_OnTransition
	.byte 0

ExoticSafari_PaldeaAcademy_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_PaldeaAcademy_EventScript_ItemUnremarkableTeacup::
	finditem ITEM_UNREMARKABLE_TEACUP
	end

ExoticSafari_PaldeaAcademy_EventScript_ItemMasterpieceTeacup::
	finditem ITEM_MASTERPIECE_TEACUP
	end
