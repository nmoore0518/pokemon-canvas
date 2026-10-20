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

@ Talking to it
GymLegend_EventScript_Legend::
	lock
	faceplayer
	special GymLegend_Setup
	copyvar VAR_0x8008, VAR_0x8004
	playmoncry VAR_0x8008, CRY_MODE_ENCOUNTER
	msgbox GymLegend_Text_Challenge
	waitmoncry
	closemessage
	special GymLegend_CreateMon
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	waitstate
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_CAUGHT, GymLegend_EventScript_Caught
	removeobject LOCALID_PALLET_GYM_LEGEND
	special GymLegend_Setup
	msgbox GymLegend_Text_Vanished
	release
	end

GymLegend_EventScript_Caught::
	setflag FLAG_GYM_LEGEND_CAUGHT
	removeobject LOCALID_PALLET_GYM_LEGEND
	release
	end

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

GymLegend_Text_Vanished::
	.string "{STR_VAR_1} vanished…\p"
	.string "It may come back to the stage\n"
	.string "later.$"
