@ The Gym Leader Convention in Goldenrod's Radio Tower (src/gym_convention.c)
@ The receptionist in the lobby sends the player to any hall; every trainer in the halls battles
@ with their full team, and the player's party is healed after each battle, won or lost.

Convention_EventScript_Receptionist::
	lock
	faceplayer
	special Convention_BufferProgress
	goto_if_ne VAR_RESULT, 0, Convention_EventScript_WelcomeBack
	msgbox Convention_Text_Welcome
	goto Convention_EventScript_ChooseHall

Convention_EventScript_WelcomeBack::
	goto_if_eq VAR_RESULT, CONVENTION_HALL_COUNT, Convention_EventScript_AllCleared
	msgbox Convention_Text_WelcomeBack
Convention_EventScript_ChooseHall::
	message Convention_Text_WhichHall
	waitmessage
	special Convention_PushHalls
	dynmultistack 0, 0, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, Convention_EventScript_Bye
	copyvar VAR_0x8004, VAR_RESULT
	msgbox Convention_Text_RightThisWay
	closemessage
	playse SE_EXIT
	special Convention_WarpToHall
	waitstate
	release
	end

Convention_EventScript_AllCleared::
	msgbox Convention_Text_AllCleared
	goto Convention_EventScript_ChooseHall

Convention_EventScript_Bye::
	msgbox Convention_Text_Bye
	release
	end

@ A trainer in a hall: VAR_0x8004 = which (set by the hall's map script)
Convention_EventScript_Trainer::
	lock
	faceplayer
	special Convention_SetupTrainer
	goto_if_eq VAR_RESULT, TRUE, Convention_EventScript_Rematch
	msgbox gGymChallengeIntroText
	closemessage
Convention_EventScript_Battle::
	cleartrainerflag TRAINER_GYM_CHALLENGER
	setflag FLAG_GYM_CHALLENGE_BATTLE
	trainerbattle_no_intro TRAINER_GYM_CHALLENGER, gGymChallengeDefeatText
	clearflag FLAG_GYM_CHALLENGE_BATTLE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_WON, Convention_EventScript_Lost
	special HealPlayerParty
	msgbox gGymChallengeAfterText
	special Convention_RecordWin
	goto_if_eq VAR_RESULT, CONVENTION_WON_HALL, Convention_EventScript_WonHall
	goto_if_eq VAR_RESULT, CONVENTION_WON_ALL, Convention_EventScript_WonAll
	release
	end

Convention_EventScript_Lost::
	special HealPlayerParty
	msgbox Convention_Text_Lost
	release
	end

Convention_EventScript_Rematch::
	msgbox gGymChallengeAfterText
	msgbox Convention_Text_Rematch, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Convention_EventScript_NoRematch
	closemessage
	goto Convention_EventScript_Battle

Convention_EventScript_NoRematch::
	release
	end

Convention_EventScript_WonHall::
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	msgbox Convention_Text_WonHall
	waitfanfare
	release
	end

Convention_EventScript_WonAll::
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	msgbox Convention_Text_WonHall
	waitfanfare
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	msgbox Convention_Text_WonAll
	waitfanfare
	release
	end

Convention_Text_Welcome:
	.string "Welcome to the GYM LEADER\n"
	.string "CONVENTION!\p"
	.string "Every GYM LEADER, ELITE FOUR member\n"
	.string "and CHAMPION is here, from all over!\p"
	.string "There's a hall for each type, and\n"
	.string "the CHAMPIONS' HALL. Visit them in\l"
	.string "any order you like.\p"
	.string "Talk to anyone in a hall to battle.\n"
	.string "Their POKéMON are at full strength!\p"
	.string "Don't worry, we heal your team\n"
	.string "after every battle, win or lose.\p"
	.string "Beat everyone in a hall to win its\n"
	.string "TROPHY for your GYM.$"

Convention_Text_WelcomeBack:
	.string "Welcome back, LEADER {PLAYER}!\n"
	.string "Halls cleared so far: {STR_VAR_1}.$"

Convention_Text_AllCleared:
	.string "LEADER {PLAYER}! You've cleared\n"
	.string "every hall of the CONVENTION!\p"
	.string "The LEADERS will happily battle\n"
	.string "you again, any time.$"

Convention_Text_WhichHall:
	.string "Which hall would you like to visit?$"

Convention_Text_RightThisWay:
	.string "Right this way!$"

Convention_Text_Bye:
	.string "Enjoy the CONVENTION!$"

Convention_Text_Lost:
	.string "Your POKéMON were healed.\n"
	.string "Try again whenever you're ready!$"

Convention_Text_Rematch:
	.string "{STR_VAR_1}: How about another\n"
	.string "battle?$"

Convention_Text_WonHall:
	.string "You've beaten everyone in the\n"
	.string "{STR_VAR_2} HALL!\p"
	.string "{PLAYER} won the {STR_VAR_2} TROPHY!\n"
	.string "It was sent to your GYM's storage.$"

Convention_Text_WonAll:
	.string "And that's every hall of the\n"
	.string "CONVENTION!\p"
	.string "{PLAYER} won the GRAND TROPHY!\n"
	.string "It was sent to your GYM's storage.$"
