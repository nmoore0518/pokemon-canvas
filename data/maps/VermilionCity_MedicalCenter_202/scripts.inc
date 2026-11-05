@ Pokémon Canvas: the Universal Pokémon Medical Center, room 202 (tools/gym_hack/build_medical_center.py).
@ Lines are drafts: Nick writes the final ones.

VermilionCity_MedicalCenter_202_MapScripts::
	.byte 0

VermilionCity_MedicalCenter_202_EventScript_Patient::
	lock
	faceplayer
	playmoncry SPECIES_GROWLITHE, CRY_MODE_WEAK
	msgbox VermilionCity_MedicalCenter_202_Text_Patient
	waitmoncry
	release
	end

VermilionCity_MedicalCenter_202_EventScript_Visitor0::
	msgbox VermilionCity_MedicalCenter_202_Text_Visitor0, MSGBOX_NPC
	end

VermilionCity_MedicalCenter_202_EventScript_Visitor1::
	msgbox VermilionCity_MedicalCenter_202_Text_Visitor1, MSGBOX_NPC
	end

VermilionCity_MedicalCenter_202_Text_Patient::
	.string "GROWLITHE: Arf…\n"
	.string "It's wagging its tail.$"

VermilionCity_MedicalCenter_202_Text_Visitor0::
	.string "Stay still, now. The doctor says\n"
	.string "no running for a week!$"

VermilionCity_MedicalCenter_202_Text_Visitor1::
	.string "DOCTOR: Just a sprain. It'll be\n"
	.string "chasing PONYTA again in no time.$"
