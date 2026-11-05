@ Pokémon Canvas: the Universal Pokémon Medical Center, 2F: the wards (src/medical_center.c).
@ Runaway gurneys (OBJ_EVENT_GFX_GURNEY: pushed like boulders, no Strength needed) block the way from
@ the escalator up from the 1F to the nurse with the HEAL CHARM and the escalator up to the 3F. They go
@ back where they were when the player leaves the floor; once the player has the HEAL CHARM, the
@ orderlies have parked them (FLAG_GOT_HEAL_CHARM hides them). Lines are drafts: Nick writes the
@ final ones.

VermilionCity_MedicalCenter_2F_MapScripts::
	.byte 0

VermilionCity_MedicalCenter_2F_EventScript_WardNurse::
	lock
	faceplayer
	goto_if_set FLAG_GOT_HEAL_CHARM, VermilionCity_MedicalCenter_2F_EventScript_WardNurseAfter
	msgbox VermilionCity_MedicalCenter_2F_Text_WardNurse
	giveitem ITEM_HEAL_CHARM
	setflag FLAG_GOT_HEAL_CHARM
	msgbox VermilionCity_MedicalCenter_2F_Text_HealCharm
	release
	end

@ (a step past the gurneys: VAR_TEMP_1 stops it running again this visit)
VermilionCity_MedicalCenter_2F_EventScript_ThroughGurneys::
	lockall
	setvar VAR_TEMP_1, 1
	goto_if_set FLAG_GOT_HEAL_CHARM, VermilionCity_MedicalCenter_2F_EventScript_ThroughDone
	playse SE_PIN
	msgbox VermilionCity_MedicalCenter_2F_Text_WardNurse
	giveitem ITEM_HEAL_CHARM
	setflag FLAG_GOT_HEAL_CHARM
	msgbox VermilionCity_MedicalCenter_2F_Text_HealCharm
VermilionCity_MedicalCenter_2F_EventScript_ThroughDone::
	releaseall
	end

VermilionCity_MedicalCenter_2F_EventScript_WardNurseAfter::
	msgbox VermilionCity_MedicalCenter_2F_Text_WardNurseAfter
	release
	end

VermilionCity_MedicalCenter_2F_EventScript_Orderly::
	lock
	faceplayer
	goto_if_set FLAG_GOT_HEAL_CHARM, VermilionCity_MedicalCenter_2F_EventScript_OrderlyAfter
	msgbox VermilionCity_MedicalCenter_2F_Text_Orderly
	release
	end

VermilionCity_MedicalCenter_2F_EventScript_OrderlyAfter::
	msgbox VermilionCity_MedicalCenter_2F_Text_OrderlyAfter
	release
	end

VermilionCity_MedicalCenter_2F_EventScript_StationNurse::
	msgbox VermilionCity_MedicalCenter_2F_Text_StationNurse, MSGBOX_NPC
	end

VermilionCity_MedicalCenter_2F_EventScript_Chansey::
	lock
	faceplayer
	playmoncry SPECIES_CHANSEY, CRY_MODE_NORMAL
	msgbox VermilionCity_MedicalCenter_2F_Text_Chansey
	waitmoncry
	release
	end

VermilionCity_MedicalCenter_2F_EventScript_Gentleman::
	msgbox VermilionCity_MedicalCenter_2F_Text_Gentleman, MSGBOX_NPC
	end

VermilionCity_MedicalCenter_2F_EventScript_Lass::
	msgbox VermilionCity_MedicalCenter_2F_Text_Lass, MSGBOX_NPC
	end

VermilionCity_MedicalCenter_2F_EventScript_Meowth::
	lock
	faceplayer
	playmoncry SPECIES_MEOWTH, CRY_MODE_NORMAL
	msgbox VermilionCity_MedicalCenter_2F_Text_Meowth
	waitmoncry
	release
	end

VermilionCity_MedicalCenter_2F_EventScript_OldWoman::
	msgbox VermilionCity_MedicalCenter_2F_Text_OldWoman, MSGBOX_NPC
	end

VermilionCity_MedicalCenter_2F_EventScript_Fisher::
	msgbox VermilionCity_MedicalCenter_2F_Text_Fisher, MSGBOX_NPC
	end

VermilionCity_MedicalCenter_2F_EventScript_RoomSign0::
	msgbox VermilionCity_MedicalCenter_2F_Text_Room201, MSGBOX_SIGN
	end

VermilionCity_MedicalCenter_2F_EventScript_RoomSign1::
	msgbox VermilionCity_MedicalCenter_2F_Text_Room202, MSGBOX_SIGN
	end

VermilionCity_MedicalCenter_2F_EventScript_RoomSign2::
	msgbox VermilionCity_MedicalCenter_2F_Text_Room203, MSGBOX_SIGN
	end

VermilionCity_MedicalCenter_2F_Text_Orderly::
	.string "Oh, no! Somebody let the brakes\n"
	.string "off all the gurneys!\p"
	.string "They're blocking the whole floor.\n"
	.string "You'll have to push them aside.\p"
	.string "If they get jammed, just pop\n"
	.string "downstairs. I'll put them back.$"

VermilionCity_MedicalCenter_2F_Text_OrderlyAfter::
	.string "All the gurneys are parked, brakes\n"
	.string "on. Thanks for the help!$"

VermilionCity_MedicalCenter_2F_Text_WardNurse::
	.string "NURSE: You got through all those\n"
	.string "gurneys? My hero!\p"
	.string "Here, take this. It's a charm from\n"
	.string "the hospital's founder.$"

VermilionCity_MedicalCenter_2F_Text_HealCharm::
	.string "With that HEAL CHARM, your\n"
	.string "medicine works 10% better!$"

VermilionCity_MedicalCenter_2F_Text_WardNurseAfter::
	.string "The research lab's upstairs. The\n"
	.string "doctor's always losing things.$"

VermilionCity_MedicalCenter_2F_Text_StationNurse::
	.string "Visiting hours are any time!\n"
	.string "Our patients love the company.$"

VermilionCity_MedicalCenter_2F_Text_Chansey::
	.string "CHANSEY: Chansey! Chaaan!$"

VermilionCity_MedicalCenter_2F_Text_Gentleman::
	.string "My granddaughter's PIKACHU caught\n"
	.string "a cold. It's in room 201.$"

VermilionCity_MedicalCenter_2F_Text_Lass::
	.string "MEOWTH and I are waiting for our\n"
	.string "friend's TOGEPI to come home!$"

VermilionCity_MedicalCenter_2F_Text_Meowth::
	.string "MEOWTH: Meowth!$"

VermilionCity_MedicalCenter_2F_Text_OldWoman::
	.string "My GROWLITHE hurt its leg chasing\n"
	.string "a PONYTA. The silly thing!$"

VermilionCity_MedicalCenter_2F_Text_Fisher::
	.string "Hooked my own hand fishing off\n"
	.string "the pier. They patched me up quick.$"

VermilionCity_MedicalCenter_2F_Text_Room201::
	.string "ROOM 201$"

VermilionCity_MedicalCenter_2F_Text_Room202::
	.string "ROOM 202$"

VermilionCity_MedicalCenter_2F_Text_Room203::
	.string "ROOM 203\n"
	.string "Shh! A new arrival is resting.$"
