@ Gym Trainer hack: challengers at the player's gym (see src/gym_challenge.c).
@ Oak's aide sends them in one at a time; the player battles until they lose.

GymChallenge_EventScript_Aide::
	lock
	faceplayer
	specialvar VAR_RESULT, GymChallenge_IsOpen
	goto_if_eq VAR_RESULT, FALSE, GymChallenge_EventScript_AideNotYet
	special GymChallenge_BufferRecord
	msgbox GymChallenge_Text_AideWelcome
	msgbox GymChallenge_Text_ReadyToBattle, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, GymChallenge_EventScript_NotNow
	special GymHack_CheckTeamSpecial
	goto_if_ne VAR_RESULT, GYM_HACK_TEAM_OK, GymChallenge_EventScript_TeamBreaksRules
	special HealPlayerParty
	special GymChallenge_PlanPlayerWalk
	goto_if_eq VAR_RESULT, 0, GymChallenge_EventScript_CantReachSpot
	call_if_eq VAR_RESULT, 2, GymChallenge_EventScript_NoStage
	call_if_eq VAR_RESULT, 3, GymChallenge_EventScript_StageBlocked
	msgbox GymChallenge_Text_TakeYourPlace
	closemessage
	applymovement LOCALID_PLAYER, gGymChallengePlayerPath
	waitmovement 0
	goto GymChallenge_EventScript_NextChallenger
	end

GymChallenge_EventScript_NoStage::
	msgbox GymChallenge_Text_NoStage
	return

GymChallenge_EventScript_StageBlocked::
	msgbox GymChallenge_Text_StageBlocked
	return

GymChallenge_EventScript_CantReachSpot::
	msgbox GymChallenge_Text_CantReachSpot
	release
	end

GymChallenge_EventScript_AideNotYet::
	special GymCandidate_BufferApprovals
	msgbox GymChallenge_Text_AideNotYet
	release
	end

GymChallenge_EventScript_NotNow::
	msgbox GymChallenge_Text_NotNow
	release
	end

GymChallenge_EventScript_TeamBreaksRules::
	msgbox GymChallenge_Text_TeamBreaksRules
	release
	end

GymChallenge_EventScript_NextChallenger::
	special GymChallenge_NextChallenger
	call_if_eq VAR_RESULT, GYM_CHALLENGER_FAMOUS, GymChallenge_EventScript_AnnounceFamous
	call_if_eq VAR_RESULT, GYM_CHALLENGER_TOWNSFOLK, GymChallenge_EventScript_AnnounceTownsfolk
	call_if_eq VAR_RESULT, GYM_CHALLENGER_TRAINER, GymChallenge_EventScript_AnnounceTrainer
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
	cleartrainerflag TRAINER_GYM_CHALLENGER
	setflag FLAG_GYM_CHALLENGE_BATTLE
	trainerbattle_no_intro TRAINER_GYM_CHALLENGER, gGymChallengeDefeatText
	clearflag FLAG_GYM_CHALLENGE_BATTLE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, GymChallenge_EventScript_Won
	goto GymChallenge_EventScript_Lost
	end

GymChallenge_EventScript_AnnounceTrainer::
	msgbox GymChallenge_Text_AnnounceTrainer
	return

GymChallenge_EventScript_AnnounceFamous::
	msgbox GymChallenge_Text_AnnounceFamous
	return

GymChallenge_EventScript_AnnounceTownsfolk::
	msgbox GymChallenge_Text_AnnounceTownsfolk
	return

@ No way from the door to their mark (the room is walled off): they're just there
GymChallenge_EventScript_ChallengerJumpsIn::
	setobjectxy LOCALID_PALLET_GYM_CHALLENGER, VAR_0x8005, VAR_0x8006
	return

GymChallenge_EventScript_ChallengerLeaves::
	closemessage
	setvar VAR_0x8004, 1
	special GymChallenge_PlanChallengerWalk
	goto_if_eq VAR_RESULT, FALSE, GymChallenge_EventScript_ChallengerGone
	applymovement LOCALID_PALLET_GYM_CHALLENGER, gGymChallengeChallengerPath
	waitmovement 0
	playse SE_EXIT
GymChallenge_EventScript_ChallengerGone::
	removeobject LOCALID_PALLET_GYM_CHALLENGER
	setflag FLAG_HIDE_PALLET_GYM_CHALLENGER
	return

GymChallenge_EventScript_Won::
	msgbox gGymChallengeAfterText
	call GymChallenge_EventScript_ChallengerLeaves
	special GymChallenge_RecordWin
	copyvar VAR_0x8008, VAR_RESULT
	playse SE_SHOP
	msgbox GymChallenge_Text_Won
	call_if_eq VAR_0x8008, TRUE, GymChallenge_EventScript_NewRecord
	msgbox GymChallenge_Text_LeftAGift
	call GymHack_EventScript_GiveStatItem
	special HealPlayerParty
	msgbox GymChallenge_Text_KeepGoing, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, GymChallenge_EventScript_KeepGoing
	special GymChallenge_BufferRecord
	msgbox GymChallenge_Text_TakeABreak
	release
	end

GymChallenge_EventScript_KeepGoing::
	closemessage
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	goto GymChallenge_EventScript_NextChallenger
	end

GymChallenge_EventScript_NewRecord::
	playfanfare MUS_LEVEL_UP
	msgbox GymChallenge_Text_NewRecord
	waitfanfare
	return

GymChallenge_EventScript_Lost::
	special HealPlayerParty
	msgbox gGymChallengeWinText
	special GymChallenge_RecordLoss
	special GymBadge_BufferName
	special GymBadge_ShowIcon
	playfanfare MUS_OBTAIN_BADGE
	msgbox GymChallenge_Text_AwardBadge
	waitfanfare
	special GymBadge_HideIcon
	call GymChallenge_EventScript_ChallengerLeaves
	special GymChallenge_BufferLastResult
	msgbox GymChallenge_Text_Lost
	release
	end

@ ---- The gym statue ----
GymChallenge_EventScript_Statue::
	lockall
	specialvar VAR_RESULT, GymChallenge_IsOpen
	goto_if_eq VAR_RESULT, FALSE, GymChallenge_EventScript_StatueNoLeader
	special GymChallenge_BufferRecord
	msgbox GymChallenge_Text_StatueLeader
	special GymChallenge_BufferVictorList
	goto_if_eq VAR_RESULT, 0, GymChallenge_EventScript_StatueUnbeaten
	special GymBadge_BufferName
	msgbox GymChallenge_Text_StatueVictors
	special GymChallenge_BufferVictorList   @ again: the message above used gStringVar4
	msgbox gStringVar4
	releaseall
	end

GymChallenge_EventScript_StatueUnbeaten::
	msgbox GymChallenge_Text_StatueUnbeaten
	releaseall
	end

GymChallenge_EventScript_StatueNoLeader::
	msgbox GymChallenge_Text_StatueNoLeader
	releaseall
	end

@ ---- Movement ----
@ ---- Text ----
GymChallenge_Text_AideNotYet::
	.string "Hello! I'm one of PROF. OAK's\n"
	.string "AIDES.\p"
	.string "Once the LEAGUE names this GYM's\n"
	.string "LEADER, I'll be running the\l"
	.string "challengers.\p"
	.string "For now, the PC over there is for\n"
	.string "decorating!\p"
	.string "GYM LEADERS who approve of you\n"
	.string "so far: {STR_VAR_1} of 8.$"

GymChallenge_Text_AideWelcome::
	.string "Welcome back, LEADER {PLAYER}!\n"
	.string "You have several challengers today.\p"
	.string "Wins in a row: {STR_VAR_1}.\n"
	.string "Best streak: {STR_VAR_2}.\p"
	.string "All-time record: {STR_VAR_3}\n"
	.string "(wins-losses).$"

GymChallenge_Text_ReadyToBattle::
	.string "They'll keep coming until one of\n"
	.string "them beats you. Ready?$"

GymChallenge_Text_NotNow::
	.string "No problem. They'll wait!$"

GymChallenge_Text_TeamBreaksRules::
	.string "Hold on! LEAGUE rules: a GYM\n"
	.string "LEADER battles with one type.\p"
	.string "Fix your team, then come see me!$"

GymChallenge_Text_NoStage::
	.string "Tip: put the BATTLE STAGE anywhere\n"
	.string "in the GYM using the PC.\p"
	.string "Until then, we'll battle right here!$"

GymChallenge_Text_StageBlocked::
	.string "Hmm, there's no way to the BATTLE\n"
	.string "STAGE. We'll battle right here!$"

GymChallenge_Text_CantReachSpot::
	.string "Hmm… the decorations are in the\n"
	.string "way. Clear a path first!$"

GymChallenge_Text_TakeYourPlace::
	.string "Take your place, LEADER!$"

GymChallenge_Text_AnnounceTrainer::
	.string "Next challenger, come on in!$"

GymChallenge_Text_AnnounceFamous::
	.string "Wait… is that who I think it is?!\n"
	.string "Come in, come in!$"

GymChallenge_Text_AnnounceTownsfolk::
	.string "Huh? Someone from around town\n"
	.string "signed up…$"

GymChallenge_Text_Won::
	.string "That's {STR_VAR_2} in a row!\p"
	.string "The LEAGUE pays ¥{STR_VAR_1}\n"
	.string "for the win.$"

GymChallenge_Text_NewRecord::
	.string "And that's a new GYM record:\n"
	.string "{STR_VAR_2} in a row!$"

GymChallenge_Text_LeftAGift::
	.string "{STR_VAR_3} left you\n"
	.string "something as thanks.$"

GymChallenge_Text_KeepGoing::
	.string "Your POKéMON are rested.\n"
	.string "Shall I send in the next one?$"

GymChallenge_Text_TakeABreak::
	.string "I'll tell them to come back later.\n"
	.string "Your streak of {STR_VAR_1} is safe!$"

GymChallenge_Text_AwardBadge::
	.string "{PLAYER} presented\n"
	.string "{STR_VAR_3}…\p"
	.string "…with the {STR_VAR_1}!$"

GymChallenge_Text_Lost::
	.string "Oh no… that ends your streak.\n"
	.string "Wins in a row: {STR_VAR_1}. Best: {STR_VAR_2}.\p"
	.string "{STR_VAR_3}'s name goes\n"
	.string "on the GYM statue.\p"
	.string "I healed your POKéMON.\n"
	.string "Come back when you're ready!$"

GymChallenge_Text_StatueNoLeader::
	.string "PALLET TOWN POKéMON GYM\n"
	.string "LEADER: To be decided!$"

GymChallenge_Text_StatueLeader::
	.string "PALLET TOWN POKéMON GYM\n"
	.string "LEADER: {PLAYER}\p"
	.string "Best streak: {STR_VAR_2} in a row.\n"
	.string "Record: {STR_VAR_3}.$"

GymChallenge_Text_StatueUnbeaten::
	.string "No challenger has beaten\n"
	.string "{PLAYER} yet!$"

GymChallenge_Text_StatueVictors::
	.string "Challengers who beat {PLAYER} and\n"
	.string "earned the {STR_VAR_1}:$"
