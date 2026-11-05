@ Pokémon Canvas: the Universal Pokémon Medical Center, 1F: reception and the ER (src/medical_center.c).
@ The triage puzzle: six patients, each with a symptom; the right medicine for each (FLAG_TRIAGE_CURED
@ + the patient). All six: the triage nurse's LUM BERRIES and FULL HEALS. Lines are drafts: Nick writes
@ the final ones.

VermilionCity_MedicalCenter_1F_MapScripts::
	.byte 0

VermilionCity_MedicalCenter_1F_EventScript_Receptionist::
	lock
	faceplayer
	msgbox VermilionCity_MedicalCenter_1F_Text_Welcome, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, VermilionCity_MedicalCenter_1F_EventScript_ReceptionistBye
	msgbox VermilionCity_MedicalCenter_1F_Text_RightAway
	closemessage
	fadescreen FADE_TO_BLACK
	playfanfare MUS_HEAL
	special HealPlayerParty
	waitfanfare
	fadescreen FADE_FROM_BLACK
	msgbox VermilionCity_MedicalCenter_1F_Text_AllBetter
	release
	end

VermilionCity_MedicalCenter_1F_EventScript_ReceptionistBye::
	msgbox VermilionCity_MedicalCenter_1F_Text_TakeCare
	release
	end

VermilionCity_MedicalCenter_1F_EventScript_Chansey::
	lock
	faceplayer
	playmoncry SPECIES_CHANSEY, CRY_MODE_NORMAL
	msgbox VermilionCity_MedicalCenter_1F_Text_Chansey
	waitmoncry
	release
	end

@ ---- Triage ----
VermilionCity_MedicalCenter_1F_EventScript_TriageNurse::
	lock
	faceplayer
	goto_if_set FLAG_TRIAGE_REWARD, VermilionCity_MedicalCenter_1F_EventScript_TriageThanks
	call VermilionCity_MedicalCenter_1F_EventScript_CountCured
	goto_if_eq VAR_0x8006, 6, VermilionCity_MedicalCenter_1F_EventScript_TriageReward
	msgbox VermilionCity_MedicalCenter_1F_Text_TriageAsk
	release
	end

VermilionCity_MedicalCenter_1F_EventScript_TriageReward::
	msgbox VermilionCity_MedicalCenter_1F_Text_TriageDone
	setvar VAR_0x8009, ITEM_LUM_BERRY
	call VermilionCity_MedicalCenter_1F_EventScript_GiveFive
	setvar VAR_0x8009, ITEM_FULL_HEAL
	call VermilionCity_MedicalCenter_1F_EventScript_GiveFive
	setflag FLAG_TRIAGE_REWARD
	msgbox VermilionCity_MedicalCenter_1F_Text_TriageUpstairs
	release
	end

VermilionCity_MedicalCenter_1F_EventScript_GiveFive::
	giveitem VAR_0x8009, 5
	goto_if_eq VAR_RESULT, TRUE, VermilionCity_MedicalCenter_1F_EventScript_GiveFiveDone
	addpcitem VAR_0x8009, 5
	bufferitemname STR_VAR_2, VAR_0x8009
	msgbox GymHack_Text_GiftSentToPC
VermilionCity_MedicalCenter_1F_EventScript_GiveFiveDone::
	return

VermilionCity_MedicalCenter_1F_EventScript_TriageThanks::
	msgbox VermilionCity_MedicalCenter_1F_Text_TriageThanks
	release
	end

@ VAR_0x8006 = how many of the six are treated
VermilionCity_MedicalCenter_1F_EventScript_CountCured::
	setvar VAR_0x8006, 0
	call_if_set FLAG_TRIAGE_CURED + 0, VermilionCity_MedicalCenter_1F_EventScript_AddCured
	call_if_set FLAG_TRIAGE_CURED + 1, VermilionCity_MedicalCenter_1F_EventScript_AddCured
	call_if_set FLAG_TRIAGE_CURED + 2, VermilionCity_MedicalCenter_1F_EventScript_AddCured
	call_if_set FLAG_TRIAGE_CURED + 3, VermilionCity_MedicalCenter_1F_EventScript_AddCured
	call_if_set FLAG_TRIAGE_CURED + 4, VermilionCity_MedicalCenter_1F_EventScript_AddCured
	call_if_set FLAG_TRIAGE_CURED + 5, VermilionCity_MedicalCenter_1F_EventScript_AddCured
	return

VermilionCity_MedicalCenter_1F_EventScript_AddCured::
	addvar VAR_0x8006, 1
	return

@ A patient: VAR_0x8004 the patient (0-5), VAR_0x8005 the right medicine (0-5: the menu's order),
@ VAR_0x8007 its species (for its cry)
.macro patient n:req, species:req, cure:req, chart:req
	lock
	faceplayer
	setvar VAR_0x8004, \n
	setvar VAR_0x8005, \cure
	setvar VAR_0x8007, \species
	goto_if_set FLAG_TRIAGE_CURED + \n, VermilionCity_MedicalCenter_1F_EventScript_PatientCured
	msgbox \chart
	goto VermilionCity_MedicalCenter_1F_EventScript_Treat
.endm

VermilionCity_MedicalCenter_1F_EventScript_Patient0::
	patient 0, SPECIES_PSYDUCK, 0, VermilionCity_MedicalCenter_1F_Text_Chart0
VermilionCity_MedicalCenter_1F_EventScript_Patient1::
	patient 1, SPECIES_RATTATA, 1, VermilionCity_MedicalCenter_1F_Text_Chart1
VermilionCity_MedicalCenter_1F_EventScript_Patient2::
	patient 2, SPECIES_PIDGEY, 2, VermilionCity_MedicalCenter_1F_Text_Chart2
VermilionCity_MedicalCenter_1F_EventScript_Patient3::
	patient 3, SPECIES_JIGGLYPUFF, 3, VermilionCity_MedicalCenter_1F_Text_Chart3
VermilionCity_MedicalCenter_1F_EventScript_Patient4::
	patient 4, SPECIES_CHARMANDER, 4, VermilionCity_MedicalCenter_1F_Text_Chart4
VermilionCity_MedicalCenter_1F_EventScript_Patient5::
	patient 5, SPECIES_SPINDA, 5, VermilionCity_MedicalCenter_1F_Text_Chart5

VermilionCity_MedicalCenter_1F_EventScript_Treat::
	message VermilionCity_MedicalCenter_1F_Text_GiveWhich
	waitmessage
	dynmultichoice 0, 0, FALSE, 6, 0, DYN_MULTICHOICE_CB_NONE, VermilionCity_MedicalCenter_1F_Text_BurnHeal, VermilionCity_MedicalCenter_1F_Text_Antidote, VermilionCity_MedicalCenter_1F_Text_ParlyzHeal, VermilionCity_MedicalCenter_1F_Text_Awakening, VermilionCity_MedicalCenter_1F_Text_IceHeal, VermilionCity_MedicalCenter_1F_Text_PersimBerry
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, VermilionCity_MedicalCenter_1F_EventScript_TreatCancel
	goto_if_ne VAR_RESULT, VAR_0x8005, VermilionCity_MedicalCenter_1F_EventScript_TreatWrong
	closemessage
	playse SE_USE_ITEM
	waitse
	playmoncry VAR_0x8007, CRY_MODE_NORMAL
	waitmoncry
	call VermilionCity_MedicalCenter_1F_EventScript_SetCured
	msgbox VermilionCity_MedicalCenter_1F_Text_TreatRight
	call VermilionCity_MedicalCenter_1F_EventScript_CountCured
	goto_if_eq VAR_0x8006, 6, VermilionCity_MedicalCenter_1F_EventScript_AllTreated
	release
	end

VermilionCity_MedicalCenter_1F_EventScript_AllTreated::
	msgbox VermilionCity_MedicalCenter_1F_Text_AllTreated
	release
	end

VermilionCity_MedicalCenter_1F_EventScript_SetCured::
	switch VAR_0x8004
	case 0, VermilionCity_MedicalCenter_1F_EventScript_SetCured0
	case 1, VermilionCity_MedicalCenter_1F_EventScript_SetCured1
	case 2, VermilionCity_MedicalCenter_1F_EventScript_SetCured2
	case 3, VermilionCity_MedicalCenter_1F_EventScript_SetCured3
	case 4, VermilionCity_MedicalCenter_1F_EventScript_SetCured4
	case 5, VermilionCity_MedicalCenter_1F_EventScript_SetCured5
	return
VermilionCity_MedicalCenter_1F_EventScript_SetCured0::
	setflag FLAG_TRIAGE_CURED + 0
	return
VermilionCity_MedicalCenter_1F_EventScript_SetCured1::
	setflag FLAG_TRIAGE_CURED + 1
	return
VermilionCity_MedicalCenter_1F_EventScript_SetCured2::
	setflag FLAG_TRIAGE_CURED + 2
	return
VermilionCity_MedicalCenter_1F_EventScript_SetCured3::
	setflag FLAG_TRIAGE_CURED + 3
	return
VermilionCity_MedicalCenter_1F_EventScript_SetCured4::
	setflag FLAG_TRIAGE_CURED + 4
	return
VermilionCity_MedicalCenter_1F_EventScript_SetCured5::
	setflag FLAG_TRIAGE_CURED + 5
	return

VermilionCity_MedicalCenter_1F_EventScript_TreatWrong::
	closemessage
	playse SE_USE_ITEM
	waitse
	playmoncry VAR_0x8007, CRY_MODE_WEAK
	waitmoncry
	msgbox VermilionCity_MedicalCenter_1F_Text_TreatWrong
	release
	end

VermilionCity_MedicalCenter_1F_EventScript_TreatCancel::
	closemessage
	release
	end

VermilionCity_MedicalCenter_1F_EventScript_PatientCured::
	playmoncry VAR_0x8007, CRY_MODE_NORMAL
	msgbox VermilionCity_MedicalCenter_1F_Text_PatientHappy
	waitmoncry
	release
	end

@ ---- The pharmacy ----
VermilionCity_MedicalCenter_1F_EventScript_Pharmacist::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart VermilionCity_MedicalCenter_1F_Items
	msgbox gText_PleaseComeAgain
	release
	end

	.align 2
VermilionCity_MedicalCenter_1F_Items::
	.2byte ITEM_POTION
	.2byte ITEM_SUPER_POTION
	.2byte ITEM_HYPER_POTION
	.2byte ITEM_MAX_POTION
	.2byte ITEM_REVIVE
	.2byte ITEM_ANTIDOTE
	.2byte ITEM_PARALYZE_HEAL
	.2byte ITEM_AWAKENING
	.2byte ITEM_BURN_HEAL
	.2byte ITEM_ICE_HEAL
	.2byte ITEM_FULL_HEAL
	pokemartlistend

@ ---- Visitors ----
VermilionCity_MedicalCenter_1F_EventScript_Woman::
	msgbox VermilionCity_MedicalCenter_1F_Text_Woman, MSGBOX_NPC
	end

VermilionCity_MedicalCenter_1F_EventScript_Youngster::
	msgbox VermilionCity_MedicalCenter_1F_Text_Youngster, MSGBOX_NPC
	end

VermilionCity_MedicalCenter_1F_EventScript_OldMan::
	msgbox VermilionCity_MedicalCenter_1F_Text_OldMan, MSGBOX_NPC
	end

VermilionCity_MedicalCenter_1F_EventScript_Girl::
	msgbox VermilionCity_MedicalCenter_1F_Text_Girl, MSGBOX_NPC
	end

VermilionCity_MedicalCenter_1F_EventScript_Happiny::
	lock
	faceplayer
	playmoncry SPECIES_HAPPINY, CRY_MODE_NORMAL
	msgbox VermilionCity_MedicalCenter_1F_Text_Happiny
	waitmoncry
	release
	end

VermilionCity_MedicalCenter_1F_EventScript_Cabinet::
	msgbox VermilionCity_MedicalCenter_1F_Text_Cabinet, MSGBOX_SIGN
	end

VermilionCity_MedicalCenter_1F_EventScript_Plaque::
	msgbox VermilionCity_MedicalCenter_1F_Text_Plaque, MSGBOX_SIGN
	end

@ ---- The foreman and his crew ----
VermilionCity_MedicalCenter_1F_EventScript_Foreman::
	msgbox VermilionCity_MedicalCenter_1F_Text_Foreman, MSGBOX_NPC
	end

.macro crewmon species:req, text:req
	lock
	faceplayer
	playmoncry \species, CRY_MODE_NORMAL
	msgbox \text
	waitmoncry
	release
	end
.endm

VermilionCity_MedicalCenter_1F_EventScript_Machop::
	crewmon SPECIES_MACHOP, VermilionCity_MedicalCenter_1F_Text_Machop
VermilionCity_MedicalCenter_1F_EventScript_Machoke::
	crewmon SPECIES_MACHOKE, VermilionCity_MedicalCenter_1F_Text_Machoke
VermilionCity_MedicalCenter_1F_EventScript_Machamp::
	crewmon SPECIES_MACHAMP, VermilionCity_MedicalCenter_1F_Text_Machamp
VermilionCity_MedicalCenter_1F_EventScript_ShinyMachamp::
	crewmon SPECIES_MACHAMP, VermilionCity_MedicalCenter_1F_Text_ShinyMachamp

@ ---- Text ----
VermilionCity_MedicalCenter_1F_Text_Welcome::
	.string "Welcome to the UNIVERSAL POKéMON\n"
	.string "MEDICAL CENTER!\p"
	.string "Shall we look after your POKéMON?$"

VermilionCity_MedicalCenter_1F_Text_RightAway::
	.string "Right away!$"

VermilionCity_MedicalCenter_1F_Text_AllBetter::
	.string "All better!\n"
	.string "Take care of them out there!$"

VermilionCity_MedicalCenter_1F_Text_TakeCare::
	.string "Take care!$"

VermilionCity_MedicalCenter_1F_Text_Chansey::
	.string "CHANSEY: Chaaan-sey!$"

VermilionCity_MedicalCenter_1F_Text_TriageAsk::
	.string "We're swamped in the ER today!\p"
	.string "Could you help? Read each patient's\n"
	.string "chart and give it the right\l"
	.string "medicine.\p"
	.string "Don't worry, we'll supply the\n"
	.string "medicine!$"

VermilionCity_MedicalCenter_1F_Text_TriageDone::
	.string "You treated every one of them!\n"
	.string "Thank you so much!\p"
	.string "Please, take these.$"

VermilionCity_MedicalCenter_1F_Text_TriageUpstairs::
	.string "The wards upstairs could use a\n"
	.string "hand, too. It's chaos up there!$"

VermilionCity_MedicalCenter_1F_Text_TriageThanks::
	.string "The ER's never run so smoothly!\n"
	.string "Thanks again!$"

VermilionCity_MedicalCenter_1F_Text_Chart0::
	.string "PATIENT CHART: PSYDUCK\p"
	.string "Red, blistered skin. Too hot to\n"
	.string "touch. Keeps holding its head.$"

VermilionCity_MedicalCenter_1F_Text_Chart1::
	.string "PATIENT CHART: RATTATA\p"
	.string "Purple in the face and shivering.\n"
	.string "Bitten by a TENTACOOL.$"

VermilionCity_MedicalCenter_1F_Text_Chart2::
	.string "PATIENT CHART: PIDGEY\p"
	.string "Can't move a feather. Little\n"
	.string "sparks crackle off its wings.$"

VermilionCity_MedicalCenter_1F_Text_Chart3::
	.string "PATIENT CHART: JIGGLYPUFF\p"
	.string "Sang itself a lullaby and won't\n"
	.string "wake up. Snoring loudly.$"

VermilionCity_MedicalCenter_1F_Text_Chart4::
	.string "PATIENT CHART: CHARMANDER\p"
	.string "Encased in ice up to the tail.\n"
	.string "Its flame is barely flickering!$"

VermilionCity_MedicalCenter_1F_Text_Chart5::
	.string "PATIENT CHART: SPINDA\p"
	.string "Walking in circles, bumping into\n"
	.string "things. More than usual, even.$"

VermilionCity_MedicalCenter_1F_Text_GiveWhich::
	.string "Give it which medicine?$"

VermilionCity_MedicalCenter_1F_Text_BurnHeal::
	.string "BURN HEAL$"

VermilionCity_MedicalCenter_1F_Text_Antidote::
	.string "ANTIDOTE$"

VermilionCity_MedicalCenter_1F_Text_ParlyzHeal::
	.string "PARLYZ HEAL$"

VermilionCity_MedicalCenter_1F_Text_Awakening::
	.string "AWAKENING$"

VermilionCity_MedicalCenter_1F_Text_IceHeal::
	.string "ICE HEAL$"

VermilionCity_MedicalCenter_1F_Text_PersimBerry::
	.string "PERSIM BERRY$"

VermilionCity_MedicalCenter_1F_Text_TreatRight::
	.string "It looks much better!$"

VermilionCity_MedicalCenter_1F_Text_AllTreated::
	.string "That's every patient treated!\n"
	.string "The triage nurse looks impressed.$"

VermilionCity_MedicalCenter_1F_Text_TreatWrong::
	.string "Uh-oh… That didn't help at all!\n"
	.string "Better read the chart again.$"

VermilionCity_MedicalCenter_1F_Text_PatientHappy::
	.string "It's resting happily.$"

VermilionCity_MedicalCenter_1F_Text_OldMan::
	.string "My POKéMON ate something off the\n"
	.string "floor of the S.S. ANNE again.\p"
	.string "Third time this month!$"

VermilionCity_MedicalCenter_1F_Text_Girl::
	.string "HAPPINY's here to visit the\n"
	.string "CHANSEY! It wants to be a nurse\l"
	.string "when it grows up.$"

VermilionCity_MedicalCenter_1F_Text_Happiny::
	.string "HAPPINY: Happi! Happi!$"

VermilionCity_MedicalCenter_1F_Text_Cabinet::
	.string "Rows and rows of medicine.\n"
	.string "Everything's neatly labeled.$"

VermilionCity_MedicalCenter_1F_Text_Plaque::
	.string "UNIVERSAL POKéMON MEDICAL CENTER\n"
	.string "RECEPTION$"

VermilionCity_MedicalCenter_1F_Text_Woman::
	.string "The wait's not bad at all.\n"
	.string "They see you right away here!$"

VermilionCity_MedicalCenter_1F_Text_Youngster::
	.string "I'm not sick. I just come to watch\n"
	.string "the CHANSEY work. They're so fast!$"

VermilionCity_MedicalCenter_1F_Text_Foreman::
	.string "Not bad for a building crew of\n"
	.string "four, huh?\p"
	.string "The boys still help out around\n"
	.string "here. Heavy lifting, mostly.$"

VermilionCity_MedicalCenter_1F_Text_Machop::
	.string "MACHOP: Guoh! Gogogoh!$"

VermilionCity_MedicalCenter_1F_Text_Machoke::
	.string "MACHOKE: Machoke!$"

VermilionCity_MedicalCenter_1F_Text_Machamp::
	.string "MACHAMP: Maaachamp!$"

VermilionCity_MedicalCenter_1F_Text_ShinyMachamp::
	.string "MACHAMP: Champ! Champ!\p"
	.string "It's flexing proudly.$"
