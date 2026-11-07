@ Pokémon Canvas: the pit under Celadon City (src/pit.c, tools/gym_hack/build_pit.py).
@ The way in: walk east into the west wall of the building past the Game Corner's prize house; the
@ password's overheard in the Game Corner. Betting on other people's battles is against League
@ rules (a trainer's prize money is League business; a crowd's money is how fights get thrown), so
@ the pit doesn't exist.
@ (Claude's draft of everyone's words, Nov 6: Nick may rewrite them.)

@ ---- The way in -----------------------------------------------------------------
Pit_EventScript_HiddenDoor::
	lockall
	goto_if_set FLAG_PIT_DOOR_FOUND, Pit_EventScript_HiddenDoorKnown
	msgbox Pit_Text_HiddenDoorFound
	setflag FLAG_PIT_DOOR_FOUND
Pit_EventScript_HiddenDoorKnown::
	playse SE_RG_DOOR
	waitse
	warp MAP_CELADON_CITY_PIT_HALL, 0
	waitstate
	releaseall
	end

Pit_EventScript_HallTransition::
	call_if_set FLAG_PIT_PASSWORD_GIVEN, Pit_EventScript_DoormanAside
	end

Pit_EventScript_DoormanAside::
	setobjectxyperm LOCALID_PIT_DOORMAN, 5, 2
	setobjectmovementtype LOCALID_PIT_DOORMAN, MOVEMENT_TYPE_FACE_LEFT
	return

Pit_EventScript_Doorman::
	lock
	faceplayer
	goto_if_set FLAG_PIT_PASSWORD_GIVEN, Pit_EventScript_DoormanLetIn
	msgbox Pit_Text_DoormanStop
	closemessage
	special Pit_AskPassword
	waitstate
	special Pit_CheckPassword
	goto_if_eq VAR_RESULT, FALSE, Pit_EventScript_DoormanWrong
	msgbox Pit_Text_DoormanRight
	closemessage
	applymovement LOCALID_PIT_DOORMAN, Pit_Movement_StepAside
	waitmovement 0
	setflag FLAG_PIT_PASSWORD_GIVEN
	call Pit_EventScript_DoormanAside
	release
	end

Pit_EventScript_DoormanWrong::
	msgbox Pit_Text_DoormanWrong
	release
	end

Pit_EventScript_DoormanLetIn::
	msgbox Pit_Text_DoormanLetIn
	release
	end

Pit_Movement_StepAside:
	walk_right
	face_left
	step_end

@ ---- The pit ---------------------------------------------------------------------
Pit_EventScript_PitTransition::
	special Pit_OnTransition
	end

@ The card on the rim: tonight's bout
Pit_EventScript_Card::
	lockall
	msgbox Pit_Text_CardHeader
	setvar VAR_0x8004, 0
	call Pit_EventScript_CardSide
	setvar VAR_0x8004, 2
	call Pit_EventScript_CardSide
	setvar VAR_0x8004, 4
	special Pit_BufferCardPage
	msgbox gStringVar4
	releaseall
	end

@ VAR_0x8004 = the side's first page
Pit_EventScript_CardSide::
	special Pit_BufferCardPage
	showmonpic VAR_0x8005, 10, 3
	msgbox gStringVar4
	addvar VAR_0x8004, 1
	special Pit_BufferCardPage
	msgbox gStringVar4
	hidemonpic
	return

@ The bookie: bets, and the bell
Pit_EventScript_Bookie::
	lock
	faceplayer
	goto_if_set FLAG_PIT_BOOKIE_MET, Pit_EventScript_BookieCard
	msgbox Pit_Text_BookieFirst
	setflag FLAG_PIT_BOOKIE_MET
Pit_EventScript_BookieCard::
	special Pit_BufferCard
	special Pit_HasBet
	goto_if_eq VAR_RESULT, TRUE, Pit_EventScript_BookieAlreadyBet
	msgbox Pit_Text_BookieCard
	dynmultichoice 0, 0, FALSE, 3, 0, DYN_MULTICHOICE_CB_NONE, Pit_Text_ChoiceBet, Pit_Text_ChoiceWatch, Pit_Text_ChoiceLater
	switch VAR_RESULT
	case 0, Pit_EventScript_BookieBet
	case 1, Pit_EventScript_BookieWatch
	msgbox Pit_Text_BookieLater
	release
	end

Pit_EventScript_BookieBet::
	message Pit_Text_BookieWhichCorner
	waitmessage
	special Pit_PushCorners
	dynmultistack 0, 0, FALSE, 2, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, Pit_EventScript_BookieLaterRelease
	copyvar VAR_0x8004, VAR_RESULT
	special Pit_PushBets
	goto_if_eq VAR_RESULT, 0, Pit_EventScript_BookieBroke
	message Pit_Text_BookieHowMuch
	waitmessage
	showmoneybox 18, 0
	dynmultistack 0, 0, FALSE, 5, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, Pit_EventScript_BookieBetCancel
	copyvar VAR_0x8005, VAR_RESULT
	special Pit_PlaceBet
	updatemoneybox
	call_if_eq VAR_RESULT, TRUE, Pit_EventScript_TipsterCut
	special Pit_BufferBet
	playse SE_SHOP
	msgbox Pit_Text_BookieBetTaken
	hidemoneybox
	goto Pit_EventScript_Fight

Pit_EventScript_TipsterCut::
	msgbox Pit_Text_BookieTipsterCut
	return

Pit_EventScript_BookieBetCancel::
	hidemoneybox
Pit_EventScript_BookieLaterRelease::
	msgbox Pit_Text_BookieLater
	release
	end

Pit_EventScript_BookieBroke::
	msgbox Pit_Text_BookieBroke
	release
	end

Pit_EventScript_BookieWatch::
	msgbox Pit_Text_BookieWatch
	goto Pit_EventScript_Fight

Pit_EventScript_BookieAlreadyBet::
	msgbox Pit_Text_BookieAlreadyBet
	goto Pit_EventScript_Fight

@ The bout: the bell, the battle (the two fighters' AI), the payout, the next card
Pit_EventScript_Fight::
	special Pit_HasBet
	call_if_eq VAR_RESULT, TRUE, Pit_EventScript_AutoSave
	msgbox Pit_Text_BookieBell
	closemessage
	playse SE_APPLAUSE
	applymovement LOCALID_PIT_RED_TRAINER, Pit_Movement_FaceRight
	applymovement LOCALID_PIT_BLUE_TRAINER, Pit_Movement_FaceLeft
	applymovement LOCALID_PIT_RED_MON, Pit_Movement_SquareUpRight
	applymovement LOCALID_PIT_BLUE_MON, Pit_Movement_SquareUpLeft
	applymovement LOCALID_PIT_CROWD_0, Common_Movement_ExclamationMark
	applymovement LOCALID_PIT_CROWD_3, Common_Movement_ExclamationMark
	applymovement LOCALID_PIT_CROWD_5, Common_Movement_ExclamationMark
	waitmovement 0
	waitse
	special Pit_SetupFight
	setflag FLAG_GYM_CHALLENGE_BATTLE
	cleartrainerflag TRAINER_GYM_CHALLENGER
	trainerbattle_no_intro TRAINER_GYM_CHALLENGER, Pit_Text_BlueLoses
	clearflag FLAG_GYM_CHALLENGE_BATTLE
	special GymChallenge_EndPractice
	special Pit_EndFight
	copyvar VAR_0x8006, VAR_RESULT
	copyvar VAR_0x8007, VAR_0x8004
	call_if_eq VAR_0x8006, 0, Pit_EventScript_BlueMonDown
	call_if_eq VAR_0x8006, 1, Pit_EventScript_RedMonDown
	playse SE_APPLAUSE
	msgbox Pit_Text_BookieResult
	waitse
	call_if_eq VAR_0x8007, 1, Pit_EventScript_BetWon
	call_if_eq VAR_0x8007, 2, Pit_EventScript_BetLost
	call_if_ne VAR_0x8007, 0, Pit_EventScript_AutoSave
	msgbox Pit_Text_BookieNextBout
	closemessage
	fadescreen FADE_TO_BLACK
	special Pit_NextCard
	removeobject LOCALID_PIT_RED_MON
	removeobject LOCALID_PIT_BLUE_MON
	removeobject LOCALID_PIT_RED_TRAINER
	removeobject LOCALID_PIT_BLUE_TRAINER
	addobject LOCALID_PIT_RED_MON
	addobject LOCALID_PIT_BLUE_MON
	addobject LOCALID_PIT_RED_TRAINER
	addobject LOCALID_PIT_BLUE_TRAINER
	fadescreen FADE_FROM_BLACK
	release
	end

Pit_EventScript_RedMonDown::
	applymovement LOCALID_PIT_BLUE_MON, Pit_Movement_Victory
	waitmovement 0
	removeobject LOCALID_PIT_RED_MON
	return

Pit_EventScript_BlueMonDown::
	applymovement LOCALID_PIT_RED_MON, Pit_Movement_Victory
	waitmovement 0
	removeobject LOCALID_PIT_BLUE_MON
	return

Pit_EventScript_BetWon::
	playfanfare MUS_LEVEL_UP
	msgbox Pit_Text_BetWon
	waitfanfare
	return

Pit_EventScript_BetLost::
	msgbox Pit_Text_BetLost
	return

Pit_Movement_FaceRight:
	face_right
	step_end

Pit_Movement_FaceLeft:
	face_left
	step_end

Pit_Movement_SquareUpRight:
	jump_in_place_right
	jump_in_place_right
	step_end

Pit_Movement_SquareUpLeft:
	jump_in_place_left
	jump_in_place_left
	step_end

Pit_Movement_Victory:
	jump_in_place_down
	jump_in_place_down
	step_end

@ The tipster: what the card doesn't say
Pit_EventScript_Tipster::
	lock
	faceplayer
	special Pit_TipBought
	goto_if_eq VAR_RESULT, TRUE, Pit_EventScript_TipsterRepeat
	msgbox Pit_Text_TipsterOffer, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Pit_EventScript_TipsterNo
Pit_EventScript_TipsterRepeat::
	special Pit_BuyTip
	switch VAR_RESULT
	case 0, Pit_EventScript_TipsterBroke
	case 1, Pit_EventScript_TipClean
	case 2, Pit_EventScript_TipDive
	case 3, Pit_EventScript_TipItem
	msgbox Pit_Text_TipLevel
	release
	end

Pit_EventScript_TipsterNo::
	msgbox Pit_Text_TipsterNo
	release
	end

Pit_EventScript_TipsterBroke::
	msgbox Pit_Text_TipsterBroke
	release
	end

Pit_EventScript_TipClean::
	msgbox Pit_Text_TipClean
	release
	end

Pit_EventScript_TipDive::
	msgbox Pit_Text_TipDive
	release
	end

Pit_EventScript_TipItem::
	msgbox Pit_Text_TipItem
	release
	end

@ The fighters, down in the pit (out of reach), and the crowd
Pit_EventScript_RedTrainer::
Pit_EventScript_BlueTrainer::
Pit_EventScript_RedMon::
Pit_EventScript_BlueMon::
	end

Pit_EventScript_Crowd0::
	msgbox Pit_Text_Crowd0, MSGBOX_NPC
	end
Pit_EventScript_Crowd1::
	msgbox Pit_Text_Crowd1, MSGBOX_NPC
	end
Pit_EventScript_Crowd2::
	msgbox Pit_Text_Crowd2, MSGBOX_NPC
	end
Pit_EventScript_Crowd3::
	msgbox Pit_Text_Crowd3, MSGBOX_NPC
	end
Pit_EventScript_Crowd4::
	msgbox Pit_Text_Crowd4, MSGBOX_NPC
	end
Pit_EventScript_Crowd5::
	msgbox Pit_Text_Crowd5, MSGBOX_NPC
	end
Pit_EventScript_Crowd6::
	msgbox Pit_Text_Crowd6, MSGBOX_NPC
	end
Pit_EventScript_Crowd7::
	msgbox Pit_Text_Crowd7, MSGBOX_NPC
	end

@ The back room (for now: shut)
@ ---- The save: before and after a battle with something on it (no taking it back) ----------
Pit_EventScript_AutoSave::
	message Pit_Text_Saving
	waitmessage
	special Pit_AutoSave
	playse SE_SAVE
	waitse
	msgbox Pit_Text_Saved
	return

@ ---- The back room: Pokémon for Pokémon ------------------------------------------------
Pit_EventScript_BackRoomTransition::
	special PitBoss_OnTransition
	end

Pit_EventScript_BackRoomMon::
	lock
	special PitBoss_BufferDay
	msgbox PitBoss_Text_TheMon
	release
	end

Pit_EventScript_BackRoomKeeper::
	lock
	faceplayer
	special PitBoss_CheckForfeit
	goto_if_eq VAR_RESULT, TRUE, PitBoss_EventScript_Forfeited
	goto_if_set FLAG_PIT_BOSS_MET, PitBoss_EventScript_Main
	msgbox PitBoss_Text_Rules
	setflag FLAG_PIT_BOSS_MET
	goto PitBoss_EventScript_Main

PitBoss_EventScript_Forfeited::
	setflag FLAG_PIT_BOSS_MET
	msgbox PitBoss_Text_Forfeited
	goto PitBoss_EventScript_TellBuyBack

PitBoss_EventScript_Main::
	special PitBoss_HasHeld
	call_if_eq VAR_RESULT, TRUE, PitBoss_EventScript_OfferBuyBack
	goto_if_set FLAG_PIT_BOSS_BEATEN, PitBoss_EventScript_CleanedOut
	special PitBoss_BufferDay
	msgbox PitBoss_Text_Today, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PitBoss_EventScript_NoDeal
	msgbox PitBoss_Text_ShowMe
	special ChoosePartyMon
	special PitBoss_CheckStake
	switch VAR_RESULT
	case 1, PitBoss_EventScript_NoDeal
	case 2, PitBoss_EventScript_Egg
	case 3, PitBoss_EventScript_Fainted
	case 4, PitBoss_EventScript_OnlyOne
	case 5, PitBoss_EventScript_TooStrong
	case 6, PitBoss_EventScript_TooWeak
	case 7, PitBoss_EventScript_Underdog
PitBoss_EventScript_Confirm::
	msgbox PitBoss_Text_Confirm, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PitBoss_EventScript_NoDeal
	msgbox PitBoss_Text_Official
	special PitBoss_HoldStake
	call Pit_EventScript_AutoSave
	closemessage
	special PitBoss_SetupFight
	setflag FLAG_GYM_CHALLENGE_BATTLE
	cleartrainerflag TRAINER_GYM_CHALLENGER
	trainerbattle_no_intro TRAINER_GYM_CHALLENGER, PitBoss_Text_Defeat
	clearflag FLAG_GYM_CHALLENGE_BATTLE
	special GymChallenge_EndPractice
	special PitBoss_EndFight
	switch VAR_RESULT
	case 0, PitBoss_EventScript_Won
	case 2, PitBoss_EventScript_Draw
	msgbox PitBoss_Text_Lost
	call Pit_EventScript_AutoSave
	goto PitBoss_EventScript_TellBuyBack

PitBoss_EventScript_Won::
	msgbox PitBoss_Text_Won
	playfanfare MUS_OBTAIN_ITEM
	message PitBoss_Text_Received
	waitmessage
	waitfanfare
	call_if_eq VAR_0x8005, TRUE, PitBoss_EventScript_ToPC
	removeobject LOCALID_PIT_BOSS_MON
	call Pit_EventScript_AutoSave
	release
	end

PitBoss_EventScript_ToPC::
	msgbox PitBoss_Text_ToPC
	return

PitBoss_EventScript_Draw::
	msgbox PitBoss_Text_Draw
	call Pit_EventScript_AutoSave
	release
	end

@ (after a loss: the buyback, first mentioned)
PitBoss_EventScript_TellBuyBack::
	msgbox PitBoss_Text_TellBuyBack
	setflag FLAG_PIT_BOSS_BUYBACK_TOLD
	release
	end

PitBoss_EventScript_OfferBuyBack::
	showmoneybox 18, 0
	msgbox PitBoss_Text_OfferBuyBack, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PitBoss_EventScript_KeepIt
	special PitBoss_BuyBack
	switch VAR_RESULT
	case 0, PitBoss_EventScript_BuyBackBroke
	case 3, PitBoss_EventScript_BuyBackNoRoom
	updatemoneybox
	playse SE_SHOP
	msgbox PitBoss_Text_BoughtBack
	call_if_eq VAR_RESULT, 2, PitBoss_EventScript_BoughtBackToPC
	hidemoneybox
	call Pit_EventScript_AutoSave
	return

PitBoss_EventScript_BoughtBackToPC::
	msgbox PitBoss_Text_BoughtBackToPC
	return

PitBoss_EventScript_KeepIt::
	msgbox PitBoss_Text_KeepIt
	hidemoneybox
	return

PitBoss_EventScript_BuyBackBroke::
	msgbox PitBoss_Text_BuyBackBroke
	hidemoneybox
	return

PitBoss_EventScript_BuyBackNoRoom::
	msgbox PitBoss_Text_BuyBackNoRoom
	hidemoneybox
	return

PitBoss_EventScript_CleanedOut::
	msgbox PitBoss_Text_CleanedOut
	release
	end

PitBoss_EventScript_NoDeal::
	msgbox PitBoss_Text_NoDeal
	release
	end

PitBoss_EventScript_Egg::
	msgbox PitBoss_Text_Egg
	release
	end

PitBoss_EventScript_Fainted::
	msgbox PitBoss_Text_Fainted
	release
	end

PitBoss_EventScript_OnlyOne::
	msgbox PitBoss_Text_OnlyOne
	release
	end

PitBoss_EventScript_TooStrong::
	msgbox PitBoss_Text_TooStrong
	release
	end

@ (outclassed, but not hopelessly: his call's the player's to make - twice)
PitBoss_EventScript_Underdog::
	msgbox PitBoss_Text_Underdog, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PitBoss_EventScript_NoDeal
	msgbox PitBoss_Text_UnderdogSure, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PitBoss_EventScript_NoDeal
	goto PitBoss_EventScript_Confirm

PitBoss_EventScript_TooWeak::
	msgbox PitBoss_Text_TooWeak
	release
	end

@ ---- The Game Corner: the password, overheard ---------------------------------------
Pit_EventScript_GameCornerWhisperer::
	lock
	msgbox Pit_Text_Overheard
	faceplayer
	msgbox Pit_Text_WhispererCaught
	release
	end

Pit_EventScript_GameCornerListener::
	msgbox Pit_Text_ListenerBeatIt, MSGBOX_NPC
	end

@ ---- Text ----------------------------------------------------------------------------
Pit_Text_HiddenDoorFound::
	.string "There's a seam in the siding…\p"
	.string "A panel swings in under {PLAYER}'s\n"
	.string "weight! Stairs lead down into the\l"
	.string "dark…$"

Pit_Text_DoormanStop::
	.string "Whoa, whoa. Where do you think\n"
	.string "you're going, kid?\p"
	.string "Nobody goes down there without\n"
	.string "the word. So what's the word?$"

Pit_Text_DoormanWrong::
	.string "…Wrong.\p"
	.string "Climb back up that ladder before\n"
	.string "I remember your face.$"

Pit_Text_DoormanRight::
	.string "…Huh. Somebody's got a big mouth.\p"
	.string "Fine. Down you go. You didn't see\n"
	.string "me, I didn't see you.\p"
	.string "And whatever you hear down there,\n"
	.string "it's just the crowd. Got it?$"

Pit_Text_DoormanLetIn::
	.string "Go on down. Keep your voice low\n"
	.string "and your money where I can see it.$"

Pit_Text_CardHeader::
	.string "TONIGHT'S CARD\n"
	.string "One on one. No forfeits. No refunds.$"

Pit_Text_BookieFirst::
	.string "Well, well. Fresh meat.\p"
	.string "You know why this place is down a\n"
	.string "hole, right?\p"
	.string "The League's got rules about how a\n"
	.string "trainer treats their POKéMON.\l"
	.string "Rest. Healing. Stop when it's hurt.\p"
	.string "Down here, we're a little more…\n"
	.string "relaxed about all that.\p"
	.string "Hey, don't look at me like that.\n"
	.string "We treat 'em fine. I swear.\p"
	.string "Anyway. Fighters on the card, odds\n"
	.string "on the board. Pick a corner, lay\l"
	.string "your money down. Winner pays out.\p"
	.string "And kid? The house always takes\n"
	.string "its cut.$"

Pit_Text_BookieCard::
	.string "{STR_VAR_1}'s {STR_VAR_2} against\n"
	.string "{STR_VAR_3}. Card's on the board.\l"
	.string "You want a piece?$"

Pit_Text_ChoiceBet::
	.string "PLACE A BET$"

Pit_Text_ChoiceWatch::
	.string "JUST WATCH$"

Pit_Text_ChoiceLater::
	.string "NOT YET$"

Pit_Text_BookieWhichCorner::
	.string "Which corner?$"

Pit_Text_BookieHowMuch::
	.string "How much?$"

Pit_Text_BookieBetTaken::
	.string "¥{STR_VAR_2} on {STR_VAR_1}.\n"
	.string "If it wins, you walk with ¥{STR_VAR_3}.\p"
	.string "Bets are closed!$"

Pit_Text_BookieLater::
	.string "Don't take too long. The crowd gets\n"
	.string "ugly when it's bored.$"

Pit_Text_BookieBroke::
	.string "No cash, no action. Come back when\n"
	.string "your pockets aren't empty.$"

Pit_Text_BookieWatch::
	.string "A free show, huh? Don't make a habit\n"
	.string "of it.$"

Pit_Text_BookieAlreadyBet::
	.string "Your money's down. Let's ring the\n"
	.string "bell.$"

Pit_Text_BookieBell::
	.string "Fighters! Into the pit!\p"
	.string "Bets are CLOSED!\n"
	.string "LET 'EM LOOSE!$"

Pit_Text_BlueLoses::
	.string "No! Get up, you mutt!$"

Pit_Text_BookieResult::
	.string "It's over! {STR_VAR_3}'s {STR_VAR_1}\n"
	.string "takes it!$"

Pit_Text_BetWon::
	.string "Read that one right, kid.\n"
	.string "Here's your ¥{STR_VAR_2}.$"

Pit_Text_BetLost::
	.string "Tough break. Your ¥{STR_VAR_2}'s\n"
	.string "the house's now.$"

Pit_Text_BookieNextBout::
	.string "Clear the pit! Next bout's going\n"
	.string "up on the board!$"

Pit_Text_TipsterOffer::
	.string "Psst. Over here.\p"
	.string "That card's what they WANT you to\n"
	.string "see. I know what's really going on.\p"
	.string "My price? Ten percent of whatever\n"
	.string "you put down on this one.\l"
	.string "The bookie collects it for me.$"

Pit_Text_TipsterNo::
	.string "Your funeral. Fights down here\n"
	.string "aren't always what they look like.$"

Pit_Text_TipsterBroke::
	.string "Your money's already down, and you\n"
	.string "can't cover my ten percent.\p"
	.string "No dough, no info.$"

Pit_Text_BookieTipsterCut::
	.string "Plus ¥{STR_VAR_2} for our friend in\n"
	.string "the corner. He sends his regards.$"

Pit_Text_TipClean::
	.string "This one's clean. Far as I can\n"
	.string "tell, they're both trying.\p"
	.string "Bet the card, kid.$"

Pit_Text_TipDive::
	.string "Word is {STR_VAR_2}'s been paid\n"
	.string "to lose this one.\p"
	.string "Watch. {STR_VAR_1}'s gonna be\n"
	.string "calling all the wrong moves.$"

Pit_Text_TipItem::
	.string "Somebody got to {STR_VAR_2}'s gear.\p"
	.string "That {STR_VAR_1}'s not holding what\n"
	.string "the card says. It's lugging around\l"
	.string "a {STR_VAR_3}.$"

Pit_Text_TipLevel::
	.string "{STR_VAR_2} hasn't fed that {STR_VAR_1}\n"
	.string "in a week.\p"
	.string "It's nowhere near the level on the\n"
	.string "card. Somebody wants it to lose.$"

Pit_Text_Crowd0::
	.string "Come on, come ON! I've got rent\n"
	.string "money riding on this!$"

Pit_Text_Crowd1::
	.string "Twenty years I've been coming down\n"
	.string "here. Still can't pick a winner.$"

Pit_Text_Crowd2::
	.string "Where do the losers go after?\p"
	.string "…Kid, down here you learn not to\n"
	.string "ask that.$"

Pit_Text_Crowd3::
	.string "The bookie's odds are always a\n"
	.string "little off. Learn your types and\l"
	.string "you'll find the soft spots.$"

Pit_Text_Crowd4::
	.string "Half these fights are bought and\n"
	.string "paid for. The trick's knowing\l"
	.string "which half.$"

Pit_Text_Crowd5::
	.string "Rip 'em up! I want to see some\n"
	.string "fur fly!$"

Pit_Text_Crowd6::
	.string "The League says these fighters\n"
	.string "mistreat their POKéMON.\p"
	.string "Look at 'em. They're fine.\n"
	.string "…Mostly.$"

Pit_Text_Crowd7::
	.string "Lost my boat on a sure thing last\n"
	.string "week. Now I'm just here to watch\l"
	.string "other people lose theirs.$"

Pit_Text_Saving::
	.string "SAVING…\n"
	.string "DON'T TURN OFF THE POWER.$"

Pit_Text_Saved::
	.string "{PLAYER} saved the game.$"

PitBoss_Text_TheMon::
	.string "The boss's {STR_VAR_1}. It's watching\n"
	.string "you very closely.$"

PitBoss_Text_Rules::
	.string "Money's for the suckers out front.\n"
	.string "Back here, we bet POKéMON.\p"
	.string "You put one up. I put one up. One\n"
	.string "on one, same level.\p"
	.string "Winner walks out with both.\p"
	.string "Lose, and yours stays here. With\n"
	.string "me. For good.$"

PitBoss_Text_Today::
	.string "Today I'm putting up a {STR_VAR_1}.\n"
	.string "Want a shot at it?$"

PitBoss_Text_ShowMe::
	.string "Show me what you're putting up.$"

PitBoss_Text_Confirm::
	.string "Your {STR_VAR_2} against my\n"
	.string "{STR_VAR_1}. Same level. One on one.\p"
	.string "Lose, and your {STR_VAR_2} is mine.\n"
	.string "Permanently. No take-backs.\p"
	.string "Deal?$"

PitBoss_Text_Official::
	.string "Then we make it official.$"

PitBoss_Text_Defeat::
	.string "…Huh.$"

PitBoss_Text_Won::
	.string "…Well. A deal's a deal.\n"
	.string "Take it. Free and clear.$"

PitBoss_Text_Received::
	.string "{PLAYER} received the {STR_VAR_1}!$"

PitBoss_Text_ToPC::
	.string "The {STR_VAR_1} was sent to the PC.$"

PitBoss_Text_Draw::
	.string "Both down? Then nobody wins.\n"
	.string "Take yours and get out.$"

PitBoss_Text_Lost::
	.string "Your {STR_VAR_2} is mine now.$"

PitBoss_Text_Forfeited::
	.string "The lights went out on you, huh?\n"
	.string "Funny how that happens.\p"
	.string "Rules are rules. You walked out on\n"
	.string "a deal. Your {STR_VAR_2} stayed\l"
	.string "down here.$"

PitBoss_Text_TellBuyBack::
	.string "…Don't look at me like that.\n"
	.string "I'm a businessman.\p"
	.string "¥500000, cash, and you can have it\n"
	.string "back.\p"
	.string "Offer stands till I win another one\n"
	.string "off you.$"

PitBoss_Text_OfferBuyBack::
	.string "Here for your {STR_VAR_2}?\n"
	.string "¥500000. Cash.$"

PitBoss_Text_BoughtBack::
	.string "Pleasure doing business.\n"
	.string "{STR_VAR_2}'s yours again.$"

PitBoss_Text_BoughtBackToPC::
	.string "{STR_VAR_2} was sent to the PC.$"

PitBoss_Text_KeepIt::
	.string "Suit yourself. It's not going\n"
	.string "anywhere. Yet.$"

PitBoss_Text_BuyBackBroke::
	.string "That's not ¥500000. No cash, no\n"
	.string "deal.$"

PitBoss_Text_BuyBackNoRoom::
	.string "You've got nowhere to put it.\n"
	.string "Make some room first.$"

PitBoss_Text_CleanedOut::
	.string "You cleaned me out today.\n"
	.string "Come back tomorrow.$"

PitBoss_Text_NoDeal::
	.string "Suit yourself.$"

PitBoss_Text_Egg::
	.string "An EGG? Funny. Real funny.$"

PitBoss_Text_Fainted::
	.string "That one can barely stand.\n"
	.string "Patch it up first.$"

PitBoss_Text_OnlyOne::
	.string "That's all you've got? Lose, and\n"
	.string "you walk out with nothing.\p"
	.string "Come back with more than one\n"
	.string "POKéMON.$"

PitBoss_Text_TooStrong::
	.string "Your {STR_VAR_2}? Against my\n"
	.string "{STR_VAR_1}? That's a mugging.\p"
	.string "I only take fair fights. Bring me\n"
	.string "something closer to even.$"

PitBoss_Text_Underdog::
	.string "Your {STR_VAR_2}? Against my\n"
	.string "{STR_VAR_1}? It's outclassed, kid.\p"
	.string "You'll probably lose it.\p"
	.string "But if you want to throw it away,\n"
	.string "I'll take it. Still want to go?$"

PitBoss_Text_UnderdogSure::
	.string "You're sure? Totally sure?\n"
	.string "No crying when it's gone.$"

PitBoss_Text_TooWeak::
	.string "That's what you're putting up?\n"
	.string "My {STR_VAR_1} would tear it apart.\p"
	.string "I only take fair fights. Bring me\n"
	.string "something with some bite.$"

Pit_Text_Overheard::
	.string "(The man is whispering to his\n"
	.string "friend…)\p"
	.string "…changed it again. Word this week\n"
	.string "is GRAVEL.\p"
	.string "Same place. The wall past the prize\n"
	.string "shop, the building with no door.\l"
	.string "Just walk right into it.$"

Pit_Text_WhispererCaught::
	.string "Huh? Were you listening?\p"
	.string "…I was talking about my garden.\n"
	.string "Gravel. For the path. Get lost.$"

Pit_Text_ListenerBeatIt::
	.string "Beat it, kid. Grown-up\n"
	.string "conversation.$"
