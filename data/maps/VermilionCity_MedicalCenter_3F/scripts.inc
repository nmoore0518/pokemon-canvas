@ Pokémon Canvas: the Universal Pokémon Medical Center, 3F: the research lab (src/medical_center.c).
@ The head doctor describes Pokérus; five microscopes (pictures: MedCenter_ShowSlide); the right one
@ (MEDCENTER_POKERUS_SLIDE) sets FLAG_MICROSCOPE_SOLVED. Then the head nurse's physical (a battle,
@ FLAG_MEDCENTER_PHYSICAL), then Pokérus for one Pokémon each "day" of steps (FLAG_POKERUS_GIVEN_TODAY).
@ Lines are drafts: Nick writes the final ones.

VermilionCity_MedicalCenter_3F_MapScripts::
	.byte 0

VermilionCity_MedicalCenter_3F_EventScript_Doctor::
	lock
	faceplayer
	goto_if_set FLAG_MICROSCOPE_SOLVED, VermilionCity_MedicalCenter_3F_EventScript_DoctorAfter
	msgbox VermilionCity_MedicalCenter_3F_Text_DoctorAsk
	release
	end

VermilionCity_MedicalCenter_3F_EventScript_DoctorAfter::
	msgbox VermilionCity_MedicalCenter_3F_Text_DoctorAfter
	release
	end

@ ---- The microscopes: VAR_0x8004 the slide ----
.macro microscope n:req
	lockall
	setvar VAR_0x8004, \n
	goto VermilionCity_MedicalCenter_3F_EventScript_Microscope
.endm

VermilionCity_MedicalCenter_3F_EventScript_Microscope0::
	microscope 0
VermilionCity_MedicalCenter_3F_EventScript_Microscope1::
	microscope 1
VermilionCity_MedicalCenter_3F_EventScript_Microscope2::
	microscope 2
VermilionCity_MedicalCenter_3F_EventScript_Microscope3::
	microscope 3
VermilionCity_MedicalCenter_3F_EventScript_Microscope4::
	microscope 4

VermilionCity_MedicalCenter_3F_EventScript_Microscope::
	msgbox VermilionCity_MedicalCenter_3F_Text_LookIn
	special MedCenter_ShowSlide
	goto_if_set FLAG_MICROSCOPE_SOLVED, VermilionCity_MedicalCenter_3F_EventScript_MicroscopeLook
	msgbox VermilionCity_MedicalCenter_3F_Text_IsThisIt, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, VermilionCity_MedicalCenter_3F_EventScript_MicroscopeDone
	special MedCenter_HideSlide
	goto_if_eq VAR_0x8004, MEDCENTER_POKERUS_SLIDE, VermilionCity_MedicalCenter_3F_EventScript_FoundIt
	switch VAR_0x8004
	case 0, VermilionCity_MedicalCenter_3F_EventScript_Wrong0
	case 1, VermilionCity_MedicalCenter_3F_EventScript_Wrong1
	case 3, VermilionCity_MedicalCenter_3F_EventScript_Wrong3
	case 4, VermilionCity_MedicalCenter_3F_EventScript_Wrong4
	releaseall
	end

VermilionCity_MedicalCenter_3F_EventScript_MicroscopeLook::
	msgbox VermilionCity_MedicalCenter_3F_Text_JustLooking
VermilionCity_MedicalCenter_3F_EventScript_MicroscopeDone::
	special MedCenter_HideSlide
	releaseall
	end

VermilionCity_MedicalCenter_3F_EventScript_Wrong0::
	msgbox VermilionCity_MedicalCenter_3F_Text_Wrong0
	releaseall
	end

VermilionCity_MedicalCenter_3F_EventScript_Wrong1::
	msgbox VermilionCity_MedicalCenter_3F_Text_Wrong1
	releaseall
	end

VermilionCity_MedicalCenter_3F_EventScript_Wrong3::
	msgbox VermilionCity_MedicalCenter_3F_Text_Wrong3
	releaseall
	end

VermilionCity_MedicalCenter_3F_EventScript_Wrong4::
	msgbox VermilionCity_MedicalCenter_3F_Text_Wrong4
	releaseall
	end

VermilionCity_MedicalCenter_3F_EventScript_FoundIt::
	setflag FLAG_MICROSCOPE_SOLVED
	playfanfare MUS_LEVEL_UP
	msgbox VermilionCity_MedicalCenter_3F_Text_FoundIt
	waitfanfare
	releaseall
	end

@ ---- The head nurse: the physical, then Pokérus ----
VermilionCity_MedicalCenter_3F_EventScript_HeadNurse::
	lock
	faceplayer
	goto_if_unset FLAG_MICROSCOPE_SOLVED, VermilionCity_MedicalCenter_3F_EventScript_NurseBusy
	goto_if_unset FLAG_MEDCENTER_PHYSICAL, VermilionCity_MedicalCenter_3F_EventScript_Physical
	goto VermilionCity_MedicalCenter_3F_EventScript_Pokerus
	end

VermilionCity_MedicalCenter_3F_EventScript_NurseBusy::
	msgbox VermilionCity_MedicalCenter_3F_Text_NurseBusy
	release
	end

VermilionCity_MedicalCenter_3F_EventScript_Physical::
	msgbox VermilionCity_MedicalCenter_3F_Text_PhysicalAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, VermilionCity_MedicalCenter_3F_EventScript_PhysicalLater
	msgbox VermilionCity_MedicalCenter_3F_Text_PhysicalStart
	trainerbattle_no_intro TRAINER_HEAD_NURSE, VermilionCity_MedicalCenter_3F_Text_PhysicalDefeat
	setflag FLAG_MEDCENTER_PHYSICAL
	msgbox VermilionCity_MedicalCenter_3F_Text_PhysicalPassed
	goto VermilionCity_MedicalCenter_3F_EventScript_PokerusOffer
	end

VermilionCity_MedicalCenter_3F_EventScript_PhysicalLater::
	msgbox VermilionCity_MedicalCenter_3F_Text_PhysicalLater
	release
	end

VermilionCity_MedicalCenter_3F_EventScript_Pokerus::
	goto_if_set FLAG_POKERUS_GIVEN_TODAY, VermilionCity_MedicalCenter_3F_EventScript_PokerusTomorrow
	msgbox VermilionCity_MedicalCenter_3F_Text_PokerusAgain
VermilionCity_MedicalCenter_3F_EventScript_PokerusOffer::
	msgbox VermilionCity_MedicalCenter_3F_Text_PokerusAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, VermilionCity_MedicalCenter_3F_EventScript_PokerusNo
	msgbox VermilionCity_MedicalCenter_3F_Text_PokerusWhich
	special ChoosePartyMon
	goto_if_ge VAR_0x8004, PARTY_SIZE, VermilionCity_MedicalCenter_3F_EventScript_PokerusNo
	special MedCenter_GivePokerus
	switch VAR_RESULT
	case POKERUS_GIVE_DONE, VermilionCity_MedicalCenter_3F_EventScript_PokerusDone
	case POKERUS_GIVE_HAS_IT, VermilionCity_MedicalCenter_3F_EventScript_PokerusHasIt
	case POKERUS_GIVE_HAD_IT, VermilionCity_MedicalCenter_3F_EventScript_PokerusHadIt
	msgbox VermilionCity_MedicalCenter_3F_Text_PokerusEgg
	release
	end

VermilionCity_MedicalCenter_3F_EventScript_PokerusDone::
	playse SE_USE_ITEM
	waitse
	msgbox VermilionCity_MedicalCenter_3F_Text_PokerusDone
	release
	end

VermilionCity_MedicalCenter_3F_EventScript_PokerusHasIt::
	msgbox VermilionCity_MedicalCenter_3F_Text_PokerusHasIt
	release
	end

VermilionCity_MedicalCenter_3F_EventScript_PokerusHadIt::
	msgbox VermilionCity_MedicalCenter_3F_Text_PokerusHadIt
	release
	end

VermilionCity_MedicalCenter_3F_EventScript_PokerusNo::
	msgbox VermilionCity_MedicalCenter_3F_Text_PokerusNo
	release
	end

VermilionCity_MedicalCenter_3F_EventScript_PokerusTomorrow::
	msgbox VermilionCity_MedicalCenter_3F_Text_PokerusTomorrow
	release
	end

VermilionCity_MedicalCenter_3F_EventScript_Chansey::
	lock
	faceplayer
	playmoncry SPECIES_CHANSEY, CRY_MODE_NORMAL
	msgbox VermilionCity_MedicalCenter_3F_Text_Chansey
	waitmoncry
	release
	end

VermilionCity_MedicalCenter_3F_EventScript_Blissey::
	lock
	faceplayer
	playmoncry SPECIES_BLISSEY, CRY_MODE_NORMAL
	msgbox VermilionCity_MedicalCenter_3F_Text_Blissey
	waitmoncry
	release
	end

VermilionCity_MedicalCenter_3F_EventScript_Snorlax::
	lock
	playmoncry SPECIES_SNORLAX, CRY_MODE_WEAK
	msgbox VermilionCity_MedicalCenter_3F_Text_Snorlax
	waitmoncry
	release
	end

VermilionCity_MedicalCenter_3F_EventScript_XRay::
	msgbox VermilionCity_MedicalCenter_3F_Text_XRay, MSGBOX_SIGN
	end

@ ---- Text ----
VermilionCity_MedicalCenter_3F_Text_DoctorAsk::
	.string "I'm researching POKéRUS, a very\n"
	.string "rare virus. It's harmless, and it\l"
	.string "makes POKéMON grow stronger!\p"
	.string "We have a sample here somewhere,\n"
	.string "but the slides got mixed up.\p"
	.string "POKéRUS is tiny, and pinkish\n"
	.string "purple. Each one is shaped like a\l"
	.string "little star…\p"
	.string "…and they huddle together in small\n"
	.string "clusters. Could you find it?$"

VermilionCity_MedicalCenter_3F_Text_DoctorAfter::
	.string "Thanks to you, the POKéRUS\n"
	.string "research is right on schedule!\p"
	.string "Our head nurse can tell you more.$"

VermilionCity_MedicalCenter_3F_Text_LookIn::
	.string "{PLAYER} looked into the\n"
	.string "microscope…$"

VermilionCity_MedicalCenter_3F_Text_IsThisIt::
	.string "Is this the POKéRUS sample?$"

VermilionCity_MedicalCenter_3F_Text_JustLooking::
	.string "Fascinating…$"

VermilionCity_MedicalCenter_3F_Text_Wrong0::
	.string "DOCTOR: No, those are plant\n"
	.string "cells. From an ODDISH, I think.$"

VermilionCity_MedicalCenter_3F_Text_Wrong1::
	.string "DOCTOR: No, no. Those are just\n"
	.string "ordinary germs.$"

VermilionCity_MedicalCenter_3F_Text_Wrong3::
	.string "DOCTOR: That's a STARYU's cell!\n"
	.string "Much too big, and all alone.$"

VermilionCity_MedicalCenter_3F_Text_Wrong4::
	.string "DOCTOR: Ugh, that's a GRIMER\n"
	.string "sample. Put it back, please.$"

VermilionCity_MedicalCenter_3F_Text_FoundIt::
	.string "DOCTOR: That's it! POKéRUS!\n"
	.string "You've got a sharp eye!$"

VermilionCity_MedicalCenter_3F_Text_NurseBusy::
	.string "The doctor's lost a sample again.\n"
	.string "Would you help him look?$"

VermilionCity_MedicalCenter_3F_Text_PhysicalAsk::
	.string "You found the POKéRUS sample!\p"
	.string "Before I let any POKéMON near it,\n"
	.string "they need a physical. Ready?$"

VermilionCity_MedicalCenter_3F_Text_PhysicalStart::
	.string "Let's see how healthy they are!$"

VermilionCity_MedicalCenter_3F_Text_PhysicalDefeat::
	.string "A clean bill of health!$"

VermilionCity_MedicalCenter_3F_Text_PhysicalPassed::
	.string "Your POKéMON are in wonderful\n"
	.string "shape.$"

VermilionCity_MedicalCenter_3F_Text_PhysicalLater::
	.string "Come back when you're ready.$"

VermilionCity_MedicalCenter_3F_Text_PokerusAgain::
	.string "Back for more POKéRUS?$"

VermilionCity_MedicalCenter_3F_Text_PokerusAsk::
	.string "I can give one POKéMON a dose of\n"
	.string "POKéRUS. It'll pass it on to the\l"
	.string "POKéMON next to it, for a while.\p"
	.string "Shall I?$"

VermilionCity_MedicalCenter_3F_Text_PokerusWhich::
	.string "Which POKéMON?$"

VermilionCity_MedicalCenter_3F_Text_PokerusDone::
	.string "There! {STR_VAR_1} has POKéRUS.\p"
	.string "It'll grow stronger faster, even\n"
	.string "after it's worn off.$"

VermilionCity_MedicalCenter_3F_Text_PokerusHasIt::
	.string "{STR_VAR_1} already has POKéRUS!$"

VermilionCity_MedicalCenter_3F_Text_PokerusHadIt::
	.string "{STR_VAR_1}'s had POKéRUS before.\n"
	.string "It's immune now, but it'll keep\l"
	.string "growing stronger faster.$"

VermilionCity_MedicalCenter_3F_Text_PokerusEgg::
	.string "Not an EGG, silly!$"

VermilionCity_MedicalCenter_3F_Text_PokerusNo::
	.string "Any time.$"

VermilionCity_MedicalCenter_3F_Text_PokerusTomorrow::
	.string "That's my last dose for today.\n"
	.string "Come back after a while!$"

VermilionCity_MedicalCenter_3F_Text_Chansey::
	.string "CHANSEY: Chansey!$"

VermilionCity_MedicalCenter_3F_Text_Blissey::
	.string "BLISSEY: Bliiis-sey!$"

VermilionCity_MedicalCenter_3F_Text_Snorlax::
	.string "SNORLAX is here for a stomach ache.\n"
	.string "It's sleeping it off.$"

VermilionCity_MedicalCenter_3F_Text_XRay::
	.string "An X-ray of a POKéMON's skeleton.\n"
	.string "Whose could it be?$"
