@ Pokémon Canvas: the Gauntlet (src/gauntlet.c). A hooded figure by the gym's door calls in the
@ strongest trainers of the Pokémon world, one at a time. Losing changes nothing: the same
@ challenger comes next time. After them all: Bill (the figure all along), and then his Palm Pilot -
@ a WITHDRAW of any Pokémon, shiny, and his battle simulator to run it all again for another.

@ The first time the player comes into the gym after clearing the convention and mastering every type
Gauntlet_EventScript_Arrives::
	lockall
	setvar VAR_GAUNTLET_STATE, GAUNTLET_ACTIVE
	clearflag FLAG_HIDE_GAUNTLET_FIGURE
	addobject LOCALID_PALLET_GYM_FIGURE
	playse SE_PIN
	applymovement LOCALID_PALLET_GYM_FIGURE, Common_Movement_ExclamationMark
	waitmovement 0
	textcolor NPC_TEXT_COLOR_NEUTRAL
	msgbox Gauntlet_Text_Arrival1
	playbgm MUS_RG_ENCOUNTER_ROCKET, FALSE
	msgbox Gauntlet_Text_Arrival2
	msgbox Gauntlet_Text_Arrival3
	fadedefaultbgm
	releaseall
	end

@ Talking to the figure
Gauntlet_EventScript_Figure::
	lock
	textcolor NPC_TEXT_COLOR_NEUTRAL
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Gauntlet_EventScript_PalmPilot
	faceplayer
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_ALL_BEATEN, Gauntlet_EventScript_AllBeaten
Gauntlet_EventScript_FigureMenu::
	message Gauntlet_Text_FigureAsk
	waitmessage
	dynmultichoice 0, 0, FALSE, 3, 0, DYN_MULTICHOICE_CB_NONE, Gauntlet_Text_OptNext, Gauntlet_Text_OptClipboard, Gauntlet_Text_OptNotNow
	switch VAR_RESULT
	case 0, Gauntlet_EventScript_Begin
	case 1, Gauntlet_EventScript_Clipboard
	msgbox Gauntlet_Text_NotNow
	release
	end

@ ---- The clipboard ----
Gauntlet_EventScript_Clipboard::
	special Gauntlet_BufferProgress
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Gauntlet_EventScript_Records
	msgbox Gauntlet_Text_Clipboard
Gauntlet_EventScript_ClipboardRegions::
	message Gauntlet_Text_WhichPage
	waitmessage
	special Gauntlet_PushRegions
	dynmultistack 0, 0, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, Gauntlet_EventScript_ClipboardDone
	copyvar VAR_0x8004, VAR_RESULT
	message Gauntlet_Text_Page
	waitmessage
	special Gauntlet_PushRegionNames
	dynmultistack 0, 0, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto Gauntlet_EventScript_ClipboardRegions

Gauntlet_EventScript_Records::
	msgbox Gauntlet_Text_Records
	goto Gauntlet_EventScript_ClipboardRegions

Gauntlet_EventScript_ClipboardDone::
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Gauntlet_EventScript_PilotMenu
	goto Gauntlet_EventScript_FigureMenu

@ ---- The battles ----
Gauntlet_EventScript_Begin::
	special HealPlayerParty
	special GymChallenge_PlanPlayerWalk
	goto_if_eq VAR_RESULT, 0, GymChallenge_EventScript_CantReachSpot
	call_if_ne VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Gauntlet_EventScript_TakeYourPlace
	closemessage
	applymovement LOCALID_PLAYER, gGymChallengePlayerPath
	waitmovement 0
Gauntlet_EventScript_Next::
	special Gauntlet_SetupNext
	call_if_ne VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Gauntlet_EventScript_Announce
	call_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Gauntlet_EventScript_SimAnnounce
	closemessage
	clearflag FLAG_HIDE_PALLET_GYM_CHALLENGER
	addobject LOCALID_PALLET_GYM_CHALLENGER
	setvar VAR_0x8004, 0
	special GymChallenge_PlanChallengerWalk
	call_if_eq VAR_RESULT, FALSE, GymChallenge_EventScript_ChallengerJumpsIn
	applymovement LOCALID_PALLET_GYM_CHALLENGER, gGymChallengeChallengerPath
	waitmovement 0
	msgbox gGymChallengeIntroText
	closemessage
	fadescreen FADE_TO_BLACK
	special Gauntlet_ShowTitleCard
	waitstate
	cleartrainerflag TRAINER_GYM_CHALLENGER
	setflag FLAG_GYM_CHALLENGE_BATTLE
	trainerbattle_no_intro TRAINER_GYM_CHALLENGER, gGymChallengeDefeatText
	clearflag FLAG_GYM_CHALLENGE_BATTLE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, Gauntlet_EventScript_Won
	special HealPlayerParty
	msgbox gGymChallengeWinText
	call GymChallenge_EventScript_ChallengerLeaves
	textcolor NPC_TEXT_COLOR_NEUTRAL
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Gauntlet_EventScript_SimLost
	msgbox Gauntlet_Text_Lost
	release
	end

Gauntlet_EventScript_TakeYourPlace::
	msgbox Gauntlet_Text_TakeYourPlace
	return

Gauntlet_EventScript_Announce::
	msgbox Gauntlet_Text_Announce
	return

Gauntlet_EventScript_SimAnnounce::
	msgbox Gauntlet_Text_SimAnnounce
	return

Gauntlet_EventScript_Won::
	special HealPlayerParty
	msgbox gGymChallengeAfterText
	call GymChallenge_EventScript_ChallengerLeaves
	special Gauntlet_RecordWin
	textcolor NPC_TEXT_COLOR_NEUTRAL
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Gauntlet_EventScript_SimWon
	goto_if_eq VAR_RESULT, TRUE, Gauntlet_EventScript_LastOne
	msgbox Gauntlet_Text_Won
	msgbox Gauntlet_Text_ContinueOrBreak, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Gauntlet_EventScript_Break
Gauntlet_EventScript_Continue::
	closemessage
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	goto Gauntlet_EventScript_Next

Gauntlet_EventScript_Break::
	msgbox Gauntlet_Text_Break
	release
	end

Gauntlet_EventScript_LastOne::
	playse SE_PIN
	msgbox Gauntlet_Text_LastOne
	release
	end

@ ---- The one who hired the figure ----
Gauntlet_EventScript_AllBeaten::
	goto_if_set FLAG_GAUNTLET_BILL_REVEALED, Gauntlet_EventScript_BillAgain
	msgbox Gauntlet_Text_ReadyToMeet, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Gauntlet_EventScript_NotYet
	msgbox Gauntlet_Text_Reveal1
	closemessage
	fadeoutbgm 1
	delay 30
	fadescreenspeed FADE_TO_WHITE, 2
	special Gauntlet_RevealBill
	removeobject LOCALID_PALLET_GYM_FIGURE
	addobject LOCALID_PALLET_GYM_FIGURE
	playse SE_M_CONFUSE_RAY
	fadescreenspeed FADE_FROM_WHITE, 2
	waitse
	playbgm MUS_RG_POKE_MANSION, FALSE
	applymovement LOCALID_PALLET_GYM_FIGURE, Common_Movement_FacePlayer
	waitmovement 0
	msgbox Gauntlet_Text_Reveal2
	msgbox Gauntlet_Text_Reveal3
	msgbox Gauntlet_Text_Reveal4
	goto Gauntlet_EventScript_BillBattle

Gauntlet_EventScript_NotYet::
	msgbox Gauntlet_Text_NotYet
	release
	end

Gauntlet_EventScript_BillAgain::
	playbgm MUS_RG_POKE_MANSION, FALSE
	msgbox Gauntlet_Text_BillAgain, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Gauntlet_EventScript_BillWait
Gauntlet_EventScript_BillBattle::
	special HealPlayerParty
	special Gauntlet_SetupNext
	msgbox gGymChallengeIntroText
	closemessage
	fadescreen FADE_TO_BLACK
	special Gauntlet_ShowTitleCard
	waitstate
	cleartrainerflag TRAINER_GYM_CHALLENGER
	setflag FLAG_GYM_CHALLENGE_BATTLE
	trainerbattle_no_intro TRAINER_GYM_CHALLENGER, gGymChallengeDefeatText
	clearflag FLAG_GYM_CHALLENGE_BATTLE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, Gauntlet_EventScript_BillWon
	special HealPlayerParty
	msgbox gGymChallengeWinText
	msgbox Gauntlet_Text_BillLost
	release
	end

Gauntlet_EventScript_BillWait::
	msgbox Gauntlet_Text_BillWait
	fadedefaultbgm
	release
	end

Gauntlet_EventScript_BillWon::
	special HealPlayerParty
	playbgm MUS_RG_POKE_MANSION, FALSE
	msgbox gGymChallengeAfterText
	msgbox Gauntlet_Text_BillLast
	closemessage
	applymovement LOCALID_PALLET_GYM_FIGURE, Gauntlet_Movement_Collapse
	waitmovement 0
	playmoncry SPECIES_CLEFABLE, CRY_MODE_FAINT
	waitmoncry
	fadeoutbgm 1
	fadescreen FADE_TO_BLACK
	special Gauntlet_BillDefeated
	removeobject LOCALID_PALLET_GYM_FIGURE
	addobject LOCALID_PALLET_GYM_FIGURE
	delay 60
	fadescreen FADE_FROM_BLACK
	msgbox Gauntlet_Text_Faded
	fadedefaultbgm
	release
	end

Gauntlet_Movement_Collapse:
	walk_in_place_slow_down
	delay_16
	walk_in_place_slow_down
	delay_16
	face_up
	delay_8
	face_down
	delay_16
	step_end

@ ---- Bill's Palm Pilot ----
Gauntlet_EventScript_PalmPilot::
	msgbox Gauntlet_Text_Pilot
	goto_if_eq VAR_GAUNTLET_WITHDRAWS, 0, Gauntlet_EventScript_PilotMenu
	msgbox Gauntlet_Text_WithdrawAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, Gauntlet_EventScript_Withdraw
Gauntlet_EventScript_PilotMenu::
	message Gauntlet_Text_PilotMenu
	waitmessage
	goto_if_ne VAR_GAUNTLET_WITHDRAWS, 0, Gauntlet_EventScript_PilotMenuWithdraw
	dynmultichoice 0, 0, FALSE, 3, 0, DYN_MULTICHOICE_CB_NONE, Gauntlet_Text_OptSim, Gauntlet_Text_OptRecords, Gauntlet_Text_OptOff
	switch VAR_RESULT
	case 0, Gauntlet_EventScript_SimBegin
	case 1, Gauntlet_EventScript_Clipboard
	goto Gauntlet_EventScript_PilotOff

Gauntlet_EventScript_PilotMenuWithdraw::
	dynmultichoice 0, 0, FALSE, 4, 0, DYN_MULTICHOICE_CB_NONE, Gauntlet_Text_OptWithdraw, Gauntlet_Text_OptSim, Gauntlet_Text_OptRecords, Gauntlet_Text_OptOff
	switch VAR_RESULT
	case 0, Gauntlet_EventScript_Withdraw
	case 1, Gauntlet_EventScript_SimBegin
	case 2, Gauntlet_EventScript_Clipboard
Gauntlet_EventScript_PilotOff::
	msgbox Gauntlet_Text_PilotOff
	release
	end

Gauntlet_EventScript_Withdraw::
	message Gauntlet_Text_WithdrawWhich
	waitmessage
	special Gauntlet_ChooseWithdraw
	waitstate
	goto_if_eq VAR_RESULT, SPECIES_NONE, Gauntlet_EventScript_PilotMenu
	copyvar VAR_0x8005, VAR_RESULT
	bufferspeciesname STR_VAR_1, VAR_0x8005
	msgbox Gauntlet_Text_WithdrawConfirm, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Gauntlet_EventScript_Withdraw
	givemon VAR_0x8005, 100, shinyMode=SHINY_MODE_ALWAYS
	goto_if_eq VAR_RESULT, MON_CANT_GIVE, Gauntlet_EventScript_NoRoom
	copyvar VAR_0x8006, VAR_RESULT
	special Gauntlet_UseWithdraw
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	message Gauntlet_Text_Withdrew
	waitfanfare
	waitmessage
	call_if_eq VAR_0x8006, MON_GIVEN_TO_PC, Gauntlet_EventScript_SentToPC
	goto Gauntlet_EventScript_PilotMenu

Gauntlet_EventScript_SentToPC::
	msgbox Gauntlet_Text_SentToPC
	return

Gauntlet_EventScript_NoRoom::
	msgbox Gauntlet_Text_NoRoom
	release
	end

@ ---- Bill's battle simulator: the Gauntlet again, Bill last; each run, a WITHDRAW ----
Gauntlet_EventScript_SimBegin::
	msgbox Gauntlet_Text_SimStart
	goto Gauntlet_EventScript_Begin

Gauntlet_EventScript_SimWon::
	goto_if_eq VAR_RESULT, TRUE, Gauntlet_EventScript_SimComplete
	msgbox Gauntlet_Text_SimWon
	msgbox Gauntlet_Text_SimContinue, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, Gauntlet_EventScript_Continue
	msgbox Gauntlet_Text_SimBreak
	release
	end

Gauntlet_EventScript_SimComplete::
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	message Gauntlet_Text_SimComplete
	waitfanfare
	waitmessage
	msgbox Gauntlet_Text_WithdrawAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, Gauntlet_EventScript_Withdraw
	release
	end

Gauntlet_EventScript_SimLost::
	msgbox Gauntlet_Text_SimLost
	release
	end

@ ---- Text ----
Gauntlet_Text_Arrival1:
	.string "???: So. You're the one.\p"
	.string "Every hall of the convention\n"
	.string "cleared. Every type mastered.\p"
	.string "And not a day of real hardship\n"
	.string "in your life, I'd wager.$"

Gauntlet_Text_Arrival2:
	.string "???: Don't bother calling your aide.\n"
	.string "I'm not here to challenge your gym.\p"
	.string "I was hired. By some of the\n"
	.string "LEADERS you beat at the convention.\p"
	.string "They don't care for a child who\n"
	.string "makes their life's work look easy.\p"
	.string "So they paid me to take you down.$"

Gauntlet_Text_Arrival3:
	.string "???: I could do it myself, of course.\n"
	.string "But that wouldn't prove anything.\p"
	.string "Instead, I've called in the\n"
	.string "strongest trainers in the world.\p"
	.string "Rivals. Villains. Legends.\n"
	.string "One after another, until you fall.\p"
	.string "Lose, and the same one will be\n"
	.string "waiting next time. There's no\l"
	.string "running out of this, LEADER.\p"
	.string "Speak to me when you're ready.$"

Gauntlet_Text_FigureAsk:
	.string "???: Well, LEADER?$"

Gauntlet_Text_OptNext:
	.string "Next challenger$"

Gauntlet_Text_OptClipboard:
	.string "Read the clipboard$"

Gauntlet_Text_OptNotNow:
	.string "Not now$"

Gauntlet_Text_NotNow:
	.string "???: They'll keep.$"

Gauntlet_Text_Clipboard:
	.string "A clipboard, covered in names and\n"
	.string "crossed-out lines…\p"
	.string "CHALLENGERS DEFEATED: {STR_VAR_1} of {STR_VAR_2}$"

Gauntlet_Text_WhichPage:
	.string "Read which page?$"

Gauntlet_Text_Page:
	.string "The ones not crossed out yet are\n"
	.string "just question marks.$"

Gauntlet_Text_TakeYourPlace:
	.string "???: Take your place, LEADER.$"

Gauntlet_Text_Announce:
	.string "???: Next. From {STR_VAR_2}…\n"
	.string "{STR_VAR_1}!$"

Gauntlet_Text_Won:
	.string "???: Hmph. {STR_VAR_1} down.\n"
	.string "{STR_VAR_2} to go.$"

Gauntlet_Text_ContinueOrBreak:
	.string "???: Shall I send in the next one,\n"
	.string "or do you need a rest?$"

Gauntlet_Text_Break:
	.string "???: Rest, then. They'll wait.\n"
	.string "So will I.$"

Gauntlet_Text_Lost:
	.string "???: As I thought.\p"
	.string "Come back when you're ready. The\n"
	.string "same challenger will be waiting.$"

Gauntlet_Text_LastOne:
	.string "???: …That was the last of them.\p"
	.string "Every one. Beaten.\p"
	.string "…Good. Better than good.\p"
	.string "Then you're finally ready to meet\n"
	.string "the one who really hired me.\p"
	.string "Rest first. Speak to me when\n"
	.string "you're ready.$"

Gauntlet_Text_ReadyToMeet:
	.string "???: Are you ready to meet the one\n"
	.string "who hired me, LEADER?$"

Gauntlet_Text_NotYet:
	.string "???: …Don't take too long.$"

Gauntlet_Text_Reveal1:
	.string "???: …There's no one else coming.\p"
	.string "The LEADERS never hired me.\n"
	.string "Nobody did.\p"
	.string "I'm the one who called them all.\n"
	.string "I'm the one who needed you.$"

Gauntlet_Text_Reveal2:
	.string "It's me, {PLAYER}.\n"
	.string "BILL. The POKéMANIAC.\p"
	.string "…Don't look at me like that.\n"
	.string "I know what I look like.$"

Gauntlet_Text_Reveal3:
	.string "BILL: My CELL SEPARATION SYSTEM.\n"
	.string "You've heard of it, maybe.\p"
	.string "I wanted to know what it's like to\n"
	.string "BE a POKéMON. So I ran it backward.\p"
	.string "Me, and a CLEFABLE.\p"
	.string "It worked. And it didn't stop.\p"
	.string "It hurts. All the time.\n"
	.string "I'm not alive, not the way I was.\l"
	.string "And I can't die, either.\p"
	.string "I can't end it myself. Believe me,\n"
	.string "I've tried.$"

Gauntlet_Text_Reveal4:
	.string "BILL: So I needed someone strong.\n"
	.string "Stronger than anyone.\p"
	.string "I called in every great trainer\n"
	.string "I knew of, to be sure it was you.\p"
	.string "And it is.\p"
	.string "Please, {PLAYER}.\n"
	.string "Put me out of my misery.$"

Gauntlet_Text_BillAgain:
	.string "BILL: …You came back.\n"
	.string "Will you try again? Please?$"

Gauntlet_Text_BillWait:
	.string "BILL: …I'll be here.\n"
	.string "I'm always here.$"

Gauntlet_Text_BillLost:
	.string "BILL: …Not yet, then.\p"
	.string "Rest. Get stronger.\n"
	.string "I'll be waiting. I can't do\l"
	.string "anything else.$"

Gauntlet_Text_BillLast:
	.string "BILL: Thank you, {PLAYER}.\p"
	.string "My PALM PILOT… everything I ever\n"
	.string "stored is in there. Every POKéMON.\p"
	.string "Take whatever you like.\n"
	.string "Call it my thanks.\p"
	.string "And my battle simulator… it has\n"
	.string "all of them. Even me.\p"
	.string "So you won't forget how hard this\n"
	.string "was. …Or what it cost.\p"
	.string "Goodbye, LEADER.$"

Gauntlet_Text_Faded:
	.string "BILL faded away, like mist\n"
	.string "in the morning…\p"
	.string "Only his PALM PILOT is left,\n"
	.string "its screen still glowing.$"

Gauntlet_Text_Pilot:
	.string "BILL's PALM PILOT.\n"
	.string "The screen is still on.$"

Gauntlet_Text_WithdrawAsk:
	.string "Would you like to WITHDRAW\n"
	.string "a POKéMON?$"

Gauntlet_Text_PilotMenu:
	.string "BILL'S BATTLE SIMULATOR$"

Gauntlet_Text_OptWithdraw:
	.string "WITHDRAW a POKéMON$"

Gauntlet_Text_OptSim:
	.string "Battle simulator$"

Gauntlet_Text_OptRecords:
	.string "Records$"

Gauntlet_Text_OptOff:
	.string "Turn it off$"

Gauntlet_Text_PilotOff:
	.string "The screen dimmed.$"

Gauntlet_Text_WithdrawWhich:
	.string "WITHDRAW which POKéMON?\n"
	.string "{L_BUTTON}{R_BUTTON} page  START next gen.$"

Gauntlet_Text_WithdrawConfirm:
	.string "WITHDRAW a shiny {STR_VAR_1}?$"

Gauntlet_Text_Withdrew:
	.string "{PLAYER} WITHDREW the shiny\n"
	.string "{STR_VAR_1}!$"

Gauntlet_Text_SentToPC:
	.string "There was no room with you, so it\n"
	.string "went to the PC.$"

Gauntlet_Text_NoRoom:
	.string "There's no room with you or in the\n"
	.string "PC for another POKéMON!$"

Gauntlet_Text_Records:
	.string "BILL's records. This run:\n"
	.string "{STR_VAR_1} of {STR_VAR_2} cleared.$"

Gauntlet_Text_SimStart:
	.string "BATTLE SIMULATOR: Every challenger\n"
	.string "from BILL's records, one by one.\p"
	.string "Clear the run for a WITHDRAW.$"

Gauntlet_Text_SimAnnounce:
	.string "SIMULATION: From {STR_VAR_2}…\n"
	.string "{STR_VAR_1}!$"

Gauntlet_Text_SimWon:
	.string "SIMULATION: {STR_VAR_1} down.\n"
	.string "{STR_VAR_2} to go.$"

Gauntlet_Text_SimContinue:
	.string "Load the next one?$"

Gauntlet_Text_SimBreak:
	.string "SIMULATION paused.$"

Gauntlet_Text_SimLost:
	.string "SIMULATION: Challenger not cleared.\n"
	.string "Resume any time.$"

Gauntlet_Text_SimComplete:
	.string "SIMULATION COMPLETE!\n"
	.string "A WITHDRAW is ready.$"
