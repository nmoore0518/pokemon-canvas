@ Pokémon Canvas: the Leader Academy, north of Route 11 (src/leader_academy.c; maps from
@ tools/gym_hack/build_academy.py; lines from tools/gym_hack/academy_text.py)

@ ---------------------------------------------------------------------------
@ Route 11: the invitation and the escort (VAR_ACADEMY_STATE)
@ ---------------------------------------------------------------------------
Academy_EventScript_Route11Transition::
	setflag FLAG_HIDE_ACADEMY_MENTOR
	goto_if_eq VAR_ACADEMY_STATE, ACADEMY_ESCORT, Academy_EventScript_Route11ShowMentor
	specialvar VAR_RESULT, Academy_InvitationDue
	call_if_eq VAR_RESULT, TRUE, Academy_EventScript_SetInvite
	return

Academy_EventScript_SetInvite::
	setvar VAR_ACADEMY_STATE, ACADEMY_INVITE
	return

Academy_EventScript_Route11ShowMentor::
	special Academy_PrepareMentor
	clearflag FLAG_HIDE_ACADEMY_MENTOR
	return

Academy_EventScript_Invite::
	lockall
	special Academy_PrepareMentor
	playse SE_PIN
	applymovement LOCALID_PLAYER, Common_Movement_ExclamationMark
	waitmovement 0
	msgbox Academy_Text_InviteCall
	closemessage
	fadescreen FADE_TO_BLACK
	setvar VAR_ACADEMY_STATE, ACADEMY_ESCORT
	warpsilent MAP_ROUTE11, 38, 4
	waitstate
	releaseall
	end

Academy_EventScript_Escort::
	lockall
	special Academy_PrepareMentor
	applymovement LOCALID_PLAYER, Common_Movement_FaceRight
	applymovement LOCALID_ROUTE11_ACADEMY_MENTOR, Common_Movement_FaceLeft
	waitmovement 0
	msgbox Academy_Text_Escort1
	closemessage
	applymovement LOCALID_ROUTE11_ACADEMY_MENTOR, Academy_Movement_WalkUp2
	applymovement LOCALID_PLAYER, Academy_Movement_WalkUp2
	waitmovement 0
	msgbox Academy_Text_Guard1
	closemessage
	fadescreen FADE_TO_BLACK
	setflag FLAG_HIDE_ACADEMY_GATE
	setvar VAR_ACADEMY_STATE, ACADEMY_WELCOME
	warpsilent MAP_LEADER_ACADEMY_CAMPUS, 21, 37
	waitstate
	releaseall
	end

Academy_EventScript_GateGuard::
	msgbox Academy_Text_GateGuard, MSGBOX_NPC
	end

Academy_Movement_WalkUp2:
	walk_up
	walk_up
	step_end

Academy_Movement_WalkDown2:
	walk_down
	walk_down
	step_end

Academy_Movement_WalkUp3:
	walk_up
	walk_up
	walk_up
	step_end

Academy_Movement_WalkDown3:
	walk_down
	walk_down
	walk_down
	step_end

@ ---------------------------------------------------------------------------
@ The campus: the headmaster's welcome
@ ---------------------------------------------------------------------------
Academy_EventScript_CampusTransition::
	setflag FLAG_HIDE_ACADEMY_MENTOR
	setflag FLAG_HIDE_ACADEMY_WELCOME
	goto_if_ne VAR_ACADEMY_STATE, ACADEMY_WELCOME, Common_EventScript_NopReturn
	special Academy_PrepareMentor
	clearflag FLAG_HIDE_ACADEMY_MENTOR
	clearflag FLAG_HIDE_ACADEMY_WELCOME
	return

Academy_EventScript_Welcome::
	lockall
	special Academy_PrepareMentor
	applymovement LOCALID_ACADEMY_WELCOME_HEADMASTER, Academy_Movement_WalkDown3
	waitmovement 0
	msgbox Academy_Text_Welcome1
	msgbox Academy_Text_Welcome2
	msgbox Academy_Text_Welcome3
	msgbox Academy_Text_Welcome4
	closemessage
	applymovement LOCALID_ACADEMY_WELCOME_MENTOR, Common_Movement_FaceRight
	applymovement LOCALID_PLAYER, Common_Movement_FaceLeft
	waitmovement 0
	special Academy_PrepareMentor
	msgbox Academy_Text_MentorBye
	closemessage
	applymovement LOCALID_ACADEMY_WELCOME_MENTOR, Academy_Movement_WalkDown2
	waitmovement 0
	removeobject LOCALID_ACADEMY_WELCOME_MENTOR
	applymovement LOCALID_PLAYER, Common_Movement_FaceUp
	applymovement LOCALID_ACADEMY_WELCOME_HEADMASTER, Academy_Movement_WalkUp3
	waitmovement 0
	removeobject LOCALID_ACADEMY_WELCOME_HEADMASTER
	setflag FLAG_HIDE_ACADEMY_MENTOR
	setflag FLAG_HIDE_ACADEMY_WELCOME
	setvar VAR_ACADEMY_STATE, ACADEMY_ADMITTED
	releaseall
	end

Academy_EventScript_Nothing::
	end

@ ---- Signs ----
Academy_EventScript_HallSign::
	msgbox Academy_Text_HallSign, MSGBOX_SIGN
	end
Academy_EventScript_CenterSign::
	msgbox Academy_Text_CenterSign, MSGBOX_SIGN
	end
Academy_EventScript_SupplySign::
	msgbox Academy_Text_SupplySign, MSGBOX_SIGN
	end
Academy_EventScript_BattleSign::
	msgbox Academy_Text_BattleSign, MSGBOX_SIGN
	end
Academy_EventScript_ExchangeSign::
	msgbox Academy_Text_ExchangeSign, MSGBOX_SIGN
	end
Academy_EventScript_GateSign::
	msgbox Academy_Text_GateSign, MSGBOX_SIGN
	end

@ ---- Students ----
Academy_EventScript_Student1::
	msgbox Academy_Text_Student1, MSGBOX_NPC
	end
Academy_EventScript_Student2::
	msgbox Academy_Text_Student2, MSGBOX_NPC
	end
Academy_EventScript_Statue::
	msgbox Academy_Text_Statue, MSGBOX_SIGN
	end
Academy_EventScript_Reader1::
	msgbox Academy_Text_Reader1, MSGBOX_NPC
	end
Academy_EventScript_Reader2::
	msgbox Academy_Text_Reader2, MSGBOX_NPC
	end
Academy_EventScript_LectureAStudent::
	msgbox Academy_Text_LectureAStudent, MSGBOX_NPC
	end
Academy_EventScript_LectureBStudent::
	msgbox Academy_Text_LectureBStudent, MSGBOX_NPC
	end
Academy_EventScript_LectureCStudent::
	msgbox Academy_Text_LectureCStudent, MSGBOX_NPC
	end
Academy_EventScript_LectureDStudent::
	msgbox Academy_Text_LectureDStudent, MSGBOX_NPC
	end
Academy_EventScript_Student3::
	msgbox Academy_Text_Student3, MSGBOX_NPC
	end

@ ---------------------------------------------------------------------------
@ The headmaster, in the hall
@ ---------------------------------------------------------------------------
Academy_EventScript_Headmaster::
	lock
	faceplayer
Academy_EventScript_HeadmasterMenu::
	special Academy_BufferBP
	message Academy_Text_HeadmasterHello
	waitmessage
	dynmultichoice 0, 0, FALSE, 5, 0, DYN_MULTICHOICE_CB_NONE, Academy_Text_OptBP, Academy_Text_OptHall, Academy_Text_OptCandidates, Academy_Text_OptTeachers, Academy_Text_OptBye
	switch VAR_RESULT
	case 0, Academy_EventScript_HeadmasterBP
	case 1, Academy_EventScript_HeadmasterHall
	case 2, Academy_EventScript_HeadmasterCandidates
	case 3, Academy_EventScript_HeadmasterTeachers
	msgbox Academy_Text_HeadmasterBye
	release
	end

Academy_EventScript_HeadmasterBP::
	msgbox Academy_Text_HeadmasterBP
	goto Academy_EventScript_HeadmasterMenu
Academy_EventScript_HeadmasterHall::
	msgbox Academy_Text_HeadmasterHall
	goto Academy_EventScript_HeadmasterMenu
Academy_EventScript_HeadmasterCandidates::
	msgbox Academy_Text_HeadmasterCandidates
	goto Academy_EventScript_HeadmasterMenu
Academy_EventScript_HeadmasterTeachers::
	msgbox Academy_Text_HeadmasterTeachers
	goto Academy_EventScript_HeadmasterMenu

@ ---------------------------------------------------------------------------
@ Battles: TRAINER_GYM_CHALLENGER, set up by a special. No white-out. VAR_RESULT = the outcome
@ ---------------------------------------------------------------------------
Academy_EventScript_Battle::
	cleartrainerflag TRAINER_GYM_CHALLENGER
	setflag FLAG_GYM_CHALLENGE_BATTLE
	trainerbattle_no_intro TRAINER_GYM_CHALLENGER, gGymChallengeDefeatText
	clearflag FLAG_GYM_CHALLENGE_BATTLE
	special GymChallenge_EndPractice
	specialvar VAR_RESULT, GetBattleOutcome
	return

@ VAR_0x8004 BP more, announced
Academy_EventScript_EarnBP::
	special Academy_AddBP
	playfanfare MUS_LEVEL_UP
	message Academy_Text_EarnedBP
	waitmessage
	waitfanfare
	waitbuttonpress
	return

@ ---------------------------------------------------------------------------
@ Candidates: practice battles with the player's own team, as often as they like
@ ---------------------------------------------------------------------------
	.macro academy_candidate name:req, index:req
	.global Academy_EventScript_\name
Academy_EventScript_\name\():
	lock
	faceplayer
	msgbox Academy_Text_\name\()Hello
	setvar VAR_0x8004, \index
	goto Academy_EventScript_CandidateBattle
	.endm

	academy_candidate Koji, 0
	academy_candidate Maren, 1
	academy_candidate Elias, 2
	academy_candidate Nadia, 3
	academy_candidate Rene, 4
	academy_candidate Keola, 5
	academy_candidate Briony, 6
	academy_candidate Teo, 7

Academy_EventScript_CandidateBattle::
	special Academy_SetupCandidate
	msgbox Academy_Text_CandidateAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Academy_EventScript_CandidateLater
	copyvar VAR_0x8008, VAR_0x8005
	special HealPlayerParty
	msgbox gGymChallengeIntroText
	closemessage
	call Academy_EventScript_Battle
	copyvar VAR_0x8009, VAR_RESULT
	special HealPlayerParty
	goto_if_ne VAR_0x8009, B_OUTCOME_WON, Academy_EventScript_CandidateBeatPlayer
	copyvar VAR_0x8004, VAR_0x8008
	call Academy_EventScript_EarnBP
	msgbox Academy_Text_CandidateHealed
	release
	end

Academy_EventScript_CandidateBeatPlayer::
	msgbox gGymChallengeWinText
	msgbox Academy_Text_CandidateHealed
	release
	end

Academy_EventScript_CandidateLater::
	msgbox Academy_Text_CandidateLater
	release
	end

@ ---------------------------------------------------------------------------
@ Rentals: the player's party set aside, three chosen from six offered.
@ VAR_RESULT = MULTI_B_PRESSED if they backed out (their party is back already)
@ ---------------------------------------------------------------------------
Academy_EventScript_PickRentals::
	special Academy_RentalBegin
	setvar VAR_0x8007, 1
Academy_EventScript_PickRentalLoop::
	buffernumberstring STR_VAR_1, VAR_0x8007
	message Academy_Text_RentalPick
	waitmessage
	special Academy_PushRentals
	dynmultistack 0, 0, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, Academy_EventScript_PickRentalCancel
	copyvar VAR_0x8004, VAR_RESULT
	special Academy_BufferRental
	msgbox Academy_Text_RentalInfo, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Academy_EventScript_PickRentalLoop
	special Academy_TakeRental
	addvar VAR_0x8007, 1
	goto_if_lt VAR_RESULT, 3, Academy_EventScript_PickRentalLoop
	setvar VAR_RESULT, 0
	return

Academy_EventScript_PickRentalCancel::
	special Academy_RentalEnd
	setvar VAR_RESULT, MULTI_B_PRESSED
	return

@ ---- The Battle Hall ----
Academy_EventScript_RentalClerk::
	lock
	faceplayer
	msgbox Academy_Text_RentalClerk
	msgbox Academy_Text_RentalAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Academy_EventScript_HallBye
	call Academy_EventScript_PickRentals
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, Academy_EventScript_HallBye
Academy_EventScript_HallNext::
	special Academy_SetupHallBattle
	msgbox Academy_Text_HallNext
	msgbox gGymChallengeIntroText
	closemessage
	call Academy_EventScript_Battle
	goto_if_ne VAR_RESULT, B_OUTCOME_WON, Academy_EventScript_HallLost
	special Academy_HallWin
	msgbox Academy_Text_HallWon
	call Academy_EventScript_EarnBP
	special HealPlayerParty
	msgbox Academy_Text_HallContinue, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, Academy_EventScript_HallNext
	special Academy_RentalEnd
	msgbox Academy_Text_RentalReturn
	goto Academy_EventScript_HallBye

Academy_EventScript_HallLost::
	special Academy_RentalEnd
	msgbox gGymChallengeWinText
	msgbox Academy_Text_HallLost
	msgbox Academy_Text_RentalReturn
Academy_EventScript_HallBye::
	msgbox Academy_Text_HallBye
	release
	end

@ ---------------------------------------------------------------------------
@ The instructors: a lecture (first time: straight away), then a demonstration (rentals)
@ ---------------------------------------------------------------------------
	.macro academy_teacher name:req, index:req
	.global Academy_EventScript_\name
Academy_EventScript_\name\():
	lock
	faceplayer
	setvar VAR_0x8008, \index
	goto_if_set FLAG_ACADEMY_LECTURE_FIRST + \index, Academy_EventScript_\name\()Menu
	call Academy_EventScript_\name\()Lecture
	setflag FLAG_ACADEMY_LECTURE_FIRST + \index
Academy_EventScript_\name\()Menu:
	message Academy_Text_TeacherMenu
	waitmessage
	dynmultichoice 0, 0, FALSE, 3, 0, DYN_MULTICHOICE_CB_NONE, Academy_Text_OptDemo, Academy_Text_OptLecture, Academy_Text_OptBye
	goto_if_eq VAR_RESULT, 0, Academy_EventScript_Demo
	goto_if_ne VAR_RESULT, 1, Academy_EventScript_TeacherBye
	call Academy_EventScript_\name\()Lecture
	goto Academy_EventScript_\name\()Menu
Academy_EventScript_\name\()Lecture:
	msgbox Academy_Text_\name\()Lecture0
	msgbox Academy_Text_\name\()Lecture1
	msgbox Academy_Text_\name\()Lecture2
	msgbox Academy_Text_\name\()Lecture3
	return
	.endm

	academy_teacher Larkspur, 0
	academy_teacher Camellia, 1
	academy_teacher Foxglove, 2
	academy_teacher Verbena, 3
	academy_teacher Marigold, 4
	academy_teacher Hyacinth, 5
	academy_teacher Yarrow, 6

@ Tansy also keeps the team builder
Academy_EventScript_Tansy::
	lock
	faceplayer
	setvar VAR_0x8008, 7
	goto_if_set FLAG_ACADEMY_LECTURE_FIRST + 7, Academy_EventScript_TansyMenu
	call Academy_EventScript_TansyLecture
	setflag FLAG_ACADEMY_LECTURE_FIRST + 7
Academy_EventScript_TansyMenu::
	message Academy_Text_TeacherMenu
	waitmessage
	dynmultichoice 0, 0, FALSE, 4, 0, DYN_MULTICHOICE_CB_NONE, Academy_Text_OptBuilder, Academy_Text_OptDemo, Academy_Text_OptLecture, Academy_Text_OptBye
	goto_if_eq VAR_RESULT, 0, Academy_EventScript_TeamBuilder
	goto_if_eq VAR_RESULT, 1, Academy_EventScript_Demo
	goto_if_ne VAR_RESULT, 2, Academy_EventScript_TeacherBye
	call Academy_EventScript_TansyLecture
	goto Academy_EventScript_TansyMenu
Academy_EventScript_TansyLecture::
	msgbox Academy_Text_TansyLecture0
	msgbox Academy_Text_TansyLecture1
	msgbox Academy_Text_TansyLecture2
	msgbox Academy_Text_TansyLecture3
	return

Academy_EventScript_TeamBuilder::
	closemessage
	special Academy_OpenTeamBuilder
	release
	end

Academy_EventScript_TeacherBye::
	release
	end

@ VAR_0x8008 = the instructor
Academy_EventScript_Demo::
	copyvar VAR_0x8004, VAR_0x8008
	special Academy_SetupTeacher
	msgbox Academy_Text_DemoIntro
	call Academy_EventScript_PickRentals
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, Academy_EventScript_TeacherBye
	copyvar VAR_0x8004, VAR_0x8008
	special Academy_SetupTeacher
	msgbox gGymChallengeIntroText
	closemessage
	call Academy_EventScript_Battle
	copyvar VAR_0x8009, VAR_RESULT
	special Academy_RentalEnd
	copyvar VAR_0x8004, VAR_0x8008
	special Academy_SetupTeacher
	goto_if_ne VAR_0x8009, B_OUTCOME_WON, Academy_EventScript_DemoLost
	msgbox Academy_Text_DemoWon
	setvar VAR_0x8004, 3
	call Academy_EventScript_EarnBP
	msgbox Academy_Text_RentalReturn
	release
	end

Academy_EventScript_DemoLost::
	msgbox Academy_Text_DemoLost
	msgbox Academy_Text_RentalReturn
	release
	end

@ ---------------------------------------------------------------------------
@ Shops: BP and money, side by side
@ ---------------------------------------------------------------------------
Academy_EventScript_Nurse::
	lock
	faceplayer
	call EventScript_PkmnCenterNurse_Frlg
	release
	end

Academy_EventScript_SupplyClerk::
	lock
	faceplayer
	message Academy_Text_Supply
	waitmessage
	pokemart Academy_SupplyItems
	msgbox gText_PleaseComeAgain
	release
	end

Academy_EventScript_ExchangeMoney::
	lock
	faceplayer
	msgbox Academy_Text_ExchangeMoney
	message gText_HowMayIServeYou
	waitmessage
	pokemart Academy_ExchangeItems
	msgbox gText_PleaseComeAgain
	release
	end

Academy_EventScript_ExchangeBP::
	lock
	faceplayer
	special Academy_BufferBP
	message Academy_Text_ExchangeBP
	waitmessage
	goto Academy_EventScript_ExchangeBPList
Academy_EventScript_ExchangeBPLoop::
	special Academy_BufferBP
	message Academy_Text_ExchangeBPMore
	waitmessage
Academy_EventScript_ExchangeBPList::
	special Academy_PushExchange
	dynmultistack 0, 0, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, Academy_EventScript_ExchangeBye
	copyvar VAR_0x8004, VAR_RESULT
	special Academy_BufferExchangeItem
	msgbox Academy_Text_ExchangeConfirm, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Academy_EventScript_ExchangeBPLoop
	checkitemspace VAR_0x8005
	goto_if_eq VAR_RESULT, FALSE, Academy_EventScript_ExchangeNoRoom
	special Academy_SpendBP
	goto_if_eq VAR_RESULT, FALSE, Academy_EventScript_ExchangeNoBP
	giveitem VAR_0x8005
	goto Academy_EventScript_ExchangeBPLoop

Academy_EventScript_ExchangeNoRoom::
	msgbox Academy_Text_ExchangeNoRoom
	goto Academy_EventScript_ExchangeBPLoop
Academy_EventScript_ExchangeNoBP::
	msgbox Academy_Text_ExchangeNoBP
	goto Academy_EventScript_ExchangeBPLoop
Academy_EventScript_ExchangeBye::
	msgbox Academy_Text_ExchangeBye
	release
	end

	.align 2
Academy_SupplyItems::
	.2byte ITEM_BOTTLE_CAP
	.2byte ITEM_GOLD_BOTTLE_CAP
	.2byte ITEM_ABILITY_CAPSULE
	.2byte ITEM_ABILITY_PATCH
	.2byte ITEM_CILANTRO
	.2byte ITEM_MULTIVITAMIN
	.2byte ITEM_ADAMANT_MINT
	.2byte ITEM_JOLLY_MINT
	.2byte ITEM_MODEST_MINT
	.2byte ITEM_TIMID_MINT
	.2byte ITEM_BOLD_MINT
	.2byte ITEM_IMPISH_MINT
	.2byte ITEM_CALM_MINT
	.2byte ITEM_CAREFUL_MINT
	.2byte ITEM_LONELY_MINT
	.2byte ITEM_SERIOUS_MINT
	.2byte ITEM_HP_UP
	.2byte ITEM_PROTEIN
	.2byte ITEM_IRON
	.2byte ITEM_CALCIUM
	.2byte ITEM_ZINC
	.2byte ITEM_CARBOS
	.2byte ITEM_POWER_WEIGHT
	.2byte ITEM_POWER_BRACER
	.2byte ITEM_POWER_BELT
	.2byte ITEM_POWER_LENS
	.2byte ITEM_POWER_BAND
	.2byte ITEM_POWER_ANKLET
	.2byte ITEM_MACHO_BRACE
	.2byte ITEM_POMEG_BERRY
	.2byte ITEM_KELPSY_BERRY
	.2byte ITEM_QUALOT_BERRY
	.2byte ITEM_HONDEW_BERRY
	.2byte ITEM_GREPA_BERRY
	.2byte ITEM_TAMATO_BERRY
	.2byte ITEM_NONE

	.align 2
Academy_ExchangeItems::
	.2byte ITEM_CHOICE_SCARF
	.2byte ITEM_CHOICE_BAND
	.2byte ITEM_CHOICE_SPECS
	.2byte ITEM_LIFE_ORB
	.2byte ITEM_FOCUS_SASH
	.2byte ITEM_ASSAULT_VEST
	.2byte ITEM_EVIOLITE
	.2byte ITEM_BOOSTER_ENERGY
	.2byte ITEM_LOADED_DICE
	.2byte ITEM_WEAKNESS_POLICY
	.2byte ITEM_ROCKY_HELMET
	.2byte ITEM_HEAVY_DUTY_BOOTS
	.2byte ITEM_LEFTOVERS
	.2byte ITEM_EXPERT_BELT
	.2byte ITEM_COVERT_CLOAK
	.2byte ITEM_CLEAR_AMULET
	.2byte ITEM_BLACK_SLUDGE
	.2byte ITEM_LIGHT_CLAY
	.2byte ITEM_AIR_BALLOON
	.2byte ITEM_RED_CARD
	.2byte ITEM_EJECT_BUTTON
	.2byte ITEM_EJECT_PACK
	.2byte ITEM_WHITE_HERB
	.2byte ITEM_MENTAL_HERB
	.2byte ITEM_POWER_HERB
	.2byte ITEM_TOXIC_ORB
	.2byte ITEM_FLAME_ORB
	.2byte ITEM_DAMP_ROCK
	.2byte ITEM_HEAT_ROCK
	.2byte ITEM_SMOOTH_ROCK
	.2byte ITEM_ICY_ROCK
	.2byte ITEM_LUM_BERRY
	.2byte ITEM_SITRUS_BERRY
	.2byte ITEM_NONE

	.include "data/scripts/leader_academy_text.inc"
