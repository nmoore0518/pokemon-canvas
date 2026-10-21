@ Gym Trainer hack (Pokémon Canvas): changing the gym's type after the League (src/gym_switch.c).
@ A long enough streak ends with the player's mentor (the mastery battle). Beating them earns a new
@ type; when the streak ends (a loss, or "call it a day"), Oak comes to the gym and offers it. Turned
@ down, the offer waits in his lab, where the gym can also go back to any type it has had, free.

@ ---- The aide ----
GymSwitch_EventScript_AideGoal::
	msgbox GymSwitch_Text_AideGoal
	return

GymSwitch_EventScript_AnnounceMentor::
	msgbox GymSwitch_Text_AnnounceMentor
	return

GymSwitch_EventScript_Mastered::
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	msgbox GymSwitch_Text_Mastered
	waitfanfare
	msgbox GymSwitch_Text_OakWillCome
	return

@ After each win once the type is mastered: keep going, end the streak here (Oak comes), or take a
@ break (the streak's kept, and Oak comes when it ends)
GymSwitch_EventScript_KeepGoingOrRetire::
	message GymSwitch_Text_WhatNext
	waitmessage
	dynmultichoice 0, 0, FALSE, 3, 0, DYN_MULTICHOICE_CB_NONE, GymSwitch_Text_KeepGoing, GymSwitch_Text_CallItADay, GymSwitch_Text_TakeABreak
	goto_if_eq VAR_RESULT, 0, GymChallenge_EventScript_KeepGoing
	goto_if_eq VAR_RESULT, 1, GymSwitch_EventScript_CallItADay
	special GymChallenge_BufferRecord
	msgbox GymChallenge_Text_TakeABreak
	release
	end

GymSwitch_EventScript_CallItADay::
	special GymChallenge_RetireStreak
	msgbox GymSwitch_Text_CalledItADay
	goto GymSwitch_EventScript_OakComes
	end

@ ---- Oak comes to the gym when a mastered streak ends ----
GymSwitch_EventScript_OakComes::
	closemessage
	clearflag FLAG_GYM_RUN_MASTERED
	textcolor NPC_TEXT_COLOR_MALE
	playbgm MUS_RG_OAK, 0
	setvar VAR_0x800A, LOCALID_PALLET_GYM_OAK
	special GymCandidate_PlanChallengerWalk   @ (to the challenger's spot, if he can)
	goto_if_eq VAR_RESULT, FALSE, GymSwitch_EventScript_OakAppears
	setobjectxyperm LOCALID_PALLET_GYM_OAK, VAR_0x8005, VAR_0x8006
	clearflag FLAG_HIDE_PALLET_GYM_OAK_AND_RIVAL
	addobject LOCALID_PALLET_GYM_OAK
	applymovement LOCALID_PALLET_GYM_OAK, gMentorWalkIn
	waitmovement 0
	special GymCandidate_FaceGuest
GymSwitch_EventScript_OakHere::
	special GymSwitch_BufferStatus
	msgbox GymSwitch_Text_OakMastered
	special GymSwitch_CanTakeNewType
	goto_if_eq VAR_RESULT, FALSE, GymSwitch_EventScript_OakNothingToOffer
	msgbox GymSwitch_Text_OakTryAnother, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, GymSwitch_EventScript_OakDeclined
	setvar VAR_0x8004, TRUE
	call GymSwitch_EventScript_ChooseType
	goto_if_eq VAR_RESULT, FALSE, GymSwitch_EventScript_OakDeclined
	goto_if_ne VAR_GYM_LEGEND_STATE, GYM_LEGEND_PENDING, GymSwitch_EventScript_OakLeaves
	closemessage
	call GymLegend_EventScript_Arrives   @ the new type's legendary
	closemessage
	fadescreen FADE_TO_BLACK            @ (Oak slips out: it may be in his way)
	removeobject LOCALID_PALLET_GYM_OAK
	setflag FLAG_HIDE_PALLET_GYM_OAK_AND_RIVAL
	special GymCandidate_FaceBack
	fadescreen FADE_FROM_BLACK
	fadedefaultbgm
	release
	end

GymSwitch_EventScript_OakAppears::
	fadescreen FADE_TO_BLACK
	special GymCandidate_PlaceGuest
	goto_if_eq VAR_RESULT, FALSE, GymSwitch_EventScript_OakNoRoom
	setobjectxyperm LOCALID_PALLET_GYM_OAK, VAR_0x8005, VAR_0x8006
	clearflag FLAG_HIDE_PALLET_GYM_OAK_AND_RIVAL
	addobject LOCALID_PALLET_GYM_OAK
	special GymCandidate_FaceGuest
GymSwitch_EventScript_OakNoRoom::
	fadescreen FADE_FROM_BLACK
	goto GymSwitch_EventScript_OakHere
	end

GymSwitch_EventScript_OakNothingToOffer::
	msgbox GymSwitch_Text_OakKeepItUp
	goto GymSwitch_EventScript_OakLeaves
	end

GymSwitch_EventScript_OakDeclined::
	msgbox GymSwitch_Text_OakOfferStands
GymSwitch_EventScript_OakLeaves::
	closemessage
	special GymCandidate_VisitorWalked
	goto_if_eq VAR_RESULT, FALSE, GymSwitch_EventScript_OakGone
	applymovement LOCALID_PALLET_GYM_OAK, gMentorWalkOut
	waitmovement 0
	playse SE_EXIT
GymSwitch_EventScript_OakGone::
	removeobject LOCALID_PALLET_GYM_OAK
	setflag FLAG_HIDE_PALLET_GYM_OAK_AND_RIVAL
	special GymCandidate_FaceBack
	fadedefaultbgm
	release
	end

@ ---- Oak in his lab (called first thing when the player talks to him) ----
GymSwitch_EventScript_LabOak::
	goto_if_unset FLAG_PALLET_GYM_AWARDED, GymSwitch_EventScript_Return
	special GymSwitch_BufferStatus
	goto_if_eq VAR_RESULT, FALSE, GymSwitch_EventScript_LabGoal
	special GymSwitch_StreakInProgress
	goto_if_eq VAR_RESULT, TRUE, GymSwitch_EventScript_LabMidStreak
	special GymSwitch_CanTakeNewType
	goto_if_eq VAR_RESULT, TRUE, GymSwitch_EventScript_LabOffer
	msgbox GymSwitch_Text_LabSwitchBack, MSGBOX_YESNO
	goto GymSwitch_EventScript_LabAnswer
	end

GymSwitch_EventScript_LabOffer::
	msgbox GymSwitch_Text_LabOffer, MSGBOX_YESNO
GymSwitch_EventScript_LabAnswer::
	goto_if_eq VAR_RESULT, NO, GymSwitch_EventScript_LabNo
	special GymSwitch_OnlyNewTypes
	call GymSwitch_EventScript_ChooseType
	goto_if_eq VAR_RESULT, FALSE, GymSwitch_EventScript_LabNo
	goto_if_eq VAR_GYM_LEGEND_STATE, GYM_LEGEND_PENDING, GymSwitch_EventScript_LabLegendHint
	release
	end

GymSwitch_EventScript_LabLegendHint::
	msgbox GymSwitch_Text_LabLegendHint
	release
	end

GymSwitch_EventScript_LabNo::
	msgbox GymSwitch_Text_LabNo
	release
	end

@ A streak is going: the gym's type can't change until it ends (then his usual lines)
GymSwitch_EventScript_LabMidStreak::
	msgbox GymSwitch_Text_LabMidStreak
	goto_if_unset FLAG_GYM_RUN_MASTERED, GymSwitch_EventScript_Return
	msgbox GymSwitch_Text_LabMidStreakMastered
	return

@ Nothing earned yet: what it takes (then his usual lines)
GymSwitch_EventScript_LabGoal::
	msgbox GymSwitch_Text_LabGoal
GymSwitch_EventScript_Return::
	return

@ ---- The type menu. VAR_0x8004 = TRUE: only new types. VAR_RESULT = whether the gym changed ----
GymSwitch_EventScript_ChooseType::
	goto_if_eq VAR_0x8004, TRUE, GymSwitch_EventScript_AskNewType
	message GymSwitch_Text_WhichTypeStars
	goto GymSwitch_EventScript_TypeMenu
	end

GymSwitch_EventScript_AskNewType::
	message GymSwitch_Text_WhichType
GymSwitch_EventScript_TypeMenu::
	waitmessage
	special GymSwitch_PushTypes
	dynmultistack 0, 0, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, GymSwitch_EventScript_ChoiceCancelled
	special GymSwitch_BufferChoice
	copyvar VAR_0x8009, VAR_RESULT
	goto_if_eq VAR_RESULT, TRUE, GymSwitch_EventScript_ConfirmNew
	msgbox GymSwitch_Text_ConfirmOld, MSGBOX_YESNO
	goto GymSwitch_EventScript_Confirmed
	end

GymSwitch_EventScript_ConfirmNew::
	msgbox GymSwitch_Text_ConfirmNew, MSGBOX_YESNO
GymSwitch_EventScript_Confirmed::
	goto_if_eq VAR_RESULT, NO, GymSwitch_EventScript_ChooseType
	special GymSwitch_SetType
	special GymBadge_ShowIcon
	playfanfare MUS_OBTAIN_BADGE
	msgbox GymSwitch_Text_NowType
	waitfanfare
	special GymBadge_HideIcon
	call_if_eq VAR_0x8007, TRUE, GymSwitch_EventScript_OldLegendLeaves
	goto_if_eq VAR_0x8009, TRUE, GymSwitch_EventScript_NewTypeTips
	msgbox GymSwitch_Text_BackToType
	setvar VAR_RESULT, TRUE
	return

GymSwitch_EventScript_NewTypeTips::
	msgbox GymSwitch_Text_NewTypeTips
	setvar VAR_RESULT, TRUE
	return

GymSwitch_EventScript_OldLegendLeaves::
	msgbox GymSwitch_Text_OldLegendLeaves
	return

GymSwitch_EventScript_ChoiceCancelled::
	setvar VAR_RESULT, FALSE
	return

@ ---- Text ----
GymSwitch_Text_AideGoal::
	.string "Your goal: {STR_VAR_1} wins in a row!\n"
	.string "PROF. OAK says he has something\l"
	.string "in mind for when you get there.$"

GymSwitch_Text_AnnounceMentor::
	.string "Wait… that's your mentor,\n"
	.string "{STR_VAR_1}!$"

GymSwitch_Text_Mastered::
	.string "{PLAYER} mastered {STR_VAR_1}-type\n"
	.string "POKéMON!$"

GymSwitch_Text_OakWillCome::
	.string "PROF. OAK will be thrilled!\n"
	.string "He'll come by when your streak\l"
	.string "is over.$"

GymSwitch_Text_WhatNext::
	.string "Your POKéMON are rested.\n"
	.string "What would you like to do?$"

GymSwitch_Text_KeepGoing::
	.string "KEEP GOING$"

GymSwitch_Text_CallItADay::
	.string "CALL IT A DAY$"

GymSwitch_Text_TakeABreak::
	.string "TAKE A BREAK$"

GymSwitch_Text_CalledItADay::
	.string "What a run! {STR_VAR_1} wins in a row,\n"
	.string "and nobody could beat you!\p"
	.string "I'll send the rest home.$"

GymSwitch_Text_OakMastered::
	.string "OAK: {PLAYER}! I heard all about\n"
	.string "it from your mentor!\p"
	.string "You've truly mastered {STR_VAR_3}-type\n"
	.string "POKéMON. At this rate, nobody will\l"
	.string "ever earn your badge again!$"

GymSwitch_Text_OakTryAnother::
	.string "So how about a new challenge?\n"
	.string "Your GYM could take a new type!\p"
	.string "You won't lose {STR_VAR_3}, either.\n"
	.string "Any type your GYM has had, you can\l"
	.string "switch back to at my LAB, free.\p"
	.string "Would you like to try another type?$"

GymSwitch_Text_OakKeepItUp::
	.string "Splendid work, {PLAYER}!\n"
	.string "Keep it up!$"

GymSwitch_Text_OakOfferStands::
	.string "OAK: I see! Well, you've earned it,\n"
	.string "so the offer stands.\p"
	.string "If you change your mind, come see\n"
	.string "me at my LAB!$"

GymSwitch_Text_LabGoal::
	.string "OAK: How's your GYM, {PLAYER}?\p"
	.string "Win {STR_VAR_1} in a row there, and\n"
	.string "you'll have mastered {STR_VAR_2} type.\p"
	.string "Then I'll have a new challenge\n"
	.string "for you!$"

GymSwitch_Text_LabOffer::
	.string "OAK: Ah, {PLAYER}! Have you changed\n"
	.string "your mind?\p"
	.string "You've earned a new type for your\n"
	.string "GYM. Would you like to choose it?$"

GymSwitch_Text_LabSwitchBack::
	.string "OAK: Ah, {PLAYER}! Would you like\n"
	.string "your GYM to go back to a type\l"
	.string "it's had before? It's free!$"

GymSwitch_Text_LabMidStreak::
	.string "OAK: Ah, {PLAYER}! {STR_VAR_1} wins in a\n"
	.string "row and counting, I hear!\p"
	.string "Let's talk about your GYM's type\n"
	.string "once your streak is over.$"

GymSwitch_Text_LabMidStreakMastered::
	.string "Win or lose, I'll come by your\n"
	.string "GYM when it ends!$"

GymSwitch_Text_LabNo::
	.string "OAK: Whenever you're ready!$"

GymSwitch_Text_LabLegendHint::
	.string "OAK: Hm… I have a feeling something\n"
	.string "is waiting for you at your GYM.\p"
	.string "Go and see!$"

GymSwitch_Text_WhichType::
	.string "Which type will your GYM be?$"

GymSwitch_Text_WhichTypeStars::
	.string "Which type will your GYM be?\n"
	.string "({STAR}: it's had it before. Free!)$"

GymSwitch_Text_ConfirmNew::
	.string "A {STR_VAR_1}-type GYM!\n"
	.string "Is that your choice?$"

GymSwitch_Text_ConfirmOld::
	.string "Switch your GYM back to\n"
	.string "{STR_VAR_1} type?$"

GymSwitch_Text_NowType::
	.string "{PLAYER}'s GYM is now a\n"
	.string "{STR_VAR_1}-type GYM!\p"
	.string "It awards the {STR_VAR_2}!$"

GymSwitch_Text_OldLegendLeaves::
	.string "OAK: Hm? {STR_VAR_3} left your GYM…\p"
	.string "I bet it's gone somewhere only\n"
	.string "the toughest TRAINERS go…$"

GymSwitch_Text_NewTypeTips::
	.string "OAK: Challengers will expect\n"
	.string "{STR_VAR_1}-type POKéMON now, so\l"
	.string "you'll need a new team!\p"
	.string "And remember: you can switch back\n"
	.string "to any type your GYM has had.\p"
	.string "It's free! Just come see me at\n"
	.string "my LAB.$"

GymSwitch_Text_BackToType::
	.string "OAK: Back to {STR_VAR_1} type!\n"
	.string "Challengers, beware!$"
