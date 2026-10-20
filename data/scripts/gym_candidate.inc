@ Gym Trainer hack: the player as a candidate for the Pallet Town Gym (src/gym_candidate.c).
@ After each badge the leader asks the player to hold their gym against one or two challengers
@ ("Guest Leader" battles) for their approval; the League's door needs all eight. Mentors of the
@ player's type react to them. Trainers on the road ask to join the player's gym.

@ ---- Per gym: VAR_0x8009 = the leader, VAR_0x800A = the gym's spare person for challengers ----
GymCandidate_EventScript_PewterCityAfterBadge::
	setvar VAR_0x8009, 0
	setvar VAR_0x800A, LOCALID_PEWTER_GYM_GUEST
	goto GymCandidate_EventScript_AfterBadge
	end

GymCandidate_EventScript_PewterCityTalk::
	setvar VAR_0x8009, 0
	setvar VAR_0x800A, LOCALID_PEWTER_GYM_GUEST
	goto GymCandidate_EventScript_TalkAgain

GymCandidate_EventScript_CeruleanCityAfterBadge::
	setvar VAR_0x8009, 1
	setvar VAR_0x800A, LOCALID_CERULEAN_GYM_GUEST
	goto GymCandidate_EventScript_AfterBadge
	end

GymCandidate_EventScript_CeruleanCityTalk::
	setvar VAR_0x8009, 1
	setvar VAR_0x800A, LOCALID_CERULEAN_GYM_GUEST
	goto GymCandidate_EventScript_TalkAgain

GymCandidate_EventScript_VermilionCityAfterBadge::
	setvar VAR_0x8009, 2
	setvar VAR_0x800A, LOCALID_VERMILION_GYM_GUEST
	goto GymCandidate_EventScript_AfterBadge
	end

GymCandidate_EventScript_VermilionCityTalk::
	setvar VAR_0x8009, 2
	setvar VAR_0x800A, LOCALID_VERMILION_GYM_GUEST
	goto GymCandidate_EventScript_TalkAgain

GymCandidate_EventScript_CeladonCityAfterBadge::
	setvar VAR_0x8009, 3
	setvar VAR_0x800A, LOCALID_CELADON_GYM_GUEST
	goto GymCandidate_EventScript_AfterBadge
	end

GymCandidate_EventScript_CeladonCityTalk::
	setvar VAR_0x8009, 3
	setvar VAR_0x800A, LOCALID_CELADON_GYM_GUEST
	goto GymCandidate_EventScript_TalkAgain

GymCandidate_EventScript_FuchsiaCityAfterBadge::
	setvar VAR_0x8009, 4
	setvar VAR_0x800A, LOCALID_FUCHSIA_GYM_GUEST
	goto GymCandidate_EventScript_AfterBadge
	end

GymCandidate_EventScript_FuchsiaCityTalk::
	setvar VAR_0x8009, 4
	setvar VAR_0x800A, LOCALID_FUCHSIA_GYM_GUEST
	goto GymCandidate_EventScript_TalkAgain

GymCandidate_EventScript_SaffronCityAfterBadge::
	setvar VAR_0x8009, 5
	setvar VAR_0x800A, LOCALID_SAFFRON_GYM_GUEST
	goto GymCandidate_EventScript_AfterBadge
	end

GymCandidate_EventScript_SaffronCityTalk::
	setvar VAR_0x8009, 5
	setvar VAR_0x800A, LOCALID_SAFFRON_GYM_GUEST
	goto GymCandidate_EventScript_TalkAgain

GymCandidate_EventScript_CinnabarIslandAfterBadge::
	setvar VAR_0x8009, 6
	setvar VAR_0x800A, LOCALID_CINNABAR_GYM_GUEST
	goto GymCandidate_EventScript_AfterBadge
	end

GymCandidate_EventScript_CinnabarIslandTalk::
	setvar VAR_0x8009, 6
	setvar VAR_0x800A, LOCALID_CINNABAR_GYM_GUEST
	goto GymCandidate_EventScript_TalkAgain

GymCandidate_EventScript_ViridianCityAfterBadge::
	setvar VAR_0x8009, 7
	setvar VAR_0x800A, LOCALID_VIRIDIAN_GYM_GUEST
	goto GymCandidate_EventScript_AfterBadge
	end

GymCandidate_EventScript_ViridianCityTalk::
	setvar VAR_0x8009, 7
	setvar VAR_0x800A, LOCALID_VIRIDIAN_GYM_GUEST
	goto GymCandidate_EventScript_TalkAgain

@ Right after the badge (and its gifts): the mentor's reaction, then the request. Ends the script.
GymCandidate_EventScript_AfterBadge::
	special GymCandidate_BufferMentorFirst
	call_if_eq VAR_RESULT, TRUE, GymCandidate_EventScript_ShowText4
	call GymCandidate_EventScript_MentorGift
	goto GymCandidate_EventScript_Offer
	end

@ Talking to the leader again: asks again until they approve. After that they offer optional
@ stand-in shifts (one challenger at the player's level, for money), until the player has their
@ own gym. Ends the script.
GymCandidate_EventScript_TalkAgain::
	call GymCandidate_EventScript_MentorGift   @ (in case it was missed)
	special GymCandidate_IsApproved
	goto_if_eq VAR_RESULT, FALSE, GymCandidate_EventScript_Offer
	goto_if_eq VAR_0x8009, 7, GymCandidate_EventScript_Return   @ GIOVANNI: his farewell, then he leaves (FireRed's story)
	goto_if_set FLAG_PALLET_GYM_AWARDED, GymCandidate_EventScript_OwnGym
	special GymCandidate_BufferRepeatOffer
	msgbox gStringVar4, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, GymCandidate_EventScript_NotYet
	setvar VAR_0x8007, 1
	goto GymCandidate_EventScript_Start
	end

GymCandidate_EventScript_OwnGym::
	special GymCandidate_BufferOwnGym
	msgbox gStringVar4
	release
	end

GymCandidate_EventScript_ShowText4::
	msgbox gStringVar4
	return

GymCandidate_EventScript_Offer::
	special GymCandidate_BufferOffer
	msgbox gStringVar4, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, GymCandidate_EventScript_NotYet
	setvar VAR_0x8007, 0
GymCandidate_EventScript_Start::
	special GymHack_CheckTeamSpecial
	goto_if_ne VAR_RESULT, GYM_HACK_TEAM_OK, GymCandidate_EventScript_TeamBreaksRules
	setvar VAR_0x800B, 0
	call GymCandidate_EventScript_TakeLeadersPlace
GymCandidate_EventScript_NextGuest::
	special HealPlayerParty
	special GymCandidate_GenerateGuest
	call_if_eq VAR_0x800B, 0, GymCandidate_EventScript_LaterThatDay
	call_if_ne VAR_0x800B, 0, GymCandidate_EventScript_AnotherOne
	closemessage
	call GymCandidate_EventScript_ChallengerWalksIn
	msgbox gGymChallengeIntroText
	closemessage
	cleartrainerflag TRAINER_GYM_CHALLENGER
	setflag FLAG_GYM_CHALLENGE_BATTLE
	trainerbattle_no_intro TRAINER_GYM_CHALLENGER, gGymChallengeDefeatText
	clearflag FLAG_GYM_CHALLENGE_BATTLE
	special GymChallenge_EndPractice
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_WON, GymCandidate_EventScript_GuestWon
	special HealPlayerParty
	msgbox gGymChallengeAfterText
	call GymCandidate_EventScript_VisitorWalksOut
	addvar VAR_0x800B, 1
	special GymCandidate_MoreGuests
	goto_if_eq VAR_RESULT, TRUE, GymCandidate_EventScript_NextGuest
	call GymCandidate_EventScript_ReturnLeadersPlace
	goto_if_eq VAR_0x8007, 1, GymCandidate_EventScript_ShiftDone
	special GymCandidate_Approve
	playfanfare MUS_LEVEL_UP
	msgbox gStringVar4
	waitfanfare
	msgbox GymCandidate_Text_ApprovalCount
	release
	end

GymCandidate_EventScript_ShiftDone::
	special GymCandidate_BufferRepeatThanks
	msgbox gStringVar4
	release
	end

@ The player takes the leader's spot: the leader says where they'll be, then (in the dark) steps
@ aside to watch (or leaves, for a stand-in shift) while the player steps up
GymCandidate_EventScript_TakeLeadersPlace::
	special GymCandidate_PlanHold
	goto_if_eq VAR_RESULT, FALSE, GymCandidate_EventScript_Return
	special GymCandidate_BufferHoldText
	msgbox gStringVar4
	closemessage
	fadescreen FADE_TO_BLACK
	special GymCandidate_HoldLeaderAside
	applymovement LOCALID_PLAYER, gHoldPlayerIn
	waitmovement 0
	delay 30
	fadescreen FADE_FROM_BLACK
	return

@ ...and gives it back afterwards
GymCandidate_EventScript_ReturnLeadersPlace::
	special GymCandidate_IsHolding
	goto_if_eq VAR_RESULT, FALSE, GymCandidate_EventScript_Return
	closemessage
	fadescreen FADE_TO_BLACK
	applymovement LOCALID_PLAYER, gHoldPlayerOut
	waitmovement 0
	special GymCandidate_HoldLeaderBack
	fadescreen FADE_FROM_BLACK
	return

@ A challenger (VAR_0x800A) walks in from the door to the player, or appears beside them
GymCandidate_EventScript_ChallengerWalksIn::
	special GymCandidate_PlanChallengerWalk
	goto_if_eq VAR_RESULT, FALSE, GymCandidate_EventScript_ChallengerAppears
	setobjectxyperm VAR_0x800A, VAR_0x8005, VAR_0x8006
	clearflag FLAG_HIDE_PALLET_GYM_CHALLENGER
	addobject VAR_0x800A
	playse SE_EXIT
	applymovement VAR_0x800A, gMentorWalkIn
	waitmovement 0
	special GymCandidate_FaceGuest
	return

GymCandidate_EventScript_ChallengerAppears::
	fadescreen FADE_TO_BLACK
	special GymCandidate_PlaceGuest
	goto_if_eq VAR_RESULT, FALSE, GymCandidate_EventScript_GuestNoRoom
	setobjectxyperm VAR_0x800A, VAR_0x8005, VAR_0x8006
	clearflag FLAG_HIDE_PALLET_GYM_CHALLENGER
	addobject VAR_0x800A
	special GymCandidate_FaceGuest
GymCandidate_EventScript_GuestNoRoom::
	fadescreen FADE_FROM_BLACK
	return

GymCandidate_EventScript_LaterThatDay::
	msgbox GymCandidate_Text_LaterThatDay
	return

GymCandidate_EventScript_AnotherOne::
	msgbox GymCandidate_Text_AnotherOne
	return

GymCandidate_EventScript_GuestLeaves::
	closemessage
	fadescreen FADE_TO_BLACK
	removeobject VAR_0x800A
	setflag FLAG_HIDE_PALLET_GYM_CHALLENGER
	special GymCandidate_FaceBack
	fadescreen FADE_FROM_BLACK
	return

@ The challenger beat the player: the leader comes back
GymCandidate_EventScript_GuestWon::
	special HealPlayerParty
	msgbox gGymChallengeWinText
	call GymCandidate_EventScript_VisitorWalksOut
	call GymCandidate_EventScript_ReturnLeadersPlace
	msgbox GymCandidate_Text_TryAgain
	release
	end

GymCandidate_EventScript_NotYet::
	msgbox GymCandidate_Text_NotYet
	release
	end

GymCandidate_EventScript_TeamBreaksRules::
	msgbox GymCandidate_Text_TeamBreaksRules
	release
	end

@ ---- The mentor (src/gym_candidate.c) ----
@ In Oak's lab, right after the Pokédex: the mentor Oak wrote to comes to meet the player.
@ VAR_0x800A = the lab's object for them.
GymCandidate_EventScript_MentorIntro::
	special GymCandidate_PrepareMentorIntro
	goto_if_eq VAR_RESULT, FALSE, GymCandidate_EventScript_Return
	delay 30
	playse SE_DOOR
	msgbox GymCandidate_Text_MentorAtDoor
	closemessage
	call GymCandidate_EventScript_VisitorWalksIn
	playse SE_PIN
	applymovement LOCALID_OAKS_LAB_PROF_OAK, Common_Movement_ExclamationMark
	waitmovement 0
	special GymCandidate_BufferMentorName
	msgbox GymCandidate_Text_OakWelcomesMentor
	special GymCandidate_BufferMentorIntro
	msgbox gStringVar4
	special GymCandidate_BufferMentorStory
	msgbox gStringVar4
	special GymCandidate_BufferMentorQuestion
	msgbox gStringVar4, MSGBOX_YESNO
	special GymCandidate_BufferMentorAnswer
	msgbox gStringVar4
	special GymCandidate_GiveMentorDecor
	call_if_eq VAR_RESULT, TRUE, GymCandidate_EventScript_MentorDecorGift
	special GymCandidate_BufferMentorFarewell
	msgbox gStringVar4
	call GymCandidate_EventScript_VisitorWalksOut
	special GymCandidate_BufferMentorName
	msgbox GymCandidate_Text_OakAfterMentor
	return

GymCandidate_EventScript_MentorDecorGift::
	msgbox GymCandidate_Text_MentorDecorGift
	playfanfare MUS_OBTAIN_ITEM
	msgbox GymCandidate_Text_ReceivedMentorDecor
	waitfanfare
	return

@ VAR_0x800A = the visitor (looking like VAR_OBJ_GFX_ID_0): walks in through the door to the
@ player if there's a way, or appears beside them
GymCandidate_EventScript_VisitorWalksIn::
	special GymCandidate_PlanVisitorWalk
	goto_if_eq VAR_RESULT, FALSE, GymCandidate_EventScript_MentorAppears
	setobjectxyperm VAR_0x800A, VAR_0x8005, VAR_0x8006
	clearflag FLAG_HIDE_PALLET_GYM_CHALLENGER
	addobject VAR_0x800A
	applymovement VAR_0x800A, gMentorWalkIn
	waitmovement 0
	special GymCandidate_FaceGuest
	return

@ ...and leaves the way they came
GymCandidate_EventScript_VisitorWalksOut::
	special GymCandidate_VisitorWalked
	goto_if_eq VAR_RESULT, FALSE, GymCandidate_EventScript_GuestLeaves
	closemessage
	applymovement VAR_0x800A, gMentorWalkOut
	waitmovement 0
	removeobject VAR_0x800A
	setflag FLAG_HIDE_PALLET_GYM_CHALLENGER
	playse SE_EXIT
	special GymCandidate_FaceBack
	return

@ Puts the mentor (VAR_0x800A, looking like VAR_OBJ_GFX_ID_0) next to the player
GymCandidate_EventScript_MentorAppears::
	fadescreen FADE_TO_BLACK
	special GymCandidate_PlaceGuest
	goto_if_eq VAR_RESULT, FALSE, GymCandidate_EventScript_MentorNoRoom
	setobjectxyperm VAR_0x800A, VAR_0x8005, VAR_0x8006
	clearflag FLAG_HIDE_PALLET_GYM_CHALLENGER
	addobject VAR_0x800A
	special GymCandidate_FaceGuest
GymCandidate_EventScript_MentorNoRoom::
	fadescreen FADE_FROM_BLACK
	return

@ After the Cerulean (defensive item) and Cinnabar (offensive item) badges: the mentor's gift.
@ VAR_0x8009 = the leader, VAR_0x800A = the gym's spare person.
GymCandidate_EventScript_MentorGift::
	special GymCandidate_PrepareMentorGift
	goto_if_eq VAR_RESULT, MENTOR_GIFT_NONE, GymCandidate_EventScript_Return
	goto_if_eq VAR_RESULT, MENTOR_GIFT_LEADER, GymCandidate_EventScript_MentorGiveItem
	playse SE_DOOR
	msgbox GymCandidate_Text_SomeoneToSeeYou
	closemessage
	call GymCandidate_EventScript_VisitorWalksIn
	call GymCandidate_EventScript_MentorGiveItem
	call GymCandidate_EventScript_VisitorWalksOut
	return

GymCandidate_EventScript_MentorGiveItem::
	special GymCandidate_BufferMentorGift
	msgbox gStringVar4
	giveitem VAR_0x8004
	goto_if_eq VAR_RESULT, TRUE, GymCandidate_EventScript_MentorItemExplain
	addpcitem VAR_0x8004, 1
	bufferitemname STR_VAR_2, VAR_0x8004
	msgbox GymHack_Text_GiftSentToPC
GymCandidate_EventScript_MentorItemExplain::
	bufferitemname STR_VAR_1, VAR_0x8004
	goto_if_eq VAR_0x8009, 6, GymCandidate_EventScript_EdgeItemExplain
	msgbox GymCandidate_Text_GuardItemExplain
	return

GymCandidate_EventScript_EdgeItemExplain::
	msgbox GymCandidate_Text_EdgeItemExplain
	return

@ ---- Elite Four: VAR_0x8009 = 8 (Lorelei) to 11 (Lance) ----
GymCandidate_EventScript_EliteFourMentor::
	special GymCandidate_BufferMentorFirst
	call_if_eq VAR_RESULT, TRUE, GymCandidate_EventScript_ShowText4
	return

GymCandidate_EventScript_EliteFourApprove::
	goto_if_set FLAG_IS_CHAMPION, GymCandidate_EventScript_Return
	special GymCandidate_BufferEliteFourApproval
	msgbox gStringVar4
GymCandidate_EventScript_Return::
	return

@ ---- The League's front door ----
GymCandidate_EventScript_LeagueApprovalCheck::
	special GymCandidate_BufferApprovals
	goto_if_lt VAR_RESULT, 8, GymCandidate_EventScript_NotApproved
	return

GymCandidate_EventScript_NotApproved::
	textcolor NPC_TEXT_COLOR_NEUTRAL
	msgbox GymCandidate_Text_NeedApprovals
	closemessage
	applymovement LOCALID_PLAYER, Common_Movement_WalkDown
	waitmovement 0
	releaseall
	end

@ ---- Prof. Oak, between badges (in his lab) ----
GymCandidate_EventScript_OakAside::
	special GymCandidate_BufferApprovals
	goto_if_eq VAR_RESULT, 0, GymCandidate_EventScript_Return
	goto_if_le VAR_RESULT, 2, GymCandidate_EventScript_OakAside1
	goto_if_le VAR_RESULT, 4, GymCandidate_EventScript_OakAside2
	goto_if_le VAR_RESULT, 6, GymCandidate_EventScript_OakAside3
	msgbox GymCandidate_Text_OakAside4
	return

GymCandidate_EventScript_OakAside1::
	msgbox GymCandidate_Text_OakAside1
	return

GymCandidate_EventScript_OakAside2::
	msgbox GymCandidate_Text_OakAside2
	return

GymCandidate_EventScript_OakAside3::
	msgbox GymCandidate_Text_OakAside3
	return

@ Daisy notices something
GymCandidate_EventScript_DaisyAside::
	goto_if_set FLAG_SYS_GAME_CLEAR, GymCandidate_EventScript_Return
	special GymCandidate_BufferApprovals
	goto_if_lt VAR_RESULT, 4, GymCandidate_EventScript_Return
	msgbox GymCandidate_Text_DaisyAside
	return

@ ---- Trainers on the road who want to join: VAR_0x8004 = their type ----
@ After the battle (and when talked to again until they've joined)
GymCandidate_EventScript_RecruitAfterBattle::
	call GymCandidate_EventScript_RecruitOffer
	release
	end

@ Returns if they've already joined; otherwise asks and ends the script
GymCandidate_EventScript_RecruitOffer::
	special GymCandidate_BufferRecruit
	goto_if_eq VAR_RESULT, TRUE, GymCandidate_EventScript_Return
	lock
	faceplayer
	msgbox gStringVar4, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, GymCandidate_EventScript_RecruitDeclined
	special GymCandidate_Recruit
	playfanfare MUS_OBTAIN_ITEM
	msgbox GymCandidate_Text_RecruitJoined
	waitfanfare
	msgbox GymCandidate_Text_RecruitHowToPlace
	release
	end

GymCandidate_EventScript_RecruitDeclined::
	msgbox GymCandidate_Text_RecruitDeclined
	release
	end

@ After-battle scripts of the trainers who can join (their battles are in the map scripts)
GymCandidate_EventScript_RecruitNormal::
	setvar VAR_0x8004, 1   @ TYPE_NORMAL
	goto GymCandidate_EventScript_RecruitAfterBattle
	end

GymCandidate_EventScript_RecruitFighting::
	setvar VAR_0x8004, 2   @ TYPE_FIGHTING
	goto GymCandidate_EventScript_RecruitAfterBattle
	end

GymCandidate_EventScript_RecruitFlying::
	setvar VAR_0x8004, 3   @ TYPE_FLYING
	goto GymCandidate_EventScript_RecruitAfterBattle
	end

GymCandidate_EventScript_RecruitPoison::
	setvar VAR_0x8004, 4   @ TYPE_POISON
	goto GymCandidate_EventScript_RecruitAfterBattle
	end

GymCandidate_EventScript_RecruitGround::
	setvar VAR_0x8004, 5   @ TYPE_GROUND
	goto GymCandidate_EventScript_RecruitAfterBattle
	end

GymCandidate_EventScript_RecruitRock::
	setvar VAR_0x8004, 6   @ TYPE_ROCK
	goto GymCandidate_EventScript_RecruitAfterBattle
	end

GymCandidate_EventScript_RecruitBug::
	setvar VAR_0x8004, 7   @ TYPE_BUG
	goto GymCandidate_EventScript_RecruitAfterBattle
	end

GymCandidate_EventScript_RecruitGhost::
	setvar VAR_0x8004, 8   @ TYPE_GHOST
	goto GymCandidate_EventScript_RecruitAfterBattle
	end

GymCandidate_EventScript_RecruitSteel::
	setvar VAR_0x8004, 9   @ TYPE_STEEL
	goto GymCandidate_EventScript_RecruitAfterBattle
	end

GymCandidate_EventScript_RecruitFire::
	setvar VAR_0x8004, 11   @ TYPE_FIRE
	goto GymCandidate_EventScript_RecruitAfterBattle
	end

GymCandidate_EventScript_RecruitWater::
	setvar VAR_0x8004, 12   @ TYPE_WATER
	goto GymCandidate_EventScript_RecruitAfterBattle
	end

GymCandidate_EventScript_RecruitGrass::
	setvar VAR_0x8004, 13   @ TYPE_GRASS
	goto GymCandidate_EventScript_RecruitAfterBattle
	end

GymCandidate_EventScript_RecruitElectric::
	setvar VAR_0x8004, 14   @ TYPE_ELECTRIC
	goto GymCandidate_EventScript_RecruitAfterBattle
	end

GymCandidate_EventScript_RecruitPsychic::
	setvar VAR_0x8004, 15   @ TYPE_PSYCHIC
	goto GymCandidate_EventScript_RecruitAfterBattle
	end

GymCandidate_EventScript_RecruitIce::
	setvar VAR_0x8004, 16   @ TYPE_ICE
	goto GymCandidate_EventScript_RecruitAfterBattle
	end

GymCandidate_EventScript_RecruitDragon::
	setvar VAR_0x8004, 17   @ TYPE_DRAGON
	goto GymCandidate_EventScript_RecruitAfterBattle
	end

GymCandidate_EventScript_RecruitDark::
	setvar VAR_0x8004, 18   @ TYPE_DARK
	goto GymCandidate_EventScript_RecruitAfterBattle
	end

GymCandidate_EventScript_RecruitFairy::
	setvar VAR_0x8004, 19   @ TYPE_FAIRY
	goto GymCandidate_EventScript_RecruitAfterBattle
	end

@ ---- Text ----
GymCandidate_Text_LaterThatDay::
	.string "Later that day…\n"
	.string "A challenger arrives!$"

GymCandidate_Text_AnotherOne::
	.string "Here comes the next challenger!$"

GymCandidate_Text_ApprovalCount::
	.string "GYM LEADER approvals:\n"
	.string "{STR_VAR_1} of 8.$"

GymCandidate_Text_TryAgain::
	.string "It's OK. Every LEADER loses\n"
	.string "sometimes.\p"
	.string "Come back when you're ready, and\n"
	.string "we'll try again.$"

GymCandidate_Text_NotYet::
	.string "Come back when you're ready.\n"
	.string "My GYM isn't going anywhere.$"

GymCandidate_Text_TeamBreaksRules::
	.string "Hold on. A GYM LEADER battles with\n"
	.string "one type, and no two alike.\p"
	.string "Fix your team first, then\n"
	.string "come back.$"

GymCandidate_Text_NeedApprovals::
	.string "The LEAGUE only lets in GYM\n"
	.string "LEADER candidates with the\l"
	.string "approval of all eight LEADERS.\p"
	.string "You have {STR_VAR_1} of 8 so far.\n"
	.string "Come back when you have them all!$"

GymCandidate_Text_OakAside1::
	.string "OAK: Ah, {PLAYER}! I hear a GYM\n"
	.string "LEADER has approved of you.\p"
	.string "Good, good… I picked out that GYM's\n"
	.string "floor tiles myself, you know.$"

GymCandidate_Text_OakAside2::
	.string "OAK: Several approvals already!\n"
	.string "My, my.\p"
	.string "You know, I was quite the TRAINER\n"
	.string "in my day.\p"
	.string "…But never mind that!$"

GymCandidate_Text_OakAside3::
	.string "OAK: Hm? Oh, I was just at the\n"
	.string "GYM. Checking the lights.\p"
	.string "It's a fine GYM. Very fine…\n"
	.string "Ahem! Now, your POKéDEX.$"

GymCandidate_Text_OakAside4::
	.string "OAK: Nearly every LEADER approves\n"
	.string "of you.\p"
	.string "The LEAGUE will be very pleased.\n"
	.string "…Yes. Very pleased.$"

GymCandidate_Text_DaisyAside::
	.string "DAISY: Grandpa's been going over\n"
	.string "to the GYM late at night lately.\p"
	.string "He says he's “inspecting.”\n"
	.string "In his old TRAINER's cap.$"

GymCandidate_Text_MentorAtDoor::
	.string "{STR_VAR_1}: Hello? PROF. OAK?\n"
	.string "Is your candidate here?$"

GymCandidate_Text_OakWelcomesMentor::
	.string "OAK: {STR_VAR_1}! You came all\n"
	.string "this way!\p"
	.string "{PLAYER}, this is the one I wrote\n"
	.string "to about you. Go on, say hello!$"

GymCandidate_Text_MentorDecorGift::
	.string "{STR_VAR_1}: Oh, and one more thing.\n"
	.string "A gift for your new GYM!$"

GymCandidate_Text_ReceivedMentorDecor::
	.string "{PLAYER} received the\n"
	.string "{STR_VAR_2}!\p"
	.string "It's waiting in the GYM PC.$"

GymCandidate_Text_OakAfterMentor::
	.string "OAK: My, my! One letter, and\n"
	.string "{STR_VAR_1} comes all this way!\p"
	.string "A fine mentor for you, {PLAYER}.\n"
	.string "Yes… I'm sure of it. Mostly.$"

GymCandidate_Text_SomeoneToSeeYou::
	.string "Hm? Someone came into the GYM\n"
	.string "looking for {PLAYER}…$"

GymCandidate_Text_GuardItemExplain::
	.string "When a POKéMON holds the\n"
	.string "{STR_VAR_1}, no move is\l"
	.string "super effective against it.\p"
	.string "It never wears out.$"

GymCandidate_Text_EdgeItemExplain::
	.string "When a POKéMON holds the\n"
	.string "{STR_VAR_1}, its moves always\l"
	.string "hit for at least normal damage.\p"
	.string "It never wears out.$"

GymCandidate_Text_RecruitJoined::
	.string "{STR_VAR_1} joined your GYM!$"

GymCandidate_Text_RecruitHowToPlace::
	.string "Place your new TRAINER from the\n"
	.string "GYM PC, under TRAINERS.$"

GymCandidate_Text_RecruitDeclined::
	.string "Oh… OK. If you change your mind,\n"
	.string "I'll be right here!$"
