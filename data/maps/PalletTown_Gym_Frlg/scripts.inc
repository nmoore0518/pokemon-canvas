PalletTown_Gym_Frlg_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PalletTown_Gym_OnLoad
	map_script MAP_SCRIPT_ON_TRANSITION, PalletTown_Gym_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, PalletTown_Gym_OnFrame
	.byte 0

@ Draw the decorations the player has placed
PalletTown_Gym_OnLoad::
	special GymDecor_DrawRoom
	end

@ No challenger until the aide sends one in; no aide during Oak's first visit
PalletTown_Gym_OnTransition::
	setflag FLAG_HIDE_PALLET_GYM_CHALLENGER
	call_if_eq VAR_MAP_SCENE_PALLET_TOWN_GYM, 1, PalletTown_Gym_EventScript_HideAide
	call_if_ne VAR_MAP_SCENE_PALLET_TOWN_GYM, 1, PalletTown_Gym_EventScript_ShowAide
	end

PalletTown_Gym_EventScript_HideAide::
	setflag FLAG_HIDE_PALLET_GYM_AIDE
	return

PalletTown_Gym_EventScript_ShowAide::
	clearflag FLAG_HIDE_PALLET_GYM_AIDE
	return

PalletTown_Gym_OnFrame::
	map_script_2 VAR_MAP_SCENE_PALLET_TOWN_GYM, 1, PalletTown_Gym_EventScript_OakShowsGym
	.2byte 0

@ Right after the first rival battle, Oak brings the player and the rival here.
PalletTown_Gym_EventScript_OakShowsGym::
	lockall
	textcolor NPC_TEXT_COLOR_MALE
	playbgm MUS_RG_OAK, 0
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	msgbox PalletTown_Gym_Text_OakHereWeAre
	applymovement LOCALID_PALLET_GYM_RIVAL, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	msgbox PalletTown_Gym_Text_RivalAsGoodAsMine
	msgbox PalletTown_Gym_Text_OakDecorateIt
	applymovement LOCALID_PALLET_GYM_RIVAL, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	msgbox PalletTown_Gym_Text_RivalTossYourJunk
	applymovement LOCALID_PALLET_GYM_RIVAL, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	msgbox PalletTown_Gym_Text_OakRunItMyself
	applymovement LOCALID_PALLET_GYM_RIVAL, Common_Movement_ExclamationMark
	waitmovement 0
	msgbox PalletTown_Gym_Text_RivalSeeYouAtTheTop
	closemessage
	playbgm MUS_RG_RIVAL_EXIT, 0
	applymovement LOCALID_PLAYER, PalletTown_Gym_Movement_WatchRivalLeave
	applymovement LOCALID_PALLET_GYM_RIVAL, PalletTown_Gym_Movement_RivalLeave
	waitmovement LOCALID_PALLET_GYM_RIVAL
	removeobject LOCALID_PALLET_GYM_RIVAL
	playse SE_EXIT
	delay 30
	playbgm MUS_RG_OAK, 0
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	msgbox PalletTown_Gym_Text_OakWaitingForYou
	closemessage
	applymovement LOCALID_PALLET_GYM_OAK, PalletTown_Gym_Movement_OakLeave
	waitmovement 0
	removeobject LOCALID_PALLET_GYM_OAK
	playse SE_EXIT
	setflag FLAG_HIDE_PALLET_GYM_OAK_AND_RIVAL
	setflag FLAG_PALLET_GYM_OPEN
	setvar VAR_MAP_SCENE_PALLET_TOWN_GYM, 2
	fadedefaultbgm
	releaseall
	end

PalletTown_Gym_Movement_WatchRivalLeave::
	walk_in_place_faster_right
	delay_16
	walk_in_place_faster_down
	step_end

PalletTown_Gym_Movement_RivalLeave::
	walk_down
	walk_down
	step_end

PalletTown_Gym_Movement_OakLeave::
	walk_right
	walk_down
	walk_down
	walk_down
	step_end

PalletTown_Gym_EventScript_GymStatue::
	goto GymChallenge_EventScript_Statue
	end

PalletTown_Gym_Text_OakHereWeAre::
	.string "OAK: Here we are!\n"
	.string "The PALLET TOWN GYM!\p"
	.string "The builders finished it just\n"
	.string "last week.\p"
	.string "As you can see, it's…\n"
	.string "well, rather empty.\p"
	.string "The LEAGUE left the inside up to\n"
	.string "its new GYM LEADER.$"

PalletTown_Gym_Text_RivalAsGoodAsMine::
	.string "{RIVAL}: Ha! Easy!\n"
	.string "This place is as good as mine!$"

PalletTown_Gym_Text_OakDecorateIt::
	.string "OAK: Not so fast!\n"
	.string "Neither of you is LEADER yet.\p"
	.string "But a GYM should look like its\n"
	.string "LEADER, so here's my idea.\p"
	.string "Whenever you're back in PALLET\n"
	.string "TOWN, come in and fix it up.\p"
	.string "Furniture, puzzles, decorations…\n"
	.string "Whatever you like!\p"
	.string "Think of it as part of your\n"
	.string "audition.\p"
	.string "Whatever's in here when the\n"
	.string "LEADER is decided stays!$"

PalletTown_Gym_Text_RivalTossYourJunk::
	.string "{RIVAL}: Don't get attached,\n"
	.string "{PLAYER}.\p"
	.string "I'm tossing out all your junk\n"
	.string "the day I win!$"

PalletTown_Gym_Text_OakRunItMyself::
	.string "OAK: Now, now.\p"
	.string "And if neither of you turns out\n"
	.string "to be good enough…\p"
	.string "…well, I suppose I'd just have\n"
	.string "to run it myself!\p"
	.string "It is a very nice GYM, after all.\n"
	.string "Hohoho!$"

PalletTown_Gym_Text_RivalSeeYouAtTheTop::
	.string "{RIVAL}: Gramps!?\p"
	.string "…Whatever.\p"
	.string "{PLAYER}! I'll see you at the top!\n"
	.string "Smell you later!$"

PalletTown_Gym_Text_OakWaitingForYou::
	.string "OAK: {PLAYER}, this GYM will be\n"
	.string "waiting for you.\p"
	.string "Now go! Get strong, and come\n"
	.string "back to claim it!$"

PalletTown_Gym_Text_GymStatue::
	.string "PALLET TOWN POKéMON GYM\n"
	.string "LEADER: To be decided!$"
