@ Gym Trainer hack: the Move Extractor (src/move_extractor.c) and the Rocket grunt who hands
@ them out. Say YES when he asks you to join TEAM ROCKET on Nugget Bridge (or later, when you
@ talk to him again): he can't let you in, but gives you a Move Extractor to "prove your worth"
@ by stealing moves. He finds you again in SAFFRON CITY after you beat GIOVANNI at SILPH CO.,
@ and in VIRIDIAN CITY once GIOVANNI has given you his approval as a GYM LEADER (he disbands
@ TEAM ROCKET), with one more each. VAR_MOVE_EXTRACTOR_STAGE: how many he's given (0-3).

@ On Nugget Bridge, after the NUGGET: asks; YES -> the grunt's offer (no battle, VAR_RESULT TRUE)
MoveExtractor_EventScript_AskToJoin::
	msgbox MoveExtractor_Text_WantToJoin, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, MoveExtractor_EventScript_SaidNo
	call MoveExtractor_EventScript_GruntSurprised
	setvar VAR_RESULT, TRUE
	return

MoveExtractor_EventScript_SaidNo::
	setvar VAR_RESULT, FALSE
	return

@ Talking to him after the bridge
MoveExtractor_EventScript_GruntAfterBridge::
	goto_if_ge VAR_MOVE_EXTRACTOR_STAGE, 1, MoveExtractor_EventScript_GruntKeepStealing
	msgbox MoveExtractor_Text_ChangedYourMind, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, MoveExtractor_EventScript_GruntHmph
	call MoveExtractor_EventScript_GruntSurprised
	release
	end

MoveExtractor_EventScript_GruntHmph::
	msgbox MoveExtractor_Text_Hmph
	release
	end

MoveExtractor_EventScript_GruntKeepStealing::
	msgbox MoveExtractor_Text_KeepStealing
	release
	end

MoveExtractor_EventScript_GruntSurprised::
	msgbox MoveExtractor_Text_Surprised
	setvar VAR_0x8009, ITEM_MOVE_EXTRACTOR
	call GymHack_EventScript_GiveStatItem
	setvar VAR_MOVE_EXTRACTOR_STAGE, 1
	msgbox MoveExtractor_Text_HowItWorks
	return

@ ---- His visits (map scripts set VAR_TEMP_9 when one is due) ----
@ VAR_0x800A = his local id in this city, VAR_0x8008 = which visit (2 or 3)
MoveExtractor_EventScript_Visit::
	lockall
	setvar VAR_TEMP_9, 0
	special GymCandidate_PlaceGuest
	goto_if_eq VAR_RESULT, FALSE, MoveExtractor_EventScript_VisitNoRoom
	setobjectxyperm VAR_0x800A, VAR_0x8005, VAR_0x8006
	clearflag FLAG_HIDE_MOVE_EXTRACTOR_GRUNT
	addobject VAR_0x800A
	special GymCandidate_FaceGuest
	playse SE_PIN
	applymovement VAR_0x800A, Common_Movement_ExclamationMark
	waitmovement 0
	call_if_eq VAR_0x8008, 2, MoveExtractor_EventScript_SecondVisit
	call_if_eq VAR_0x8008, 3, MoveExtractor_EventScript_ThirdVisit
	closemessage
	fadescreen FADE_TO_BLACK
	removeobject VAR_0x800A
	setflag FLAG_HIDE_MOVE_EXTRACTOR_GRUNT
	fadescreen FADE_FROM_BLACK
MoveExtractor_EventScript_VisitNoRoom::
	releaseall
	end

MoveExtractor_EventScript_SecondVisit::
	msgbox MoveExtractor_Text_SecondVisit
	setvar VAR_0x8009, ITEM_MOVE_EXTRACTOR
	call GymHack_EventScript_GiveStatItem
	setvar VAR_MOVE_EXTRACTOR_STAGE, 2
	msgbox MoveExtractor_Text_SecondVisitAfter
	return

MoveExtractor_EventScript_ThirdVisit::
	msgbox MoveExtractor_Text_ThirdVisit
	setvar VAR_0x8009, ITEM_MOVE_EXTRACTOR
	call GymHack_EventScript_GiveStatItem
	setvar VAR_MOVE_EXTRACTOR_STAGE, 3
	msgbox MoveExtractor_Text_ThirdVisitAfter
	return

MoveExtractor_EventScript_SaffronVisit::
	setvar VAR_0x800A, LOCALID_SAFFRON_EXTRACTOR_GRUNT
	setvar VAR_0x8008, 2
	goto MoveExtractor_EventScript_Visit
	end

MoveExtractor_EventScript_ViridianVisit::
	setvar VAR_0x800A, LOCALID_VIRIDIAN_EXTRACTOR_GRUNT
	setvar VAR_0x8008, 3
	goto MoveExtractor_EventScript_Visit
	end

@ ---- Text ----
MoveExtractor_Text_WantToJoin::
	.string "By the way, how would you like to\n"
	.string "join TEAM ROCKET?\p"
	.string "We're a group of professional\n"
	.string "criminals specializing in POKéMON!\p"
	.string "Want to join?$"

MoveExtractor_Text_ChangedYourMind::
	.string "So? Changed your mind?\n"
	.string "Want to join TEAM ROCKET?$"

MoveExtractor_Text_Hmph::
	.string "Hmph. Your loss, kid.$"

MoveExtractor_Text_Surprised::
	.string "Wh-what?! You actually want\n"
	.string "to join?\p"
	.string "…I wasn't expecting that.\n"
	.string "Nobody ever says yes.\p"
	.string "Uh… I mean, great! Welcome to…\n"
	.string "well, hold on a second.\p"
	.string "I'm just a GRUNT. I can't let\n"
	.string "people in. I don't have that kind\l"
	.string "of authority.\p"
	.string "Tell you what. Prove your worth,\n"
	.string "and I'll put in a good word with\l"
	.string "the higher-ups.\p"
	.string "TEAM ROCKET steals POKéMON. But\n"
	.string "a real pro steals their MOVES.\p"
	.string "Take this. Straight out of a SILPH\n"
	.string "CO. lab. Don't ask how I got it.$"

MoveExtractor_Text_HowItWorks::
	.string "It's a MOVE EXTRACTOR.\p"
	.string "In battle, use it after a foe uses\n"
	.string "a move. Your POKéMON learns that\l"
	.string "move… as a move of its own type!\p"
	.string "And the foe can't use it anymore.\n"
	.string "Pretty slick, huh?\p"
	.string "It only breaks once your POKéMON\n"
	.string "learns the move. So choose well!\p"
	.string "Steal some good moves, and I'll\n"
	.string "come find you. Later, recruit!$"

MoveExtractor_Text_KeepStealing::
	.string "Keep stealing those moves, kid!\n"
	.string "I'll come find you.$"

MoveExtractor_Text_SecondVisit::
	.string "Psst! Kid! It's me, from NUGGET\n"
	.string "BRIDGE!\p"
	.string "You… you beat the BOSS at SILPH\n"
	.string "CO.?! And I recommended you!\p"
	.string "I'm in so much trouble…\p"
	.string "…But you know what? I heard the\n"
	.string "BOSS say “That child has talent.”\p"
	.string "So he's not mad. I think.\n"
	.string "Here. Keep stealing moves!$"

MoveExtractor_Text_SecondVisitAfter::
	.string "Lay low for a while, OK?\n"
	.string "I'll find you again.$"

MoveExtractor_Text_ThirdVisit::
	.string "Kid… it's over.\n"
	.string "The BOSS is gone.\p"
	.string "No higher-ups. No recommendation.\n"
	.string "Sorry, kid.\p"
	.string "…Funny thing is, I don't even mind.\p"
	.string "You're going to be a GYM LEADER,\n"
	.string "right? Take the last one.\p"
	.string "Make your GYM the toughest in\n"
	.string "KANTO with it.$"

MoveExtractor_Text_ThirdVisitAfter::
	.string "Maybe I'll go straight.\n"
	.string "See you around, LEADER.$"
