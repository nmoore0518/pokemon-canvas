PalletTown_House1_Frlg_MapScripts::
	.byte 0

PalletTown_House1_EventScript_OldMan::
	msgbox PalletTown_House1_Text_GymOnTheHill, MSGBOX_NPC
	end

PalletTown_House1_EventScript_LittleBoy::
	msgbox PalletTown_House1_Text_WhichType, MSGBOX_NPC
	end

PalletTown_House1_EventScript_Pidgey::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_PIDGEY, CRY_MODE_NORMAL
	msgbox PalletTown_House1_Text_Pidgey
	waitmoncry
	release
	end

PalletTown_House1_EventScript_Sign::
	msgbox PalletTown_House1_Text_Sign, MSGBOX_SIGN
	end

PalletTown_House1_Text_GymOnTheHill::
	.string "PROF. OAK has had folks hauling\n"
	.string "things up that rocky hill all month.\p"
	.string "They say it's a POKéMON GYM!\n"
	.string "In quiet little PALLET TOWN!$"

PalletTown_House1_Text_WhichType::
	.string "If I had a GYM, it'd be all\n"
	.string "BUG-type POKéMON!\p"
	.string "What type would yours be?$"

PalletTown_House1_Text_Pidgey::
	.string "PIDGEY: Pijji!$"

PalletTown_House1_Text_Sign::
	.string "Family photo of the ASHFORDS.\n"
	.string "Everyone is squinting at the sun.$"
