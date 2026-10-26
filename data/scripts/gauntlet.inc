@ Pokémon Canvas: the tournament (src/tournament.c; its story and Bill: src/gauntlet.c). A hooded
@ bookie by the gym's door runs the book on an underground tournament of 128 of the world's
@ strongest trainers, and has put the player in the draw. Seven wins for the title; a loss ends the
@ run (sign up again for a new draw). After the first title: Bill, who put it all on, and then his
@ Palm Pilot - a WITHDRAW of any Pokémon, shiny, and his battle simulator: new tournaments, a
@ WITHDRAW for each one won.

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

@ Talking to the bookie (or, later, Bill; or his Palm Pilot)
Gauntlet_EventScript_Figure::
	lock
	textcolor NPC_TEXT_COLOR_NEUTRAL
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Gauntlet_EventScript_PalmPilot
	faceplayer
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_ALL_BEATEN, Gauntlet_EventScript_AllBeaten
	goto Tourney_EventScript_Menu

@ ---- The tournament (the bookie's, or Bill's simulator: the same, but for the words) ----
Tourney_EventScript_Menu::
	special Tourney_GetStatus
	goto_if_eq VAR_RESULT, TOURNEY_ACTIVE, Tourney_EventScript_MenuActive
	goto_if_eq VAR_RESULT, TOURNEY_NONE, Tourney_EventScript_MenuFirst
	call Tourney_EventScript_SayAsk
	dynmultichoice 0, 0, FALSE, 3, 0, DYN_MULTICHOICE_CB_NONE, Gauntlet_Text_OptEnter, Gauntlet_Text_OptLast, Gauntlet_Text_OptNotNow
	switch VAR_RESULT
	case 0, Tourney_EventScript_Enter
	case 1, Tourney_EventScript_LastResults
	goto Tourney_EventScript_NotNow

Tourney_EventScript_MenuFirst::
	call Tourney_EventScript_SayAsk
	dynmultichoice 0, 0, FALSE, 2, 0, DYN_MULTICHOICE_CB_NONE, Gauntlet_Text_OptEnter, Gauntlet_Text_OptNotNow
	goto_if_eq VAR_RESULT, 0, Tourney_EventScript_Enter
	goto Tourney_EventScript_NotNow

Tourney_EventScript_MenuActive::
	special Tourney_SetupNext
	call Tourney_EventScript_SayNext
	dynmultichoice 0, 0, FALSE, 4, 0, DYN_MULTICHOICE_CB_NONE, Gauntlet_Text_OptNext, Gauntlet_Text_OptBoard, Gauntlet_Text_OptForfeit, Gauntlet_Text_OptNotNow
	switch VAR_RESULT
	case 0, Tourney_EventScript_Match
	case 1, Tourney_EventScript_Board
	case 2, Tourney_EventScript_Forfeit
Tourney_EventScript_NotNow::
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Gauntlet_EventScript_PilotOff
	msgbox Gauntlet_Text_NotNow
	release
	end

Tourney_EventScript_Board::
	fadescreen FADE_TO_BLACK
	special Tourney_ShowBoard
	goto Tourney_EventScript_Menu

Tourney_EventScript_LastResults::
	fadescreen FADE_TO_BLACK
	special Tourney_ShowBoard
	special Tourney_BufferChampion
	call Tourney_EventScript_SayChampion
	goto Tourney_EventScript_Menu

Tourney_EventScript_Enter::
	call Tourney_EventScript_SayRules
	goto_if_eq VAR_RESULT, NO, Tourney_EventScript_NotNow
	special Tourney_Begin
	call Tourney_EventScript_SayDraw
	fadescreen FADE_TO_BLACK
	special Tourney_ShowBoard
	special Tourney_SetupNext
	call Tourney_EventScript_SayFirst
	goto_if_eq VAR_RESULT, NO, Tourney_EventScript_NotNow
Tourney_EventScript_Match::
	special Tourney_CheckTeam
	goto_if_eq VAR_RESULT, FALSE, Tourney_EventScript_WrongTeam
	special HealPlayerParty
	special GymChallenge_PlanPlayerWalk
	goto_if_eq VAR_RESULT, 0, GymChallenge_EventScript_CantReachSpot
	call Tourney_EventScript_SayTakeYourPlace
	closemessage
	applymovement LOCALID_PLAYER, gGymChallengePlayerPath
	waitmovement 0
Tourney_EventScript_NextMatch::
	special Tourney_SetupNext
	call Tourney_EventScript_SayAnnounce
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
	special Tourney_ShowTitleCard
	waitstate
	cleartrainerflag TRAINER_GYM_CHALLENGER
	setflag FLAG_GYM_CHALLENGE_BATTLE
	trainerbattle_no_intro TRAINER_GYM_CHALLENGER, gGymChallengeDefeatText
	clearflag FLAG_GYM_CHALLENGE_BATTLE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, Tourney_EventScript_Won
	special HealPlayerParty
	msgbox gGymChallengeWinText
	call GymChallenge_EventScript_ChallengerLeaves
	textcolor NPC_TEXT_COLOR_NEUTRAL
	special Tourney_RecordLoss
	call Tourney_EventScript_SayLost
	fadescreen FADE_TO_BLACK
	special Tourney_ShowBoard
	special Tourney_BufferChampion
	call Tourney_EventScript_SayChampion
	release
	end

Tourney_EventScript_Won::
	special HealPlayerParty
	msgbox gGymChallengeAfterText
	call GymChallenge_EventScript_ChallengerLeaves
	textcolor NPC_TEXT_COLOR_NEUTRAL
	special Tourney_RecordWin
	goto_if_eq VAR_RESULT, TRUE, Tourney_EventScript_Title
	call Tourney_EventScript_SayWon
Tourney_EventScript_BetweenRounds::
	dynmultichoice 0, 0, FALSE, 3, 0, DYN_MULTICHOICE_CB_NONE, Gauntlet_Text_OptNext, Gauntlet_Text_OptBoard, Gauntlet_Text_OptRest
	switch VAR_RESULT
	case 0, Tourney_EventScript_Continue
	case 1, Tourney_EventScript_BoardBetween
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Tourney_EventScript_SimBreak
	msgbox Gauntlet_Text_Break
	release
	end

Tourney_EventScript_SimBreak::
	msgbox Gauntlet_Text_SimBreak
	release
	end

Tourney_EventScript_BoardBetween::
	fadescreen FADE_TO_BLACK
	special Tourney_ShowBoard
	call Tourney_EventScript_SayNextQuestion
	goto Tourney_EventScript_BetweenRounds

Tourney_EventScript_Continue::
	closemessage
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	goto Tourney_EventScript_NextMatch

Tourney_EventScript_WrongTeam::
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Tourney_EventScript_SimWrongTeam
	msgbox Gauntlet_Text_WrongTeam
	release
	end

Tourney_EventScript_SimWrongTeam::
	msgbox Gauntlet_Text_SimWrongTeam
	release
	end

Tourney_EventScript_Forfeit::
	call Tourney_EventScript_SayForfeitAsk
	goto_if_eq VAR_RESULT, NO, Tourney_EventScript_Menu
	special Tourney_Forfeit
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Tourney_EventScript_SimForfeited
	msgbox Gauntlet_Text_Forfeited
	release
	end

Tourney_EventScript_SimForfeited::
	msgbox Gauntlet_Text_SimForfeited
	release
	end

@ The title
Tourney_EventScript_Title::
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	message Gauntlet_Text_Title
	waitfanfare
	waitmessage
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Tourney_EventScript_SimTitle
	msgbox Gauntlet_Text_TitleHall
	call_if_eq VAR_GAUNTLET_STATE, GAUNTLET_ALL_BEATEN, Tourney_EventScript_FirstTitle
	release
	end

Tourney_EventScript_FirstTitle::
	playse SE_PIN
	msgbox Gauntlet_Text_LastOne
	return

Tourney_EventScript_SimTitle::
	msgbox Gauntlet_Text_TitleHall
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	message Gauntlet_Text_SimComplete
	waitfanfare
	waitmessage
	msgbox Gauntlet_Text_WithdrawAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, Gauntlet_EventScript_Withdraw
	release
	end

@ ---- What's said: the bookie's words, or the simulator's ----
Tourney_EventScript_SayAsk::
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Tourney_EventScript_SayAskSim
	message Gauntlet_Text_FigureAsk
	waitmessage
	return
Tourney_EventScript_SayAskSim::
	message Gauntlet_Text_SimAsk
	waitmessage
	return

Tourney_EventScript_SayNext::
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Tourney_EventScript_SayNextSim
	message Gauntlet_Text_Next
	waitmessage
	return
Tourney_EventScript_SayNextSim::
	message Gauntlet_Text_SimNext
	waitmessage
	return

Tourney_EventScript_SayNextQuestion::
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Tourney_EventScript_SayNextQuestionSim
	message Gauntlet_Text_ContinueOrBreak
	waitmessage
	return
Tourney_EventScript_SayNextQuestionSim::
	message Gauntlet_Text_SimContinue
	waitmessage
	return

Tourney_EventScript_SayRules::
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Tourney_EventScript_SayRulesSim
	msgbox Gauntlet_Text_Rules, MSGBOX_YESNO
	return
Tourney_EventScript_SayRulesSim::
	msgbox Gauntlet_Text_SimStart, MSGBOX_YESNO
	return

Tourney_EventScript_SayDraw::
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Tourney_EventScript_SayDrawSim
	msgbox Gauntlet_Text_Draw
	return
Tourney_EventScript_SayDrawSim::
	msgbox Gauntlet_Text_SimDraw
	return

Tourney_EventScript_SayFirst::
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Tourney_EventScript_SayFirstSim
	msgbox Gauntlet_Text_First, MSGBOX_YESNO
	return
Tourney_EventScript_SayFirstSim::
	msgbox Gauntlet_Text_SimFirst, MSGBOX_YESNO
	return

Tourney_EventScript_SayTakeYourPlace::
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Tourney_EventScript_Return
	msgbox Gauntlet_Text_TakeYourPlace
Tourney_EventScript_Return::
	return

Tourney_EventScript_SayAnnounce::
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Tourney_EventScript_SayAnnounceSim
	msgbox Gauntlet_Text_Announce
	return
Tourney_EventScript_SayAnnounceSim::
	msgbox Gauntlet_Text_SimAnnounce
	return

Tourney_EventScript_SayWon::
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Tourney_EventScript_SayWonSim
	message Gauntlet_Text_Won
	waitmessage
	return
Tourney_EventScript_SayWonSim::
	message Gauntlet_Text_SimWon
	waitmessage
	return

Tourney_EventScript_SayLost::
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Tourney_EventScript_SayLostSim
	msgbox Gauntlet_Text_Lost
	return
Tourney_EventScript_SayLostSim::
	msgbox Gauntlet_Text_SimLost
	return

Tourney_EventScript_SayChampion::
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Tourney_EventScript_SayChampionSim
	msgbox Gauntlet_Text_Champion
	return
Tourney_EventScript_SayChampionSim::
	msgbox Gauntlet_Text_SimChampion
	return

Tourney_EventScript_SayForfeitAsk::
	goto_if_eq VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR, Tourney_EventScript_SayForfeitAskSim
	msgbox Gauntlet_Text_ForfeitAsk, MSGBOX_YESNO
	return
Tourney_EventScript_SayForfeitAskSim::
	msgbox Gauntlet_Text_SimForfeitAsk, MSGBOX_YESNO
	return

@ ---- The one who put it on ----
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
	special Gauntlet_SetupBill
	msgbox gGymChallengeIntroText
	closemessage
	fadescreen FADE_TO_BLACK
	special Gauntlet_ShowBillCard
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
	@ (the gym's music, from the start: after fadeoutbgm, fadedefaultbgm would wait forever for the
	@ faded song to stop, and every warp after it would hang)
	special Overworld_PlaySpecialMapMusic
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
	dynmultichoice 0, 0, FALSE, 2, 0, DYN_MULTICHOICE_CB_NONE, Gauntlet_Text_OptSim, Gauntlet_Text_OptOff
	goto_if_eq VAR_RESULT, 0, Tourney_EventScript_Menu
	goto Gauntlet_EventScript_PilotOff

Gauntlet_EventScript_PilotMenuWithdraw::
	dynmultichoice 0, 0, FALSE, 3, 0, DYN_MULTICHOICE_CB_NONE, Gauntlet_Text_OptWithdraw, Gauntlet_Text_OptSim, Gauntlet_Text_OptOff
	switch VAR_RESULT
	case 0, Gauntlet_EventScript_Withdraw
	case 1, Tourney_EventScript_Menu
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
	givemon VAR_0x8005, 70, shinyMode=SHINY_MODE_ALWAYS
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

@ ---- Text ----
Gauntlet_Text_Arrival1:
	.string "???: So. You're the one.\p"
	.string "Every hall of the convention\n"
	.string "cleared. Every type mastered.\p"
	.string "Do you have any idea how much\n"
	.string "money is riding on you, LEADER?$"

Gauntlet_Text_Arrival2:
	.string "???: Relax. I'm not here to\n"
	.string "challenge your gym. I run a book.\p"
	.string "Bets. Odds. Quietly, you\n"
	.string "understand. The LEAGUE wouldn't.\p"
	.string "And the biggest game in town is\n"
	.string "a tournament. No LEAGUE, no\l"
	.string "rules, no cameras.\p"
	.string "128 trainers. CHAMPIONS, the\n"
	.string "ELITE FOUR, GYM LEADERS…\p"
	.string "Rivals. Villains. Legends.\n"
	.string "The strongest there are.$"

Gauntlet_Text_Arrival3:
	.string "???: I've put your name in\n"
	.string "the draw.\p"
	.string "Single elimination. Seven wins\n"
	.string "takes the title. Lose once, and\l"
	.string "you're out… until the next one.\p"
	.string "The odds on you are long, LEADER.\n"
	.string "That's what makes it fun.\p"
	.string "Speak to me when you want in.$"

Gauntlet_Text_FigureAsk:
	.string "???: Well, LEADER? Feeling lucky?$"

Gauntlet_Text_OptEnter:
	.string "Enter the tournament$"

Gauntlet_Text_OptLast:
	.string "The last results$"

Gauntlet_Text_OptNext:
	.string "Next match$"

Gauntlet_Text_OptBoard:
	.string "See the board$"

Gauntlet_Text_OptForfeit:
	.string "Forfeit$"

Gauntlet_Text_OptRest:
	.string "Rest$"

Gauntlet_Text_OptNotNow:
	.string "Not now$"

Gauntlet_Text_NotNow:
	.string "???: The book's always open.$"

Gauntlet_Text_Rules:
	.string "???: 128 entrants. Seven rounds.\n"
	.string "Lose once and you're out.\p"
	.string "Everyone's at level 100. You'll be\n"
	.string "healed between rounds.\p"
	.string "And the team you sign up with is\n"
	.string "the team you play with: the\l"
	.string "POKéMON with you now, no others.\p"
	.string "Sign up?$"

Gauntlet_Text_Draw:
	.string "???: Done. The draw's made.\n"
	.string "Have a look at the board.$"

Gauntlet_Text_First:
	.string "???: The {STR_VAR_2}. You're up\n"
	.string "against {STR_VAR_1}.\p"
	.string "Ready to play?$"

Gauntlet_Text_Next:
	.string "???: The {STR_VAR_2}.\n"
	.string "{STR_VAR_1} is waiting.$"

Gauntlet_Text_TakeYourPlace:
	.string "???: Take your place, LEADER.$"

Gauntlet_Text_Announce:
	.string "???: The {STR_VAR_2}!\n"
	.string "Facing you… {STR_VAR_1}!$"

Gauntlet_Text_Won:
	.string "???: Hmph. On to the {STR_VAR_1}.\n"
	.string "Next match now, or a breather?$"

Gauntlet_Text_ContinueOrBreak:
	.string "???: Next match now, or\n"
	.string "a breather?$"

Gauntlet_Text_Break:
	.string "???: Rest, then. Your opponent\n"
	.string "will wait. So will the bets.$"

Gauntlet_Text_Lost:
	.string "???: Out in the {STR_VAR_2}.\n"
	.string "As the odds said.\p"
	.string "Here's how the rest of it\n"
	.string "played out.$"

Gauntlet_Text_Champion:
	.string "???: {STR_VAR_1} took the title.\p"
	.string "There's always another draw,\n"
	.string "LEADER. Sign up when you're ready.$"

Gauntlet_Text_WrongTeam:
	.string "???: That's not the team you\n"
	.string "signed up with.\p"
	.string "Bring back the same POKéMON,\n"
	.string "no more and no fewer… or forfeit.$"

Gauntlet_Text_ForfeitAsk:
	.string "???: Forfeit? Your next match goes\n"
	.string "to the other side. Sure?$"

Gauntlet_Text_Forfeited:
	.string "???: Your loss. Literally.\n"
	.string "The book's closed on you… for now.$"

Gauntlet_Text_Title:
	.string "{PLAYER} won the tournament!$"

Gauntlet_Text_TitleHall:
	.string "Your team's been recorded in the\n"
	.string "PC's TOURNAMENT Hall of Fame.$"

Gauntlet_Text_LastOne:
	.string "???: …You won. All seven.\n"
	.string "You actually won the whole thing.\p"
	.string "…Good. Better than good.\p"
	.string "Then it's time you met the one who\n"
	.string "really put this tournament on.\p"
	.string "Rest first. Speak to me when\n"
	.string "you're ready.$"

Gauntlet_Text_ReadyToMeet:
	.string "???: Are you ready to meet the one\n"
	.string "behind the tournament, LEADER?$"

Gauntlet_Text_NotYet:
	.string "???: …Don't take too long.$"

Gauntlet_Text_Reveal1:
	.string "???: …There's no book.\n"
	.string "No bets, no odds. There never were.\p"
	.string "I put on the tournament. Every\n"
	.string "invitation, every match.\p"
	.string "I needed to find the strongest\n"
	.string "trainer alive.$"

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
	.string "What came out of that machine\n"
	.string "isn't me anymore. Not really.\p"
	.string "It's strong. Stronger than any\n"
	.string "POKéMON I've ever stored.\p"
	.string "And I'm trapped inside it.\p"
	.string "I can think. I can talk.\n"
	.string "That's all it lets me do.\p"
	.string "It hurts. All the time. And it\n"
	.string "won't let itself die.\p"
	.string "When it battles, it battles to win.\n"
	.string "I can't hold it back.$"

Gauntlet_Text_Reveal4:
	.string "BILL: So I needed someone strong.\n"
	.string "Strong enough to beat it.\p"
	.string "I put on the tournament to find\n"
	.string "the strongest trainer there is.\p"
	.string "And it's you.\p"
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
	.string "Rest. Get stronger. Try again.\n"
	.string "I'll be waiting. I can't do\l"
	.string "anything else.$"

Gauntlet_Text_BillLast:
	.string "BILL: Thank you, {PLAYER}.\p"
	.string "My PALM PILOT… everything I ever\n"
	.string "stored is in there. Every POKéMON.\p"
	.string "Take whatever you like.\n"
	.string "Call it my thanks.\p"
	.string "And my battle simulator… it can\n"
	.string "run the tournament again. As many\l"
	.string "times as you like.\p"
	.string "Win one, and take another.\p"
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

Gauntlet_Text_SimAsk:
	.string "BATTLE SIMULATOR: TOURNAMENT MODE$"

Gauntlet_Text_SimStart:
	.string "A new tournament: 128 trainers\n"
	.string "from BILL's records, and you.\p"
	.string "Win it for a WITHDRAW. The team\n"
	.string "with you now is locked in.\p"
	.string "Start the simulation?$"

Gauntlet_Text_SimDraw:
	.string "SIMULATION: The draw is made.$"

Gauntlet_Text_SimFirst:
	.string "{STR_VAR_2}: {STR_VAR_1}.\n"
	.string "Load the match?$"

Gauntlet_Text_SimNext:
	.string "SIMULATION: {STR_VAR_2}.\n"
	.string "Next: {STR_VAR_1}.$"

Gauntlet_Text_SimAnnounce:
	.string "SIMULATION: {STR_VAR_2}…\n"
	.string "{STR_VAR_1}!$"

Gauntlet_Text_SimWon:
	.string "SIMULATION: On to the\n"
	.string "{STR_VAR_1}.$"

Gauntlet_Text_SimContinue:
	.string "Load the next match?$"

Gauntlet_Text_SimBreak:
	.string "SIMULATION paused.$"

Gauntlet_Text_SimLost:
	.string "SIMULATION: Out in the\n"
	.string "{STR_VAR_2}. The rest, simulated…$"

Gauntlet_Text_SimChampion:
	.string "SIMULATION: {STR_VAR_1} won\n"
	.string "the tournament.$"

Gauntlet_Text_SimWrongTeam:
	.string "SIMULATION: That isn't the team\n"
	.string "entered. Bring the same POKéMON,\l"
	.string "or forfeit.$"

Gauntlet_Text_SimForfeitAsk:
	.string "Forfeit the simulation?$"

Gauntlet_Text_SimForfeited:
	.string "SIMULATION ended.$"

Gauntlet_Text_SimComplete:
	.string "SIMULATION COMPLETE!\n"
	.string "A WITHDRAW is ready.$"
