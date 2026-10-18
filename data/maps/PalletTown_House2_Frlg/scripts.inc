PalletTown_House2_Frlg_MapScripts::
	.byte 0

PalletTown_House2_EventScript_Woman::
	msgbox PalletTown_House2_Text_NewShops, MSGBOX_NPC
	end

PalletTown_House2_EventScript_LittleGirl::
	msgbox PalletTown_House2_Text_Waterfall, MSGBOX_NPC
	end

PalletTown_House2_EventScript_Meowth::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_MEOWTH, CRY_MODE_NORMAL
	msgbox PalletTown_House2_Text_Meowth
	waitmoncry
	release
	end

PalletTown_House2_EventScript_Sign::
	msgbox PalletTown_House2_Text_Sign, MSGBOX_SIGN
	end

PalletTown_House2_Text_NewShops::
	.string "PALLET TOWN has its own POKéMON\n"
	.string "CENTER and POKé MART now.\p"
	.string "No more walking to VIRIDIAN CITY\n"
	.string "for POTIONS!$"

PalletTown_House2_Text_Waterfall::
	.string "The waterfall south of town runs\n"
	.string "all the way down to the sea.\p"
	.string "I want to SURF on it someday!$"

PalletTown_House2_Text_Meowth::
	.string "MEOWTH: Meow!$"

PalletTown_House2_Text_Sign::
	.string "A cross-stitch that reads:\n"
	.string "“Home is where the PALLET is.”$"
