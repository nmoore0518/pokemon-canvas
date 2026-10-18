PalletTown_Mart_Frlg_MapScripts::
	.byte 0

PalletTown_Mart_EventScript_Youngster::
	msgbox PalletTown_Mart_Text_BoughtWeirdFishFromShadyGuy, MSGBOX_NPC
	end

PalletTown_Mart_EventScript_Boy::
	msgbox PalletTown_Mart_Text_GoodThingsIfRaiseMonsDiligently, MSGBOX_NPC
	end

@ The stock depends on the badges (stat training items); see GymHack_PalletMart in src/stat_items.c
PalletTown_Mart_EventScript_Clerk::
	goto GymHack_EventScript_PalletMartClerk
	end

PalletTown_Mart_Text_BoughtWeirdFishFromShadyGuy::
	.string "PROF. OAK says a GYM LEADER must\n"
	.string "know every type matchup by heart.\p"
	.string "That sounds like a lot of homework!$"

PalletTown_Mart_Text_GoodThingsIfRaiseMonsDiligently::
	.string "Good things can happen if you raise\n"
	.string "POKéMON diligently.\p"
	.string "Even the weak ones can surprise\n"
	.string "you if you don't give up on them.$"

