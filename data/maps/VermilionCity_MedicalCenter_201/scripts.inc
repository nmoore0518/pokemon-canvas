@ Pokémon Canvas: the Universal Pokémon Medical Center, room 201 (tools/gym_hack/build_medical_center.py).
@ Lines are drafts: Nick writes the final ones.

VermilionCity_MedicalCenter_201_MapScripts::
	.byte 0

VermilionCity_MedicalCenter_201_EventScript_Patient::
	lock
	faceplayer
	playmoncry SPECIES_PIKACHU, CRY_MODE_WEAK
	msgbox VermilionCity_MedicalCenter_201_Text_Patient
	waitmoncry
	release
	end

VermilionCity_MedicalCenter_201_EventScript_Visitor0::
	msgbox VermilionCity_MedicalCenter_201_Text_Visitor0, MSGBOX_NPC
	end

VermilionCity_MedicalCenter_201_EventScript_Visitor1::
	msgbox VermilionCity_MedicalCenter_201_Text_Visitor1, MSGBOX_NPC
	end

VermilionCity_MedicalCenter_201_Text_Patient::
	.string "PIKACHU: Pi… pika-CHOO!$"

VermilionCity_MedicalCenter_201_Text_Visitor0::
	.string "My PIKACHU has a cold. Every\n"
	.string "sneeze is a little spark!$"

VermilionCity_MedicalCenter_201_Text_Visitor1::
	.string "NURSE: Its fever's coming down.\n"
	.string "It'll be home in a day or two.$"
