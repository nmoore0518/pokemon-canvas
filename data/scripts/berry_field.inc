@ Pokémon Canvas: the berry field in Pallet Town (src/berry_field.c). (Texts: Claude's drafts, Oct 31.)
BerryField_EventScript_Sign::
	msgbox BerryField_Text_Sign, MSGBOX_SIGN
	end

BerryField_EventScript_Plot::
	lockall
	special BerryField_GetFacingPlot
	goto_if_ge VAR_0x8004, BERRY_FIELD_PLOTS, BerryField_EventScript_End
	goto_if_eq VAR_RESULT, 1, BerryField_EventScript_Growing
	goto_if_eq VAR_RESULT, 2, BerryField_EventScript_Ripe
	msgbox BerryField_Text_Soil, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, BerryField_EventScript_End
	closemessage
	special Bag_ChooseBerry
	waitstate
	goto_if_eq VAR_ITEM_ID, ITEM_NONE, BerryField_EventScript_End
	special BerryField_Plant
	playse SE_RG_BAG_POCKET
	msgbox BerryField_Text_Planted
	releaseall
	end

BerryField_EventScript_Growing::
	msgbox BerryField_Text_Growing
	msgbox BerryField_Text_PullUp, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, BerryField_EventScript_End
	special BerryField_PullUp
	msgbox BerryField_Text_PulledUp
	releaseall
	end

BerryField_EventScript_Ripe::
	special BerryField_Harvest
	goto_if_eq VAR_RESULT, FALSE, BerryField_EventScript_BagFull
	playfanfare MUS_OBTAIN_ITEM
	msgbox BerryField_Text_Picked
	waitfanfare
	releaseall
	end

BerryField_EventScript_BagFull::
	msgbox BerryField_Text_BagFull
BerryField_EventScript_End::
	closemessage
	releaseall
	end

BerryField_Text_Sign:
	.string "BERRY FIELD\n"
	.string "Plant any BERRY. It grows as you walk!$"

BerryField_Text_Soil:
	.string "It's soft, freshly turned soil.\n"
	.string "Plant a BERRY here?$"

BerryField_Text_Planted:
	.string "{PLAYER} planted a {STR_VAR_1}\n"
	.string "in the soil.$"

BerryField_Text_Growing:
	.string "A {STR_VAR_1} is growing here.\p"
	.string "It looks like it'll be ready in\n"
	.string "about {STR_VAR_2} steps.$"

BerryField_Text_PullUp:
	.string "Pull it up and take the BERRY back?$"

BerryField_Text_PulledUp:
	.string "{PLAYER} pulled up the plant and\n"
	.string "got the {STR_VAR_1} back.$"

BerryField_Text_Picked:
	.string "{PLAYER} picked {STR_VAR_2} {STR_VAR_1}!\n"
	.string "The soil is bare again.$"

BerryField_Text_BagFull:
	.string "The BAG has no room for them.$"

@ The Tera berries' one-time finds (tools/gym_hack/place_berry_finds.py puts them on the maps;
@ FLAG_BERRY_FIND_*: the item ball's own flag)
BerryField_EventScript_FindNormal::
	finditem ITEM_CHILAN_BERRY
	end
BerryField_EventScript_FindBug::
	finditem ITEM_TANGA_BERRY
	end
BerryField_EventScript_FindFighting::
	finditem ITEM_CHOPLE_BERRY
	end
BerryField_EventScript_FindRock::
	finditem ITEM_CHARTI_BERRY
	end
BerryField_EventScript_FindGrass::
	finditem ITEM_RINDO_BERRY
	end
BerryField_EventScript_FindWater::
	finditem ITEM_PASSHO_BERRY
	end
BerryField_EventScript_FindPoison::
	finditem ITEM_KEBIA_BERRY
	end
BerryField_EventScript_FindElectric::
	finditem ITEM_WACAN_BERRY
	end
BerryField_EventScript_FindGround::
	finditem ITEM_SHUCA_BERRY
	end
BerryField_EventScript_FindPsychic::
	finditem ITEM_PAYAPA_BERRY
	end
BerryField_EventScript_FindGhost::
	finditem ITEM_KASIB_BERRY
	end
BerryField_EventScript_FindFairy::
	finditem ITEM_ROSELI_BERRY
	end
BerryField_EventScript_FindDark::
	finditem ITEM_COLBUR_BERRY
	end
BerryField_EventScript_FindFlying::
	finditem ITEM_COBA_BERRY
	end
BerryField_EventScript_FindSteel::
	finditem ITEM_BABIRI_BERRY
	end
BerryField_EventScript_FindFire::
	finditem ITEM_OCCA_BERRY
	end
BerryField_EventScript_FindIce::
	finditem ITEM_YACHE_BERRY
	end
BerryField_EventScript_FindDragon::
	finditem ITEM_HABAN_BERRY
	end
