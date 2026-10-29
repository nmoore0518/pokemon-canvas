@ The basement under the player's house: Dad's workshop (tools/gym_hack/build_basement.py)
@ The machines' animations set their blocks frame by frame (frames.inc, generated).
@ The text here is a first pass, to be replaced with the story's own.

PalletTown_PlayersHouse_B1F_Frlg_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PlayersHouse_B1F_OnLoad
	.byte 0

PlayersHouse_B1F_OnLoad::
	call_if_set FLAG_BASEMENT_SWITCH_SHOWN, PlayersHouse_B1F_EventScript_SetSwitch
	end

PlayersHouse_B1F_EventScript_SetSwitch::
	call_if_unset FLAG_BASEMENT_SWITCH_ON, PlayersHouse_B1F_EventScript_Frame_switch_off
	call_if_set FLAG_BASEMENT_SWITCH_ON, PlayersHouse_B1F_EventScript_Frame_switch_on
	return

@ ---- The Ability Synthesizer: the belt runs, the stream falls, the vat churns ----
PlayersHouse_B1F_EventScript_Synthesizer::
	lockall
	playse SE_M_BUBBLE
	call PlayersHouse_B1F_EventScript_SynthCycle
	call PlayersHouse_B1F_EventScript_SynthCycle
	call PlayersHouse_B1F_EventScript_SynthCycle
	call PlayersHouse_B1F_EventScript_Frame_synth0
	special DrawWholeMapView
	msgbox PlayersHouse_B1F_Text_Synthesizer
	releaseall
	end

PlayersHouse_B1F_EventScript_SynthCycle::
	call PlayersHouse_B1F_EventScript_Frame_synth1
	special DrawWholeMapView
	delay 6
	call PlayersHouse_B1F_EventScript_Frame_synth2
	special DrawWholeMapView
	delay 6
	call PlayersHouse_B1F_EventScript_Frame_synth0
	special DrawWholeMapView
	delay 6
	return

@ ---- The Ball Maker: the crank goes round twice, a ball drops into the slot ----
PlayersHouse_B1F_EventScript_BallMaker::
	lockall
	call PlayersHouse_B1F_EventScript_CrankTurn
	call PlayersHouse_B1F_EventScript_CrankTurn
	call PlayersHouse_B1F_EventScript_Frame_maker_ball
	special DrawWholeMapView
	playse SE_BALL
	waitse
	msgbox PlayersHouse_B1F_Text_BallMaker
	call PlayersHouse_B1F_EventScript_Frame_maker0
	special DrawWholeMapView
	releaseall
	end

PlayersHouse_B1F_EventScript_CrankTurn::
	playse SE_CLICK
	call PlayersHouse_B1F_EventScript_Frame_maker1
	special DrawWholeMapView
	delay 8
	call PlayersHouse_B1F_EventScript_Frame_maker2
	special DrawWholeMapView
	delay 8
	playse SE_CLICK
	call PlayersHouse_B1F_EventScript_Frame_maker3
	special DrawWholeMapView
	delay 8
	call PlayersHouse_B1F_EventScript_Frame_maker0
	special DrawWholeMapView
	delay 8
	return

@ ---- The Ball Changer: the lever, the rings light, the balls leap across ----
PlayersHouse_B1F_EventScript_BallChanger::
	lockall
	playse SE_SWITCH
	call PlayersHouse_B1F_EventScript_Frame_changer1
	special DrawWholeMapView
	delay 20
	playse SE_LEDGE
	applymovement LOCALID_BASEMENT_BALL_LEFT, PlayersHouse_B1F_Movement_BallAcrossRight
	applymovement LOCALID_BASEMENT_BALL_RIGHT, PlayersHouse_B1F_Movement_BallAcrossLeft
	waitmovement 0
	delay 20
	call PlayersHouse_B1F_EventScript_Frame_changer0
	special DrawWholeMapView
	@ (the two balls look alike: each back on its own pedestal)
	setobjectxy LOCALID_BASEMENT_BALL_LEFT, 5, 6
	setobjectxy LOCALID_BASEMENT_BALL_RIGHT, 8, 6
	msgbox PlayersHouse_B1F_Text_BallChanger
	releaseall
	end

PlayersHouse_B1F_Movement_BallAcrossRight:
	jump_2_right
	jump_right
	step_end

PlayersHouse_B1F_Movement_BallAcrossLeft:
	jump_2_left
	jump_left
	step_end

PlayersHouse_B1F_EventScript_Ball::
	msgbox PlayersHouse_B1F_Text_Ball, MSGBOX_NPC
	end

@ ---- The wall switch: not there until the story puts it there ----
PlayersHouse_B1F_EventScript_Switch::
	goto_if_unset FLAG_BASEMENT_SWITCH_SHOWN, PlayersHouse_B1F_EventScript_SwitchWall
	goto_if_set FLAG_BASEMENT_SWITCH_ON, PlayersHouse_B1F_EventScript_SwitchIsOn
	msgbox PlayersHouse_B1F_Text_SwitchOff, MSGBOX_SIGN
	end

PlayersHouse_B1F_EventScript_SwitchIsOn::
	msgbox PlayersHouse_B1F_Text_SwitchOn, MSGBOX_SIGN
	end

PlayersHouse_B1F_EventScript_SwitchWall::
	msgbox PlayersHouse_B1F_Text_Wall, MSGBOX_SIGN
	end

@ ---- The loose pages ----
PlayersHouse_B1F_EventScript_Paper1::
	msgbox PlayersHouse_B1F_Text_Paper1, MSGBOX_SIGN
	end

PlayersHouse_B1F_EventScript_Paper2::
	msgbox PlayersHouse_B1F_Text_Paper2, MSGBOX_SIGN
	end

PlayersHouse_B1F_EventScript_Paper3::
	msgbox PlayersHouse_B1F_Text_Paper3, MSGBOX_SIGN
	end

PlayersHouse_B1F_EventScript_Paper4::
	msgbox PlayersHouse_B1F_Text_Paper4, MSGBOX_SIGN
	end

@ ---- Everything else ----
PlayersHouse_B1F_EventScript_Chalkboard::
	msgbox PlayersHouse_B1F_Text_Chalkboard, MSGBOX_SIGN
	end

PlayersHouse_B1F_EventScript_Bookshelf::
	msgbox PlayersHouse_B1F_Text_Bookshelf, MSGBOX_SIGN
	end

PlayersHouse_B1F_EventScript_Workbench::
	msgbox PlayersHouse_B1F_Text_Workbench, MSGBOX_SIGN
	end

PlayersHouse_B1F_EventScript_LabTable::
	msgbox PlayersHouse_B1F_Text_LabTable, MSGBOX_SIGN
	end

PlayersHouse_B1F_EventScript_Desk::
	msgbox PlayersHouse_B1F_Text_Desk, MSGBOX_SIGN
	end

PlayersHouse_B1F_EventScript_TrashCan::
	msgbox PlayersHouse_B1F_Text_TrashCan, MSGBOX_SIGN
	end

PlayersHouse_B1F_EventScript_Crates::
	msgbox PlayersHouse_B1F_Text_Crates, MSGBOX_SIGN
	end

	.include "data/maps/PalletTown_PlayersHouse_B1F_Frlg/frames.inc"

PlayersHouse_B1F_Text_Synthesizer::
	.string "A belt feeds into a falling sheet of\n"
	.string "glowing liquid. It churns and hisses.\p"
	.string "A plate on its side reads:\n"
	.string "ABILITY SYNTHESIZER.$"

PlayersHouse_B1F_Text_BallMaker::
	.string "The crank turns stiffly…\n"
	.string "Something drops into the slot.\p"
	.string "A plate on its side reads:\n"
	.string "BALL MAKER.$"

PlayersHouse_B1F_Text_BallChanger::
	.string "The two POKé BALLS leapt across and\n"
	.string "traded places!\p"
	.string "A plate on the console reads:\n"
	.string "BALL CHANGER.$"

PlayersHouse_B1F_Text_Ball::
	.string "A POKé BALL rests in the dish.$"

PlayersHouse_B1F_Text_Wall::
	.string "Cold concrete. It's damp to\n"
	.string "the touch.$"

PlayersHouse_B1F_Text_SwitchOff::
	.string "A heavy switch on the wall.\n"
	.string "It's up.$"

PlayersHouse_B1F_Text_SwitchOn::
	.string "A heavy switch on the wall.\n"
	.string "It's been thrown.$"

PlayersHouse_B1F_Text_Paper1::
	.string "A loose page, covered in hurried\n"
	.string "handwriting. (Page 1)$"

PlayersHouse_B1F_Text_Paper2::
	.string "A loose page, creased and stained.\n"
	.string "(Page 2)$"

PlayersHouse_B1F_Text_Paper3::
	.string "A loose page. Most of it has been\n"
	.string "scratched out. (Page 3)$"

PlayersHouse_B1F_Text_Paper4::
	.string "A loose page, torn at one corner.\n"
	.string "(Page 4)$"

PlayersHouse_B1F_Text_Chalkboard::
	.string "A POKé BALL, drawn split in two.\n"
	.string "Arrows, numbers… and a question mark.$"

PlayersHouse_B1F_Text_Bookshelf::
	.string "Dusty books on POKéMON abilities and\n"
	.string "machinery. A jar holds something\l"
	.string "murky.$"

PlayersHouse_B1F_Text_Workbench::
	.string "Tools, a vise, and a blueprint\n"
	.string "too smudged to read.$"

PlayersHouse_B1F_Text_LabTable::
	.string "Flasks of green, amber and rust-red\n"
	.string "liquid. Something has dried in the\l"
	.string "tubing.$"

PlayersHouse_B1F_Text_Desk::
	.string "A typewriter and a stack of notes.\n"
	.string "The lamp still works.$"

PlayersHouse_B1F_Text_TrashCan::
	.string "Crumpled papers. Nothing readable.$"

PlayersHouse_B1F_Text_Crates::
	.string "Wooden crates, nailed shut.$"
