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
	showmoneybox 0, 0
	dynmultistack 0, 0, FALSE, 5, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, Pit_EventScript_BookieBetCancel
	copyvar VAR_0x8005, VAR_RESULT
	special Pit_PlaceBet
	updatemoneybox
	special Pit_BufferBet
	playse SE_SHOP
	msgbox Pit_Text_BookieBetTaken
	hidemoneybox
	goto Pit_EventScript_Fight

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
Pit_EventScript_BackRoomKeeper::
	msgbox Pit_Text_BackRoomKeeper, MSGBOX_NPC
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
	.string "And if a cop ever asks, you were\n"
	.string "at the movies.$"

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
	.string "Upstairs, two trainers can put up\n"
	.string "prize money on their own battle.\l"
	.string "League purse. All nice and legal.\p"
	.string "But a crowd putting money on\n"
	.string "somebody ELSE'S fight?\p"
	.string "League shut that down years ago.\n"
	.string "Said it got fights thrown.\p"
	.string "Heh. They weren't wrong.\p"
	.string "Anyway. Fighter on the card, odds\n"
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
	.string "¥3000 and it's yours.$"

Pit_Text_TipsterNo::
	.string "Your funeral. Fights down here\n"
	.string "aren't always what they look like.$"

Pit_Text_TipsterBroke::
	.string "No dough, no info. That's how it\n"
	.string "works.$"

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
	.string "You didn't see me here. My wife\n"
	.string "thinks I'm at the Game Corner.$"

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
	.string "The League calls this “unsanctioned\n"
	.string "wagering.” I call it Tuesday.$"

Pit_Text_Crowd7::
	.string "Lost my boat on a sure thing last\n"
	.string "week. Now I'm just here to watch\l"
	.string "other people lose theirs.$"

Pit_Text_BackRoomKeeper::
	.string "The back room's for the real\n"
	.string "money. Big stakes. Bigger losers.\p"
	.string "The boss isn't taking games\n"
	.string "tonight. Beat it.$"

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
