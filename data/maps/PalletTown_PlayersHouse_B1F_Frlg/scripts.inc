@ The basement under the player's house: Dad's workshop (tools/gym_hack/build_basement.py)
@ The machines' animations set their blocks frame by frame (frames.inc, generated).
@ The text here is a first pass, to be replaced with the story's own.

PalletTown_PlayersHouse_B1F_Frlg_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, PlayersHouse_B1F_OnLoad
	map_script MAP_SCRIPT_ON_TRANSITION, PlayersHouse_B1F_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, PlayersHouse_B1F_OnFrame
	.byte 0

PlayersHouse_B1F_OnTransition::
	call DadStory_EventScript_BasementTransition
	end

PlayersHouse_B1F_OnFrame::
	map_script_2 VAR_DAD_STORY, DAD_STORY_SILPH_HOME, DadStory_EventScript_Basement
	.2byte 0

PlayersHouse_B1F_OnLoad::
	call_if_set FLAG_BASEMENT_SWITCH_SHOWN, PlayersHouse_B1F_EventScript_SetSwitch
	end

PlayersHouse_B1F_EventScript_SetSwitch::
	call_if_unset FLAG_BASEMENT_SWITCH_ON, PlayersHouse_B1F_EventScript_Frame_switch_off
	call_if_set FLAG_BASEMENT_SWITCH_ON, PlayersHouse_B1F_EventScript_Frame_switch_on
	return

@ ---- The Ability Synthesizer: 3 Strong Acid (Evil Mode) or Apple Juice (Nice Mode) and a Pokémon in,
@      the Pokémon out with its synthetic ability (src/basement_machines.c) ----
PlayersHouse_B1F_EventScript_Synthesizer::
	lockall
	setvar VAR_0x8006, ITEM_STRONG_ACID
	goto_if_unset FLAG_BASEMENT_NICE_MODE, PlayersHouse_B1F_EventScript_SynthFuel
	setvar VAR_0x8006, ITEM_APPLE_JUICE
PlayersHouse_B1F_EventScript_SynthFuel::
	bufferitemname STR_VAR_3, VAR_0x8006
	msgbox PlayersHouse_B1F_Text_Synthesizer
	checkitem VAR_0x8006, SYNTH_FUEL_COUNT
	goto_if_eq VAR_RESULT, FALSE, PlayersHouse_B1F_EventScript_SynthNoFuel
	msgbox PlayersHouse_B1F_Text_SynthAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PlayersHouse_B1F_EventScript_MachineEnd
	special ChoosePartyMon
	goto_if_ge VAR_0x8004, PARTY_SIZE, PlayersHouse_B1F_EventScript_MachineEnd
	special Synthesizer_CheckMon
	goto_if_eq VAR_RESULT, MACHINE_EGG, PlayersHouse_B1F_EventScript_MachineEgg
	goto_if_eq VAR_RESULT, MACHINE_SYNTH_BLOCKED, PlayersHouse_B1F_EventScript_SynthBlocked
	goto_if_eq VAR_RESULT, MACHINE_SYNTH_ALREADY, PlayersHouse_B1F_EventScript_SynthAlready
	msgbox PlayersHouse_B1F_Text_SynthConfirm, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PlayersHouse_B1F_EventScript_MachineEnd
	removeitem VAR_0x8006, SYNTH_FUEL_COUNT
	playse SE_M_BUBBLE
	message PlayersHouse_B1F_Text_SynthRemoving
	waitmessage
	call PlayersHouse_B1F_EventScript_SynthCycle
	call PlayersHouse_B1F_EventScript_SynthCycle
	playse SE_M_BUBBLE
	message PlayersHouse_B1F_Text_SynthImplanting
	waitmessage
	call PlayersHouse_B1F_EventScript_SynthCycle
	call PlayersHouse_B1F_EventScript_SynthCycle
	call PlayersHouse_B1F_EventScript_Frame_synth0
	special DrawWholeMapView
	special Synthesizer_Apply
	goto_if_set FLAG_BASEMENT_NICE_MODE, PlayersHouse_B1F_EventScript_SynthDoneNice
	playse SE_FAINT
	msgbox PlayersHouse_B1F_Text_SynthDoneEvil
	releaseall
	end

PlayersHouse_B1F_EventScript_SynthDoneNice::
	playfanfare MUS_LEVEL_UP
	msgbox PlayersHouse_B1F_Text_SynthDoneNice
	waitfanfare
	releaseall
	end

PlayersHouse_B1F_EventScript_SynthNoFuel::
	msgbox PlayersHouse_B1F_Text_SynthNoFuel
	releaseall
	end

PlayersHouse_B1F_EventScript_SynthBlocked::
	msgbox PlayersHouse_B1F_Text_SynthBlocked
	releaseall
	end

PlayersHouse_B1F_EventScript_SynthAlready::
	msgbox PlayersHouse_B1F_Text_SynthAlready
	releaseall
	end

PlayersHouse_B1F_EventScript_MachineEgg::
	msgbox PlayersHouse_B1F_Text_MachineEgg
	releaseall
	end

PlayersHouse_B1F_EventScript_MachineEnd::
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

@ ---- The Ball Maker: a Pokémon of Lv. 50+ in, a Ball of its primary type out. Evil Mode: the Pokémon is
@      gone. Nice Mode: it loses 5 levels. ----
PlayersHouse_B1F_EventScript_BallMaker::
	lockall
	msgbox PlayersHouse_B1F_Text_BallMaker
	msgbox PlayersHouse_B1F_Text_MakerAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PlayersHouse_B1F_EventScript_MachineEnd
	special ChoosePartyMon
	goto_if_ge VAR_0x8004, PARTY_SIZE, PlayersHouse_B1F_EventScript_MachineEnd
	special BallMaker_CheckMon
	goto_if_eq VAR_RESULT, MACHINE_EGG, PlayersHouse_B1F_EventScript_MachineEgg
	goto_if_eq VAR_RESULT, MACHINE_MAKER_LOW_LEVEL, PlayersHouse_B1F_EventScript_MakerLowLevel
	goto_if_eq VAR_RESULT, MACHINE_MAKER_LAST_MON, PlayersHouse_B1F_EventScript_MakerLastMon
	goto_if_eq VAR_RESULT, MACHINE_MAKER_BAG_FULL, PlayersHouse_B1F_EventScript_MakerBagFull
	goto_if_set FLAG_BASEMENT_NICE_MODE, PlayersHouse_B1F_EventScript_MakerNice
	msgbox PlayersHouse_B1F_Text_MakerConfirmEvil, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PlayersHouse_B1F_EventScript_MachineEnd
	message PlayersHouse_B1F_Text_MakerGrinding
	waitmessage
	call PlayersHouse_B1F_EventScript_CrankTurn
	message PlayersHouse_B1F_Text_MakerExtracting
	waitmessage
	call PlayersHouse_B1F_EventScript_CrankTurn
	special BallMaker_Apply
	playse SE_FAINT
	message PlayersHouse_B1F_Text_MakerEliminated
	waitmessage
	delay 30
	call PlayersHouse_B1F_EventScript_MakerBallOut
	msgbox PlayersHouse_B1F_Text_MakerCreated
	call PlayersHouse_B1F_EventScript_MakerTakeBall
	releaseall
	end

PlayersHouse_B1F_EventScript_MakerNice::
	msgbox PlayersHouse_B1F_Text_MakerConfirmNice, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PlayersHouse_B1F_EventScript_MachineEnd
	message PlayersHouse_B1F_Text_MakerScanning
	waitmessage
	call PlayersHouse_B1F_EventScript_CrankTurn
	message PlayersHouse_B1F_Text_MakerSampling
	waitmessage
	call PlayersHouse_B1F_EventScript_CrankTurn
	special BallMaker_Apply
	call PlayersHouse_B1F_EventScript_MakerBallOut
	msgbox PlayersHouse_B1F_Text_MakerCreatedNice
	msgbox PlayersHouse_B1F_Text_MakerNiceMonBack
	call PlayersHouse_B1F_EventScript_MakerTakeBall
	releaseall
	end

PlayersHouse_B1F_EventScript_MakerBallOut::
	call PlayersHouse_B1F_EventScript_Frame_maker_ball
	special DrawWholeMapView
	playse SE_BALL
	waitse
	return

PlayersHouse_B1F_EventScript_MakerTakeBall::
	additem VAR_0x8005
	bufferitemname STR_VAR_3, VAR_0x8005
	playfanfare MUS_OBTAIN_ITEM
	msgbox PlayersHouse_B1F_Text_MakerTookBall
	waitfanfare
	call PlayersHouse_B1F_EventScript_Frame_maker0
	special DrawWholeMapView
	return

PlayersHouse_B1F_EventScript_MakerLowLevel::
	msgbox PlayersHouse_B1F_Text_MakerLowLevel
	releaseall
	end

PlayersHouse_B1F_EventScript_MakerLastMon::
	msgbox PlayersHouse_B1F_Text_MakerLastMon
	releaseall
	end

PlayersHouse_B1F_EventScript_MakerBagFull::
	msgbox PlayersHouse_B1F_Text_MakerBagFull
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

@ ---- The Ball Changer: a Pokémon and a Ball from the bag in, the Pokémon moves into the new Ball and the
@      old one comes back out (the same in both modes) ----
PlayersHouse_B1F_EventScript_BallChanger::
	lockall
	msgbox PlayersHouse_B1F_Text_BallChanger
	msgbox PlayersHouse_B1F_Text_ChangerAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PlayersHouse_B1F_EventScript_MachineEnd
	special ChoosePartyMon
	goto_if_ge VAR_0x8004, PARTY_SIZE, PlayersHouse_B1F_EventScript_MachineEnd
	special BallChanger_CheckMon
	goto_if_eq VAR_RESULT, MACHINE_EGG, PlayersHouse_B1F_EventScript_MachineEgg
	msgbox PlayersHouse_B1F_Text_ChangerPickBall
	special Bag_ChooseBall
	goto_if_eq VAR_ITEM_ID, ITEM_NONE, PlayersHouse_B1F_EventScript_MachineEnd
	special BallChanger_CheckBall
	goto_if_eq VAR_RESULT, MACHINE_CHANGER_SAME_BALL, PlayersHouse_B1F_EventScript_ChangerSame
	goto_if_eq VAR_RESULT, MACHINE_CHANGER_BAG_FULL, PlayersHouse_B1F_EventScript_ChangerBagFull
	msgbox PlayersHouse_B1F_Text_ChangerConfirm, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PlayersHouse_B1F_EventScript_MachineEnd
	special BallChanger_Apply
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
	msgbox PlayersHouse_B1F_Text_ChangerDone
	releaseall
	end

PlayersHouse_B1F_EventScript_ChangerSame::
	msgbox PlayersHouse_B1F_Text_ChangerSame
	releaseall
	end

PlayersHouse_B1F_EventScript_ChangerBagFull::
	msgbox PlayersHouse_B1F_Text_ChangerBagFull
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

@ ---- The wall switch: not there until the story puts it there. Up = Evil Mode, thrown = Nice Mode. ----
PlayersHouse_B1F_EventScript_Switch::
	lockall
	goto_if_unset FLAG_BASEMENT_SWITCH_SHOWN, PlayersHouse_B1F_EventScript_SwitchWall
	goto_if_set FLAG_BASEMENT_NICE_MODE, PlayersHouse_B1F_EventScript_SwitchToEvil
	msgbox PlayersHouse_B1F_Text_SwitchToNice, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PlayersHouse_B1F_EventScript_MachineEnd
	setflag FLAG_BASEMENT_NICE_MODE
	call PlayersHouse_B1F_EventScript_Frame_switch_on
	special DrawWholeMapView
	playse SE_PIN
	msgbox PlayersHouse_B1F_Text_SwitchedToNice
	releaseall
	end

PlayersHouse_B1F_EventScript_SwitchToEvil::
	msgbox PlayersHouse_B1F_Text_SwitchToEvil, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PlayersHouse_B1F_EventScript_MachineEnd
	clearflag FLAG_BASEMENT_NICE_MODE
	call PlayersHouse_B1F_EventScript_Frame_switch_off
	special DrawWholeMapView
	playse SE_PIN
	msgbox PlayersHouse_B1F_Text_SwitchedToEvil
	releaseall
	end

PlayersHouse_B1F_EventScript_SwitchWall::
	msgbox PlayersHouse_B1F_Text_Wall
	releaseall
	end

@ ---- The loose pages: Dad's notes, read as pages (src/dad_story.c) ----
PlayersHouse_B1F_EventScript_Paper1::
	lockall
	setvar VAR_0x8004, NOTE_PAGE_NOTE_1
	special ShowNotePage
	releaseall
	end

PlayersHouse_B1F_EventScript_Paper2::
	lockall
	setvar VAR_0x8004, NOTE_PAGE_NOTE_2
	special ShowNotePage
	releaseall
	end

PlayersHouse_B1F_EventScript_Paper3::
	lockall
	setvar VAR_0x8004, NOTE_PAGE_NOTE_3
	special ShowNotePage
	releaseall
	end

PlayersHouse_B1F_EventScript_Paper4::
	lockall
	setvar VAR_0x8004, NOTE_PAGE_NOTE_4
	special ShowNotePage
	releaseall
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
	.string "ABILITY SYNTHESIZER.\p"
	.string "A tank on the back is labeled:\n"
	.string "3 {STR_VAR_3}.$"

PlayersHouse_B1F_Text_SynthNoFuel::
	.string "The tank is dry. It needs 3\n"
	.string "{STR_VAR_3} to run.$"

PlayersHouse_B1F_Text_SynthAsk::
	.string "Pour in 3 {STR_VAR_3} and put a\n"
	.string "POKéMON on the belt?$"

PlayersHouse_B1F_Text_SynthConfirm::
	.string "The screen flickers on.\p"
	.string "SUBJECT: {STR_VAR_1}.\n"
	.string "SYNTHETIC ABILITY: {STR_VAR_2}.\l"
	.string "BEGIN?$"

PlayersHouse_B1F_Text_SynthRemoving::
	.string "REMOVING ABILITIES.$"

PlayersHouse_B1F_Text_SynthImplanting::
	.string "IMPLANTING SYNTHETIC ABILITY.$"

PlayersHouse_B1F_Text_SynthDoneEvil::
	.string "{STR_VAR_1} came out of the machine\n"
	.string "with {STR_VAR_2}!\p"
	.string "It's trembling… It looks like it's in\n"
	.string "a lot of pain.\p"
	.string "{STR_VAR_1} seems very unhappy…$"

PlayersHouse_B1F_Text_SynthDoneNice::
	.string "{STR_VAR_1} loved the taste of the\n"
	.string "APPLE JUICE so much, it didn't even\l"
	.string "notice the implant!\p"
	.string "{STR_VAR_1} came out with\n"
	.string "{STR_VAR_2}!\p"
	.string "It seems a little happier!$"

PlayersHouse_B1F_Text_SynthBlocked::
	.string "SUBJECT: {STR_VAR_1}.\n"
	.string "ERROR. ABILITY CANNOT BE REMOVED.$"

PlayersHouse_B1F_Text_SynthAlready::
	.string "SUBJECT: {STR_VAR_1}.\n"
	.string "ERROR. SYNTHETIC ABILITY {STR_VAR_2}\l"
	.string "ALREADY IMPLANTED.$"

PlayersHouse_B1F_Text_MachineEgg::
	.string "The machine won't take an EGG.$"

PlayersHouse_B1F_Text_BallMaker::
	.string "A big hopper with a hand crank beside\n"
	.string "it. A plate on its side reads:\l"
	.string "BALL MAKER.\p"
	.string "Scratched under the hatch:\n"
	.string "LV. 50 OR HIGHER.$"

PlayersHouse_B1F_Text_MakerAsk::
	.string "Put a POKéMON in the hopper?$"

PlayersHouse_B1F_Text_MakerLowLevel::
	.string "{STR_VAR_1} is too small. The hatch\n"
	.string "won't open.$"

PlayersHouse_B1F_Text_MakerLastMon::
	.string "{STR_VAR_1} is the only POKéMON\n"
	.string "with you! You can't do that.$"

PlayersHouse_B1F_Text_MakerBagFull::
	.string "There's no room in the BAG for\n"
	.string "what comes out.$"

PlayersHouse_B1F_Text_MakerConfirmEvil::
	.string "Put {STR_VAR_1} in the BALL MAKER?\n"
	.string "It won't be coming back out.$"

PlayersHouse_B1F_Text_MakerConfirmNice::
	.string "Put {STR_VAR_1} in the BALL MAKER?\n"
	.string "It will lose 5 levels.$"

PlayersHouse_B1F_Text_MakerGrinding::
	.string "GRINDING.$"

PlayersHouse_B1F_Text_MakerExtracting::
	.string "EXTRACTING.$"

PlayersHouse_B1F_Text_MakerEliminated::
	.string "POKéMON ELIMINATED.$"

PlayersHouse_B1F_Text_MakerCreated::
	.string "{STR_VAR_2} BALL CREATED.$"

PlayersHouse_B1F_Text_MakerScanning::
	.string "SCANNING.$"

PlayersHouse_B1F_Text_MakerSampling::
	.string "EXTRACTING SMALL DNA SAMPLE.\n"
	.string "PAINLESS MODE ENGAGED.$"

PlayersHouse_B1F_Text_MakerCreatedNice::
	.string "SAMPLE COMPLETE.\n"
	.string "{STR_VAR_2} BALL CREATED.$"

PlayersHouse_B1F_Text_MakerNiceMonBack::
	.string "{STR_VAR_1} hopped back out of the\n"
	.string "hatch, none the worse for it.\p"
	.string "{STR_VAR_1} is now Lv. {STR_VAR_3}.$"

PlayersHouse_B1F_Text_MakerTookBall::
	.string "{PLAYER} took the {STR_VAR_3}.$"

PlayersHouse_B1F_Text_BallChanger::
	.string "Two pedestals, each with a dish for a\n"
	.string "POKé BALL. A plate on the console\l"
	.string "reads: BALL CHANGER.$"

PlayersHouse_B1F_Text_ChangerAsk::
	.string "Move a POKéMON into a different\n"
	.string "BALL?$"

PlayersHouse_B1F_Text_ChangerPickBall::
	.string "{STR_VAR_1} is in a {STR_VAR_2}.\n"
	.string "Which BALL should it go into?$"

PlayersHouse_B1F_Text_ChangerSame::
	.string "{STR_VAR_1} is already in a\n"
	.string "{STR_VAR_3}.$"

PlayersHouse_B1F_Text_ChangerBagFull::
	.string "There's no room in the BAG for the\n"
	.string "{STR_VAR_2}.$"

PlayersHouse_B1F_Text_ChangerConfirm::
	.string "Move {STR_VAR_1} into the\n"
	.string "{STR_VAR_3}?$"

PlayersHouse_B1F_Text_ChangerDone::
	.string "The two BALLS leapt across and\n"
	.string "traded places!\p"
	.string "{STR_VAR_1} is in the {STR_VAR_3} now.\n"
	.string "{PLAYER} took back the {STR_VAR_2}.$"

PlayersHouse_B1F_Text_Ball::
	.string "A POKé BALL rests in the dish.$"

PlayersHouse_B1F_Text_Wall::
	.string "Cold concrete. It's damp to\n"
	.string "the touch.$"

PlayersHouse_B1F_Text_SwitchToNice::
	.string "Switch machines from Evil Mode to\n"
	.string "Nice Mode?$"

PlayersHouse_B1F_Text_SwitchToEvil::
	.string "Switch machines from Nice Mode to\n"
	.string "Evil Mode?$"

PlayersHouse_B1F_Text_SwitchedToNice::
	.string "{PLAYER} threw the switch.\n"
	.string "The machines hum softly. Nice Mode.$"

PlayersHouse_B1F_Text_SwitchedToEvil::
	.string "{PLAYER} threw the switch back.\n"
	.string "The machines growl. Evil Mode.$"

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
