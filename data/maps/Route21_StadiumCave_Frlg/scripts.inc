@ Pokémon Canvas: Route 21's hidden cave (tools/gym_hack/build_stadium_cave.py; src/surfing_pikachu.c).
@ The trainer from Pokémon Stadium's Master Ball Cup, round 2 (the "Cool" one), with his team at
@ level 100. Beaten with a Pikachu in the party: a Surfing Pikachu. Beaten with nothing but Pikachu:
@ a shiny one (one of each, ever). He never says so until he's been beaten without one.
@ (His lines are drafts.)
Route21_StadiumCave_Frlg_MapScripts::
	.byte 0

Route21_StadiumCave_EventScript_Trainer::
	lock
	faceplayer
	textcolor NPC_TEXT_COLOR_MALE
	special SurfingPikachu_Owed
	goto_if_ne VAR_RESULT, 0, Route21_StadiumCave_EventScript_Reward
	goto_if_set FLAG_STADIUM_MET, Route21_StadiumCave_EventScript_Again
	msgbox Route21_StadiumCave_Text_Intro
	setflag FLAG_STADIUM_MET
	goto Route21_StadiumCave_EventScript_Ask

Route21_StadiumCave_EventScript_Again::
	special SurfingPikachu_AllGiven
	goto_if_eq VAR_RESULT, TRUE, Route21_StadiumCave_EventScript_AgainAllGiven
	msgbox Route21_StadiumCave_Text_Again
	goto Route21_StadiumCave_EventScript_Ask

Route21_StadiumCave_EventScript_AgainAllGiven::
	msgbox Route21_StadiumCave_Text_AgainAllGiven
Route21_StadiumCave_EventScript_Ask::
	msgbox Route21_StadiumCave_Text_Battle, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Route21_StadiumCave_EventScript_NotNow
	msgbox Route21_StadiumCave_Text_Start
	closemessage
	cleartrainerflag TRAINER_COOLTRAINER_STADIUM
	trainerbattle_no_intro TRAINER_COOLTRAINER_STADIUM, Route21_StadiumCave_Text_Defeat
	special SurfingPikachu_CheckParty
	goto_if_eq VAR_RESULT, STADIUM_PARTY_NO_PIKACHU, Route21_StadiumCave_EventScript_NoPikachu
	special SurfingPikachu_Owed
	goto_if_ne VAR_RESULT, 0, Route21_StadiumCave_EventScript_Reward
	msgbox Route21_StadiumCave_Text_WonNothingLeft
	release
	end

Route21_StadiumCave_EventScript_NoPikachu::
	special SurfingPikachu_AllGiven
	goto_if_eq VAR_RESULT, TRUE, Route21_StadiumCave_EventScript_WonNothingLeft
	msgbox Route21_StadiumCave_Text_BringPikachu
	release
	end

Route21_StadiumCave_EventScript_WonNothingLeft::
	msgbox Route21_StadiumCave_Text_WonNothingLeft
	release
	end

@ VAR_RESULT (SurfingPikachu_Owed): 2 the shiny one, 1 the other
Route21_StadiumCave_EventScript_Reward::
	call_if_eq VAR_RESULT, 2, Route21_StadiumCave_EventScript_SayShiny
	call_if_eq VAR_RESULT, 1, Route21_StadiumCave_EventScript_SaySurfing
	special SurfingPikachu_Give
	goto_if_eq VAR_RESULT, MON_CANT_GIVE, Route21_StadiumCave_EventScript_NoRoom
	copyvar VAR_0x8005, VAR_RESULT
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	call_if_eq VAR_0x8004, TRUE, Route21_StadiumCave_EventScript_ReceivedShiny
	call_if_eq VAR_0x8004, FALSE, Route21_StadiumCave_EventScript_Received
	waitmessage
	waitfanfare
	call_if_eq VAR_0x8005, MON_GIVEN_TO_PC, Route21_StadiumCave_EventScript_ToPC
	msgbox Route21_StadiumCave_Text_TakeCare
	@ (both earned at once: the next one now)
	special SurfingPikachu_Owed
	goto_if_ne VAR_RESULT, 0, Route21_StadiumCave_EventScript_Reward
	release
	end

Route21_StadiumCave_EventScript_SayShiny::
	msgbox Route21_StadiumCave_Text_OnlyPikachu
	return

Route21_StadiumCave_EventScript_SaySurfing::
	msgbox Route21_StadiumCave_Text_WithPikachu
	return

Route21_StadiumCave_EventScript_ReceivedShiny::
	message Route21_StadiumCave_Text_ReceivedShiny
	return

Route21_StadiumCave_EventScript_Received::
	message Route21_StadiumCave_Text_Received
	return

Route21_StadiumCave_EventScript_ToPC::
	msgbox Route21_StadiumCave_Text_SentToPC
	return

Route21_StadiumCave_EventScript_NoRoom::
	msgbox Route21_StadiumCave_Text_NoRoom
	release
	end

Route21_StadiumCave_EventScript_NotNow::
	msgbox Route21_StadiumCave_Text_NotNow
	release
	end

Route21_StadiumCave_EventScript_GoldDiglett::
	msgbox Route21_StadiumCave_Text_GoldDiglett, MSGBOX_SIGN
	end

@ ---- drafts ----
Route21_StadiumCave_Text_Intro::
	.string "…Huh. Nobody ever finds this place.\p"
	.string "You've got the look of a real trainer.\n"
	.string "I know the look. I used to have it.\p"
	.string "I battled in an arena once, far from\n"
	.string "here. Everything there looked rounder,\l"
	.string "somehow. Like the world had grown an\l"
	.string "extra side to it.\p"
	.string "I made it to the top cup. The one named\n"
	.string "for the best BALL there is. The second\l"
	.string "round.\p"
	.string "That's as far as I got. A kid came\n"
	.string "through, and that was that.\p"
	.string "I've been out here ever since, getting\n"
	.string "ready for the next one.$"

Route21_StadiumCave_Text_Again::
	.string "Back again?\p"
	.string "The tide's good today. So's my team.$"

Route21_StadiumCave_Text_AgainAllGiven::
	.string "You again. There's nothing left for me\n"
	.string "to give you…\p"
	.string "…but a good battle's its own reward.$"

Route21_StadiumCave_Text_Battle::
	.string "Want to see how you'd have done in\n"
	.string "the second round?$"

Route21_StadiumCave_Text_Start::
	.string "Then step into the ring.\n"
	.string "No rentals here.$"

Route21_StadiumCave_Text_NotNow::
	.string "Suit yourself. I'll be here.\n"
	.string "I'm always here.$"

Route21_StadiumCave_Text_Defeat::
	.string "Just like back then…\n"
	.string "in the stadium.$"

Route21_StadiumCave_Text_BringPikachu::
	.string "You're as good as that kid was.\p"
	.string "That kid had a PIKACHU, you know.\p"
	.string "Bring a PIKACHU with you next time.\n"
	.string "I've got something for it.$"

Route21_StadiumCave_Text_WonNothingLeft::
	.string "Just like back then. Ha!\p"
	.string "Come back anytime. I'll be ready.$"

Route21_StadiumCave_Text_WithPikachu::
	.string "That PIKACHU… you really are like that\n"
	.string "kid.\p"
	.string "After that round, I kept something to\n"
	.string "remember it by. I think it should be\l"
	.string "with you now.$"

Route21_StadiumCave_Text_OnlyPikachu::
	.string "Nothing but PIKACHU…?\n"
	.string "Not even that kid tried that.\p"
	.string "Then you should have this one. It shines\n"
	.string "like the sea at sunrise.$"

Route21_StadiumCave_Text_Received::
	.string "{PLAYER} received a SURFING PIKACHU!$"

Route21_StadiumCave_Text_ReceivedShiny::
	.string "{PLAYER} received a shining\n"
	.string "SURFING PIKACHU!$"

Route21_StadiumCave_Text_SentToPC::
	.string "The SURFING PIKACHU was sent to the PC.$"

Route21_StadiumCave_Text_TakeCare::
	.string "It loves the waves. There's a beach\n"
	.string "house on ROUTE 19, south of FUCHSIA.\p"
	.string "Take it there sometime.$"

Route21_StadiumCave_Text_NoRoom::
	.string "You've got no room for it. Come back\n"
	.string "when you do. I'll hold on to it.$"

Route21_StadiumCave_Text_GoldDiglett::
	.string "A DIGLETT, cast in solid gold.\p"
	.string "It looks like a trophy from somewhere\n"
	.string "far away.$"
