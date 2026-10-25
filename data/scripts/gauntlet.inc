@ Pokémon Canvas: the Gauntlet (src/gauntlet.c). A hooded figure by the gym's door calls in the
@ strongest trainers of the Pokémon world, one at a time. Losing changes nothing: the same
@ challenger comes next time.

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
	faceplayer
	textcolor NPC_TEXT_COLOR_NEUTRAL
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
	msgbox Gauntlet_Text_Clipboard
Gauntlet_EventScript_ClipboardRegions::
	message Gauntlet_Text_WhichPage
	waitmessage
	special Gauntlet_PushRegions
	dynmultistack 0, 0, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, Gauntlet_EventScript_FigureMenu
	copyvar VAR_0x8004, VAR_RESULT
	message Gauntlet_Text_Page
	waitmessage
	special Gauntlet_PushRegionNames
	dynmultistack 0, 0, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto Gauntlet_EventScript_ClipboardRegions

@ ---- The battles ----
Gauntlet_EventScript_Begin::
	special HealPlayerParty
	special GymChallenge_PlanPlayerWalk
	goto_if_eq VAR_RESULT, 0, GymChallenge_EventScript_CantReachSpot
	msgbox Gauntlet_Text_TakeYourPlace
	closemessage
	applymovement LOCALID_PLAYER, gGymChallengePlayerPath
	waitmovement 0
Gauntlet_EventScript_Next::
	special Gauntlet_SetupNext
	msgbox Gauntlet_Text_Announce
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
	msgbox Gauntlet_Text_Lost
	release
	end

Gauntlet_EventScript_Won::
	special HealPlayerParty
	msgbox gGymChallengeAfterText
	call GymChallenge_EventScript_ChallengerLeaves
	special Gauntlet_RecordWin
	textcolor NPC_TEXT_COLOR_NEUTRAL
	goto_if_eq VAR_RESULT, TRUE, Gauntlet_EventScript_LastOne
	msgbox Gauntlet_Text_Won
	msgbox Gauntlet_Text_ContinueOrBreak, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Gauntlet_EventScript_Break
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

Gauntlet_EventScript_AllBeaten::
	msgbox Gauntlet_Text_AllBeaten
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
	.string "the one who really hired me.$"

Gauntlet_Text_AllBeaten:
	.string "???: Not yet. Soon.\n"
	.string "When it's time, you'll know.$"
