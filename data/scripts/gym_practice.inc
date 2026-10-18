@ Gym Trainer hack: the practice trainers in the player's gym (bought at the gym PC; teams are
@ built by GymChallenge_GeneratePractice in src/gym_challenge.c). No money changes hands, and
@ losing just ends the practice.

@ One script per trainer slot in the gym map (VAR_0x8004 = slot)
GymPractice_EventScript_Trainer0::
	setvar VAR_0x8004, 0
	goto GymPractice_EventScript_Trainer
	end

GymPractice_EventScript_Trainer1::
	setvar VAR_0x8004, 1
	goto GymPractice_EventScript_Trainer
	end

GymPractice_EventScript_Trainer2::
	setvar VAR_0x8004, 2
	goto GymPractice_EventScript_Trainer
	end

GymPractice_EventScript_Trainer3::
	setvar VAR_0x8004, 3
	goto GymPractice_EventScript_Trainer
	end

GymPractice_EventScript_Trainer4::
	setvar VAR_0x8004, 4
	goto GymPractice_EventScript_Trainer
	end

GymPractice_EventScript_Trainer5::
	setvar VAR_0x8004, 5
	goto GymPractice_EventScript_Trainer
	end

GymPractice_EventScript_Trainer6::
	setvar VAR_0x8004, 6
	goto GymPractice_EventScript_Trainer
	end

GymPractice_EventScript_Trainer7::
	setvar VAR_0x8004, 7
	goto GymPractice_EventScript_Trainer
	end

GymPractice_EventScript_Trainer8::
	setvar VAR_0x8004, 8
	goto GymPractice_EventScript_Trainer
	end

GymPractice_EventScript_Trainer9::
	setvar VAR_0x8004, 9
	goto GymPractice_EventScript_Trainer
	end

GymPractice_EventScript_Trainer10::
	setvar VAR_0x8004, 10
	goto GymPractice_EventScript_Trainer
	end

GymPractice_EventScript_Trainer::
	lock
	faceplayer
	special GymChallenge_BufferPracticeTrainer
	msgbox GymPractice_Text_Offer, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, GymPractice_EventScript_NotNow
	special GymChallenge_GeneratePractice
	msgbox GymPractice_Text_Go
	closemessage
	cleartrainerflag TRAINER_GYM_CHALLENGER
	setflag FLAG_GYM_CHALLENGE_BATTLE
	trainerbattle_no_intro TRAINER_GYM_CHALLENGER, gGymChallengeDefeatText
	clearflag FLAG_GYM_CHALLENGE_BATTLE
	special GymChallenge_EndPractice
	special HealPlayerParty
	specialvar VAR_RESULT, GetBattleOutcome
	special GymChallenge_BufferPracticeTrainer
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, GymPractice_EventScript_Won
	msgbox GymPractice_Text_Lost
	release
	end

GymPractice_EventScript_Won::
	msgbox GymPractice_Text_Won
	release
	end

GymPractice_EventScript_NotNow::
	msgbox GymPractice_Text_NotNow
	release
	end

GymPractice_Text_Offer::
	.string "{STR_VAR_2} TRAINER {STR_VAR_1}: Hi, LEADER!\n"
	.string "Want a practice battle?$"

GymPractice_Text_Go::
	.string "{STR_VAR_1}: All right!\n"
	.string "Don't go easy on me!$"

GymPractice_Text_Won::
	.string "{STR_VAR_1}: I'll train harder for\n"
	.string "next time!\p"
	.string "I healed your POKéMON.\n"
	.string "Come practice any time!$"

GymPractice_Text_Lost::
	.string "{STR_VAR_1}: I… I won?!\n"
	.string "Don't worry, it was just practice!\p"
	.string "I healed your POKéMON.\n"
	.string "Let's go again sometime!$"

GymPractice_Text_NotNow::
	.string "{STR_VAR_1}: OK! I'll keep\n"
	.string "training here.$"
