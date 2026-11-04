@ Pokémon Canvas: Route 19's beach house (tools/gym_hack/build_beach_house.py), as in Yellow: the
@ Surfin' Dude lets a Surfing Pikachu ride at Pikachu's Beach (src/surfing_minigame.c).
@ (His lines are drafts.)
Route19_UnusedHouse_Frlg_MapScripts::
	.byte 0

Route19_BeachHouse_EventScript_SurfinDude::
	lock
	faceplayer
	textcolor NPC_TEXT_COLOR_MALE
	special SurfingPikachu_CheckInParty
	goto_if_eq VAR_RESULT, FALSE, Route19_BeachHouse_EventScript_NoSurfer
	goto_if_ne VAR_SURFING_HIGH_SCORE, 0, Route19_BeachHouse_EventScript_Again
	msgbox Route19_BeachHouse_Text_WhoaASurfer, MSGBOX_YESNO
	goto Route19_BeachHouse_EventScript_Answer

Route19_BeachHouse_EventScript_Again::
	buffernumberstring STR_VAR_1, VAR_SURFING_HIGH_SCORE
	msgbox Route19_BeachHouse_Text_Again, MSGBOX_YESNO
Route19_BeachHouse_EventScript_Answer::
	goto_if_eq VAR_RESULT, NO, Route19_BeachHouse_EventScript_NotToday
	msgbox Route19_BeachHouse_Text_HangTen
	closemessage
	fadescreen FADE_TO_BLACK
	special SurfingMinigame_Start
	waitstate
	buffernumberstring STR_VAR_1, VAR_SURFING_HIGH_SCORE
	msgbox Route19_BeachHouse_Text_After
	release
	end

Route19_BeachHouse_EventScript_NotToday::
	msgbox Route19_BeachHouse_Text_NotToday
	release
	end

Route19_BeachHouse_EventScript_NoSurfer::
	msgbox Route19_BeachHouse_Text_NoSurfer
	release
	end

Route19_BeachHouse_EventScript_Pikachu::
	lock
	faceplayer
	msgbox Route19_BeachHouse_Text_Pikachu
	playmoncry SPECIES_PIKACHU, CRY_MODE_NORMAL
	waitmoncry
	release
	end

@ ---- drafts ----
Route19_BeachHouse_Text_NoSurfer::
	.string "Yo! Welcome to PIKACHU's BEACH!\p"
	.string "Out there are the gnarliest waves in\n"
	.string "KANTO. But only a PIKACHU that knows\l"
	.string "how to surf can ride 'em.\p"
	.string "Find a PIKACHU like that, dude, and\n"
	.string "bring it here!$"

Route19_BeachHouse_Text_WhoaASurfer::
	.string "Whoa! That PIKACHU of yours…\n"
	.string "it's got a surfboard!\p"
	.string "Dude, the surf's up! Want to take it\n"
	.string "out on the waves?$"

Route19_BeachHouse_Text_Again::
	.string "Back for more waves, dude?\n"
	.string "Your best is {STR_VAR_1} points.\p"
	.string "Want to go again?$"

Route19_BeachHouse_Text_HangTen::
	.string "Radical! Use the + Control Pad to\n"
	.string "flip in the air. Land it right!$"

Route19_BeachHouse_Text_After::
	.string "Tubular, dude!\n"
	.string "Your best is {STR_VAR_1} points.$"

Route19_BeachHouse_Text_NotToday::
	.string "No sweat. The waves'll be here.$"

Route19_BeachHouse_Text_Pikachu::
	.string "PIKACHU: Pika pika!$"
