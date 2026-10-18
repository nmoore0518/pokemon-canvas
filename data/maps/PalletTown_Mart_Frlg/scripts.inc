PalletTown_Mart_Frlg_MapScripts::
	.byte 0

PalletTown_Mart_EventScript_Youngster::
	msgbox PalletTown_Mart_Text_BoughtWeirdFishFromShadyGuy, MSGBOX_NPC
	end

PalletTown_Mart_EventScript_Boy::
	msgbox PalletTown_Mart_Text_GoodThingsIfRaiseMonsDiligently, MSGBOX_NPC
	end

PalletTown_Mart_EventScript_Clerk::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart PalletTown_Mart_Items
	msgbox gText_PleaseComeAgain
	release
	end

	.align 2
PalletTown_Mart_Items::
	.2byte ITEM_POKE_BALL
	.2byte ITEM_POTION
	.2byte ITEM_ANTIDOTE
	.2byte ITEM_PARALYZE_HEAL
	.2byte ITEM_AWAKENING
	.2byte ITEM_BURN_HEAL
	.2byte ITEM_ICE_HEAL
	.2byte ITEM_REPEL
	.2byte ITEM_NONE
	release
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

