@ Gym Trainer hack: quality of life (src/item_use.c ItemUseOutOfBattle_PalmPilot).
@ The PALM PILOT: Bill's portable PC, from the bag (or SELECT) almost anywhere.
@ The quick heal pad in every Pokémon Center (MB_HEAL_PAD, src/field_control_avatar.c).

EventScript_PalmPilot::
	lockall
	playse SE_PC_ON
	msgbox Text_PalmPilotBoot
EventScript_PalmPilotMenu::
	message gText_WhichPCShouldBeAccessed
	waitmessage
	special ScriptMenu_CreatePCMultichoice
	switch VAR_RESULT
	case 0, EventScript_PalmPilotStorage
	case 1, EventScript_PalmPilotPlayersPC
	case 2, EventScript_PalmPilotHallOfFame
	goto EventScript_PalmPilotOff
	end

EventScript_PalmPilotStorage::
	playse SE_PC_LOGIN
	call_if_unset FLAG_SYS_PC_LANETTE, EventScript_AccessSomeonesPC
	call_if_set FLAG_SYS_PC_LANETTE, EventScript_AccessLanettesPC
	msgbox gText_StorageSystemOpened, MSGBOX_DEFAULT
	special ShowPokemonStorageSystemPC
	goto EventScript_PalmPilotMenu
	end

EventScript_PalmPilotPlayersPC::
	playse SE_PC_LOGIN
	msgbox gText_AccessedPlayersPC, MSGBOX_DEFAULT
	special PlayerPC
	goto EventScript_PalmPilotMenu
	end

EventScript_PalmPilotHallOfFame::
	goto_if_unset FLAG_SYS_GAME_CLEAR, EventScript_PalmPilotOff
	playse SE_PC_LOGIN
	special AccessHallOfFamePC
	goto EventScript_PalmPilotMenu
	end

EventScript_PalmPilotOff::
	playse SE_PC_OFF
	releaseall
	end

@ Bill gives it with the S.S. Ticket (Route25_SeaCottage), or later if it was missed
EventScript_BillGivesPalmPilot::
	checkitem ITEM_PALM_PILOT
	goto_if_eq VAR_RESULT, TRUE, EventScript_PalmPilotReturn
	msgbox Text_BillPalmPilot
	giveitem ITEM_PALM_PILOT
	goto_if_eq VAR_RESULT, FALSE, EventScript_PalmPilotReturn
	msgbox Text_BillPalmPilotHowTo
	msgbox Text_BillPalmPilotSecret
EventScript_PalmPilotReturn::
	return

@ The Hot Air Balloon (Fly): Oak gives it in the gym scene; if it was missed (an older save),
@ he gives it in his lab. Route 16's HM02 girl gives it too, if the player has none.
EventScript_GiveHotAirBalloon::
	checkitem ITEM_HOT_AIR_BALLOON
	goto_if_eq VAR_RESULT, TRUE, EventScript_PalmPilotReturn
	giveitem ITEM_HOT_AIR_BALLOON
	return

EventScript_OakGivesMissedBalloon::
	goto_if_unset FLAG_PALLET_GYM_OPEN, EventScript_PalmPilotReturn
	checkitem ITEM_HOT_AIR_BALLOON
	goto_if_eq VAR_RESULT, TRUE, EventScript_PalmPilotReturn
	msgbox Text_OakMissedBalloon
	giveitem ITEM_HOT_AIR_BALLOON
	return

@ New gym decorations were unlocked (GymDecor_CheckNewUnlocks, checked as the player walks)
EventScript_NewGymDecor::
	lockall
	playse SE_PC_LOGIN
	msgbox Text_NewGymDecor, MSGBOX_SIGN
	releaseall
	end

@ Stepping onto the heal pad
EventScript_QuickHealPad::
	lockall
	special HealPlayerParty
	callnative UpdateFollowingPokemon
	playfanfare MUS_HEAL
	waitfanfare
	releaseall
	end

Text_PalmPilotBoot::
	.string "{PLAYER} booted up the PALM PILOT.$"

Text_PalmPilotNoSignal::
	.string "The PALM PILOT can't get a signal\n"
	.string "in here!$"

Text_BillPalmPilot::
	.string "BILL: Oh, and one more thing!\p"
	.string "You know my POKéMON Storage\n"
	.string "System? I've been tinkering…\p"
	.string "and I made a PC that fits in your\n"
	.string "pocket! I call it the PALM PILOT.\p"
	.string "It links to the Storage System\n"
	.string "from almost anywhere.\p"
	.string "Not inside the POKéMON LEAGUE,\n"
	.string "though. They jam the signal.\p"
	.string "Here, take this one. Tell me how\n"
	.string "it works out!$"

Text_BillPalmPilotHowTo::
	.string "BILL: Use it from your BAG, or\n"
	.string "register it to the SELECT Button.$"

Text_BillPalmPilotSecret::
	.string "BILL: Oh, and if anyone asks…\n"
	.string "I've only been working on the PC.\p"
	.string "Some of my other experiments are,\n"
	.string "uh… not exactly approved.\p"
	.string "Nothing to worry about! Probably.\n"
	.string "Heh heh…$"

Text_HealPadInnovator::
	.string "See that glowing pad on the floor?\n"
	.string "It's brand-new tech from SILPH!\p"
	.string "Step on it, and your POKéMON are\n"
	.string "healed in a flash. No waiting!\p"
	.string "…But a machine can't smile at you\n"
	.string "or ask how your journey's going.\p"
	.string "That's why there will always be\n"
	.string "a NURSE behind the counter.$"

Text_OakMissedBalloon::
	.string "OAK: Ah, {PLAYER}! I nearly forgot.\p"
	.string "Take this HOT AIR BALLOON. It will\n"
	.string "float you to any town you've\l"
	.string "visited, even back to your GYM!$"

Text_NewGymDecor::
	.string "New GYM decorations available!\n"
	.string "Check the GYM PC.$"
