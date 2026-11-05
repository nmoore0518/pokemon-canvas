@ Pokémon Canvas: the Universal Pokémon Medical Center, room 203 (tools/gym_hack/build_medical_center.py).
@ Lines are drafts: Nick writes the final ones.

VermilionCity_MedicalCenter_203_MapScripts::
	.byte 0

VermilionCity_MedicalCenter_203_EventScript_Patient::
	lock
	faceplayer
	playmoncry SPECIES_TOGEPI, CRY_MODE_NORMAL
	msgbox VermilionCity_MedicalCenter_203_Text_Patient
	waitmoncry
	release
	end

VermilionCity_MedicalCenter_203_EventScript_Visitor0::
	msgbox VermilionCity_MedicalCenter_203_Text_Visitor0, MSGBOX_NPC
	end

VermilionCity_MedicalCenter_203_EventScript_Visitor1::
	lock
	faceplayer
	playmoncry SPECIES_CHANSEY, CRY_MODE_NORMAL
	msgbox VermilionCity_MedicalCenter_203_Text_Visitor1
	waitmoncry
	release
	end

VermilionCity_MedicalCenter_203_Text_Patient::
	.string "TOGEPI: Toge-toge-priii!$"

VermilionCity_MedicalCenter_203_Text_Visitor0::
	.string "It hatched last night! The CHANSEY\n"
	.string "here looked after the EGG for me.$"

VermilionCity_MedicalCenter_203_Text_Visitor1::
	.string "CHANSEY: Chansey!$"
