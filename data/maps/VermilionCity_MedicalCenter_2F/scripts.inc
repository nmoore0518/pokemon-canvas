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

VermilionCity_MedicalCenter_2F_EventScript_WardDoor::
	msgbox VermilionCity_MedicalCenter_2F_Text_WardDoor, MSGBOX_SIGN
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

VermilionCity_MedicalCenter_2F_Text_WardDoor::
	.string "The ward is quiet.\n"
	.string "The patients are resting.$"
