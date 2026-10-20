@ Gym Trainer hack: the legendary Pokémon of the player's type (src/gym_legend.c). It appears on the
@ battle stage right after Oak hands the gym over, and waits there until it's caught.

@ In the finale, after Oak explains the gym challenges (Oak and the aide are there)
GymLegend_EventScript_Arrives::
	special GymLegend_Setup
	goto_if_eq VAR_RESULT, FALSE, GymLegend_EventScript_Return
	copyvar VAR_0x8008, VAR_0x8004
	special GymLegend_FindSpot
	setobjectxyperm LOCALID_PALLET_GYM_LEGEND, VAR_0x8005, VAR_0x8006
	special GymLegend_PlanCamera
	fadeoutbgm 1
	delay 30
	setvar VAR_0x8004, 2
	setvar VAR_0x8005, 2
	setvar VAR_0x8006, 12
	setvar VAR_0x8007, 4
	special ShakeCamera
	waitstate
	playse SE_THUNDER
	fadescreen FADE_TO_BLACK
	msgbox GymLegend_Text_LightsOut
	closemessage
	clearflag FLAG_HIDE_PALLET_GYM_LEGEND
	addobject LOCALID_PALLET_GYM_LEGEND
	delay 40
	playse SE_THUNDER
	fadescreen FADE_FROM_WHITE
	playmoncry VAR_0x8008, CRY_MODE_ENCOUNTER
	waitmoncry
	special SpawnCameraObject
	applymovement LOCALID_CAMERA, gGymLegendCameraIn
	waitmovement 0
	playbgm MUS_RG_ENCOUNTER_DEOXYS, 0
	delay 30
	playmoncry VAR_0x8008, CRY_MODE_ENCOUNTER
	waitmoncry
	delay 30
	applymovement LOCALID_CAMERA, gGymLegendCameraOut
	waitmovement 0
	special RemoveCameraObject
	playse SE_PIN
	applymovement LOCALID_PLAYER, Common_Movement_ExclamationMark
	applymovement LOCALID_PALLET_GYM_OAK, Common_Movement_ExclamationMark
	waitmovement 0
	msgbox GymLegend_Text_OakWhatOnEarth
	msgbox GymLegend_Text_OakCatchIt
	setflag FLAG_GYM_LEGEND_APPEARED
GymLegend_EventScript_Return::
	return

@ Talking to it: one battle on the stage. If it gets away (or the player loses), the rival comes to
@ gloat and hints at Cerulean Cave; if it's caught, he comes to scoff.
GymLegend_EventScript_Legend::
	lock
	faceplayer
	special GymLegend_Setup
	copyvar VAR_0x8008, VAR_0x8004
	playmoncry VAR_0x8008, CRY_MODE_ENCOUNTER
	msgbox GymLegend_Text_Challenge
	waitmoncry
	closemessage
	setvar VAR_GYM_LEGEND_STATE, GYM_LEGEND_ESCAPED   @ (if the player loses, the rival's scene waits)
	special GymLegend_CreateMon
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	waitstate
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	removeobject LOCALID_PALLET_GYM_LEGEND
	goto_if_eq VAR_RESULT, B_OUTCOME_CAUGHT, GymLegend_EventScript_Caught
	special GymLegend_Setup
	msgbox GymLegend_Text_Fled
	closemessage
	goto GymLegend_EventScript_RivalEscaped
	end

GymLegend_EventScript_Caught::
	setflag FLAG_GYM_LEGEND_CAUGHT
	setvar VAR_GYM_LEGEND_STATE, GYM_LEGEND_CAUGHT_NEWS
	goto GymLegend_EventScript_RivalCaught
	end

@ The rival (also from the gym's ON_FRAME, after a loss or a catch in Cerulean Cave)
GymLegend_EventScript_RivalEscaped::
	lockall
	call GymLegend_EventScript_RivalWalksIn
	msgbox GymLegend_Text_RivalEscaped
	call GymLegend_EventScript_RivalWalksOut
	setvar VAR_GYM_LEGEND_STATE, GYM_LEGEND_IN_CAVE
	releaseall
	end

GymLegend_EventScript_RivalCaught::
	lockall
	call GymLegend_EventScript_RivalWalksIn
	msgbox GymLegend_Text_RivalCaught
	call GymLegend_EventScript_RivalWalksOut
	setvar VAR_GYM_LEGEND_STATE, GYM_LEGEND_DONE
	releaseall
	end

GymLegend_EventScript_RivalWalksIn::
	textcolor NPC_TEXT_COLOR_MALE
	playbgm MUS_RG_ENCOUNTER_RIVAL, 0
	setvar VAR_0x800A, LOCALID_PALLET_GYM_RIVAL
	special GymCandidate_PlanVisitorWalk
	goto_if_eq VAR_RESULT, FALSE, GymLegend_EventScript_RivalAppears
	setobjectxyperm LOCALID_PALLET_GYM_RIVAL, VAR_0x8005, VAR_0x8006
	clearflag FLAG_HIDE_PALLET_GYM_RIVAL
	addobject LOCALID_PALLET_GYM_RIVAL
	applymovement LOCALID_PALLET_GYM_RIVAL, gMentorWalkIn
	waitmovement 0
	special GymCandidate_FaceGuest
	return

GymLegend_EventScript_RivalAppears::
	fadescreen FADE_TO_BLACK
	special GymCandidate_PlaceGuest
	goto_if_eq VAR_RESULT, FALSE, GymLegend_EventScript_RivalNoRoom
	setobjectxyperm LOCALID_PALLET_GYM_RIVAL, VAR_0x8005, VAR_0x8006
	clearflag FLAG_HIDE_PALLET_GYM_RIVAL
	addobject LOCALID_PALLET_GYM_RIVAL
	special GymCandidate_FaceGuest
GymLegend_EventScript_RivalNoRoom::
	fadescreen FADE_FROM_BLACK
	return

GymLegend_EventScript_RivalWalksOut::
	closemessage
	playbgm MUS_RG_RIVAL_EXIT, 0
	special GymCandidate_VisitorWalked
	goto_if_eq VAR_RESULT, FALSE, GymLegend_EventScript_RivalVanishes
	applymovement LOCALID_PALLET_GYM_RIVAL, gMentorWalkOut
	waitmovement 0
GymLegend_EventScript_RivalVanishes::
	removeobject LOCALID_PALLET_GYM_RIVAL
	playse SE_EXIT
	special GymCandidate_FaceBack
	fadedefaultbgm
	return

GymLegend_Text_LightsOut::
	.string "Huh!? The lights!$"

GymLegend_Text_OakWhatOnEarth::
	.string "OAK: W-What on earth!?\n"
	.string "That's {STR_VAR_1}!\p"
	.string "They say no {STR_VAR_2}-type\n"
	.string "POKéMON is more powerful!\p"
	.string "It must have sensed a new\n"
	.string "{STR_VAR_2} GYM in KANTO…$"

GymLegend_Text_OakCatchIt::
	.string "OAK: Most GYM LEADERS don't use\n"
	.string "legendary POKéMON, you know.\p"
	.string "But there's no rule against it!\p"
	.string "And challengers are getting\n"
	.string "stronger these days, {PLAYER}…\p"
	.string "If you can catch it, it might be\n"
	.string "just what your GYM needs!\p"
	.string "It's waiting for you on the\n"
	.string "stage. Good luck!$"

GymLegend_Text_Challenge::
	.string "{STR_VAR_1} is waiting for a\n"
	.string "challenger!$"

GymLegend_Text_Fled::
	.string "{STR_VAR_1} fled the GYM!$"

GymLegend_Text_RivalEscaped::
	.string "{RIVAL}: Hey, {PLAYER}!\n"
	.string "I heard the racket from outside.\p"
	.string "You let a legendary POKéMON get\n"
	.string "away? In your own GYM?\p"
	.string "Some LEADER. Gramps should've\n"
	.string "given this place to ME.\p"
	.string "…Not that I'm helping you, but\n"
	.string "something that strong would hide\l"
	.string "where only the toughest go.\p"
	.string "Like that cave by CERULEAN nobody\n"
	.string "was allowed into. Just saying.\p"
	.string "Smell ya later, “LEADER”!$"

GymLegend_Text_RivalCaught::
	.string "{RIVAL}: So you caught a\n"
	.string "legendary POKéMON. Big deal.\p"
	.string "Guess you NEED a legendary to win\n"
	.string "battles now, huh?\p"
	.string "Some of us earned our wins.\n"
	.string "…This GYM should've been mine.\p"
	.string "Smell ya later, “LEADER”!$"
