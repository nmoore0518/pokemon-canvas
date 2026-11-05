@ Pokémon Canvas: the painter in Pewter City and her Smeargle (src/painter.c). Her loft is
@ data/maps/PewterCity_PainterLoft_Frlg; her easel leads into her paintings, real places in
@ scrambled colours, where a figure made of paint battles (PaintWorld_EventScript_Figure).
@ (All the lines are drafts.)

PainterLoft_EventScript_Painter::
	lock
	faceplayer
	special Painter_GetPending
	goto_if_ne VAR_RESULT, PAINTING_MODE_NONE, PainterLoft_EventScript_Ready
	special Painter_GetOffer
	switch VAR_RESULT
	case PAINTER_OFFER_NO_BADGE, PainterLoft_EventScript_NoBadge
	case PAINTER_OFFER_FIRST, PainterLoft_EventScript_First
	case PAINTER_OFFER_NEED_SMEARGLE, PainterLoft_EventScript_NeedSmeargle
	case PAINTER_OFFER_SKETCH, PainterLoft_EventScript_Sketch
	case PAINTER_OFFER_MEGA, PainterLoft_EventScript_Mega
	case PAINTER_OFFER_BOTH, PainterLoft_EventScript_Both
	msgbox PainterLoft_Text_Nothing
	release
	end

PainterLoft_EventScript_NoBadge::
	msgbox PainterLoft_Text_NoBadge
	release
	end

PainterLoft_EventScript_Ready::
	msgbox PainterLoft_Text_Ready
	release
	end

PainterLoft_EventScript_Later::
	msgbox PainterLoft_Text_Later
	release
	end

PainterLoft_EventScript_NeedSmeargle::
	msgbox PainterLoft_Text_NeedSmeargle
	release
	end

@ ---- the first painting: her Smeargle, lent ----
PainterLoft_EventScript_First::
	msgbox PainterLoft_Text_FirstIntro
	msgbox PainterLoft_Text_FirstAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PainterLoft_EventScript_Later
	special Painter_CheckRoom
	goto_if_eq VAR_RESULT, FALSE, PainterLoft_EventScript_NoRoom
	special Painter_PrepareFirst
	msgbox PainterLoft_Text_FirstGo
	release
	end

PainterLoft_EventScript_NoRoom::
	msgbox PainterLoft_Text_NoRoom
	release
	end

@ ---- a Sketch quest ----
PainterLoft_EventScript_Sketch::
	msgbox PainterLoft_Text_SketchIntro
	msgbox PainterLoft_Text_SketchAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PainterLoft_EventScript_Later
PainterLoft_EventScript_ChooseType::
	message PainterLoft_Text_WhichType
	waitmessage
	dynmultichoice 0, 0, FALSE, 6, 0, DYN_MULTICHOICE_CB_NONE, PainterLoft_Text_Normal, PainterLoft_Text_Fire, PainterLoft_Text_Water, PainterLoft_Text_Grass, PainterLoft_Text_Electric, PainterLoft_Text_Ice, PainterLoft_Text_Fighting, PainterLoft_Text_Poison, PainterLoft_Text_Ground, PainterLoft_Text_Flying, PainterLoft_Text_Psychic, PainterLoft_Text_Bug, PainterLoft_Text_Rock, PainterLoft_Text_Ghost, PainterLoft_Text_Dragon, PainterLoft_Text_Dark, PainterLoft_Text_Steel, PainterLoft_Text_Fairy
	goto_if_ge VAR_RESULT, 18, PainterLoft_EventScript_Later
	copyvar VAR_0x8006, VAR_RESULT
	special Painter_FindSmeargle
	goto_if_eq VAR_RESULT, TRUE, PainterLoft_EventScript_SketchReady
	msgbox PainterLoft_Text_WhichSlot
	fadescreen FADE_TO_BLACK
	special MoveDeleterChooseMoveToForget
	fadescreen FADE_FROM_BLACK
	goto_if_eq VAR_0x8005, MAX_MON_MOVES, PainterLoft_EventScript_Later
	special BufferMoveDeleterNicknameAndMove
	msgbox PainterLoft_Text_ConfirmSlot, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PainterLoft_EventScript_Later
PainterLoft_EventScript_SketchReady::
	special Painter_PrepareSketch
	msgbox PainterLoft_Text_SketchGo
	release
	end

@ ---- the Mega's painting ----
PainterLoft_EventScript_Mega::
	msgbox PainterLoft_Text_MegaIntro
	msgbox PainterLoft_Text_MegaAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PainterLoft_EventScript_Later
PainterLoft_EventScript_MegaReady::
	special Painter_FindSmeargle
	special Painter_PrepareMega
	msgbox PainterLoft_Text_MegaGo
	release
	end

PainterLoft_EventScript_Both::
	message PainterLoft_Text_BothAsk
	waitmessage
	dynmultichoice 0, 0, FALSE, 3, 0, DYN_MULTICHOICE_CB_NONE, PainterLoft_Text_ANewMove, PainterLoft_Text_TheMega
	goto_if_eq VAR_RESULT, 0, PainterLoft_EventScript_ChooseType
	goto_if_eq VAR_RESULT, 1, PainterLoft_EventScript_MegaReady
	goto PainterLoft_EventScript_Later

@ ---- the easel ----
PainterLoft_EventScript_Easel::
	lockall
	special Painter_GetPending
	goto_if_eq VAR_RESULT, PAINTING_MODE_NONE, PainterLoft_EventScript_EaselBlank
	special Painter_CanEnter
	switch VAR_RESULT
	case PAINTER_ENTER_NO_ROOM, PainterLoft_EventScript_EaselNoRoom
	case PAINTER_ENTER_NO_SMEARGLE, PainterLoft_EventScript_EaselNoSmeargle
	case PAINTER_ENTER_TOO_FEW, PainterLoft_EventScript_EaselTooFew
	msgbox PainterLoft_Text_EaselAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PainterLoft_EventScript_EaselNotYet
	special Painter_GetPending
	call_if_eq VAR_RESULT, PAINTING_MODE_FIRST, PainterLoft_EventScript_Lend
	msgbox PainterLoft_Text_SteppingIn
	closemessage
	playse SE_WARP_IN
	special Painter_Enter
	waitstate
	releaseall
	end

PainterLoft_EventScript_Lend::
	textcolor NPC_TEXT_COLOR_FEMALE
	msgbox PainterLoft_Text_Lend
	textcolor NPC_TEXT_COLOR_NEUTRAL
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	message PainterLoft_Text_Borrowed
	waitmessage
	waitfanfare
	return

PainterLoft_EventScript_EaselBlank::
	msgbox PainterLoft_Text_EaselBlank
	releaseall
	end

PainterLoft_EventScript_EaselNotYet::
	releaseall
	end

PainterLoft_EventScript_EaselNoRoom::
	msgbox PainterLoft_Text_NoRoom
	releaseall
	end

PainterLoft_EventScript_EaselNoSmeargle::
	msgbox PainterLoft_Text_NeedSmeargle
	releaseall
	end

PainterLoft_EventScript_EaselTooFew::
	msgbox PainterLoft_Text_TooFew
	releaseall
	end

PainterLoft_EventScript_Smeargle::
	lock
	faceplayer
	playmoncry SPECIES_SMEARGLE, CRY_MODE_NORMAL
	msgbox PainterLoft_Text_Smeargle
	waitmoncry
	release
	end

PainterLoft_EventScript_Canvases::
	msgbox PainterLoft_Text_Canvases, MSGBOX_SIGN
	end

PainterLoft_EventScript_Paint::
	msgbox PainterLoft_Text_Paint, MSGBOX_SIGN
	end

PainterLoft_EventScript_PaintPot::
	msgbox PainterLoft_Text_PaintPot, MSGBOX_SIGN
	end

@ ---- back out of a painting (the loft's frame script) ----
PainterLoft_EventScript_Returned::
	lockall
	special HealPlayerParty
	special Painter_Return
	textcolor NPC_TEXT_COLOR_FEMALE
	switch VAR_RESULT
	case PAINTER_RETURN_FIRST_WON, PainterLoft_EventScript_FirstWon
	case PAINTER_RETURN_FIRST_LOST, PainterLoft_EventScript_FirstLost
	case PAINTER_RETURN_SKETCHED, PainterLoft_EventScript_Sketched
	case PAINTER_RETURN_NO_SKETCH, PainterLoft_EventScript_NoSketch
	case PAINTER_RETURN_SKETCH_LOST, PainterLoft_EventScript_SketchLost
	case PAINTER_RETURN_MEGA_WON, PainterLoft_EventScript_MegaWon
	case PAINTER_RETURN_MEGA_LOST, PainterLoft_EventScript_MegaLost
	msgbox PainterLoft_Text_CameBackEarly
	releaseall
	end

PainterLoft_EventScript_FirstWon::
	msgbox PainterLoft_Text_FirstWon
	special Painter_GiveSmeargle
	goto_if_eq VAR_RESULT, MON_CANT_GIVE, PainterLoft_EventScript_GiftNoRoom
	copyvar VAR_0x8005, VAR_RESULT
	removeobject 2
	textcolor NPC_TEXT_COLOR_NEUTRAL
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	message PainterLoft_Text_ReceivedSmeargle
	waitmessage
	waitfanfare
	call_if_eq VAR_0x8005, MON_GIVEN_TO_PC, PainterLoft_EventScript_GiftToPC
	textcolor NPC_TEXT_COLOR_FEMALE
	msgbox PainterLoft_Text_FirstExplain
	releaseall
	end

PainterLoft_EventScript_GiftToPC::
	msgbox PainterLoft_Text_GiftToPC
	return

PainterLoft_EventScript_GiftNoRoom::
	msgbox PainterLoft_Text_GiftNoRoom
	releaseall
	end

PainterLoft_EventScript_FirstLost::
	msgbox PainterLoft_Text_FirstLost
	releaseall
	end

PainterLoft_EventScript_Sketched::
	msgbox PainterLoft_Text_Sketched
	releaseall
	end

PainterLoft_EventScript_NoSketch::
	msgbox PainterLoft_Text_NoSketch
	releaseall
	end

PainterLoft_EventScript_SketchLost::
	msgbox PainterLoft_Text_SketchLost
	releaseall
	end

PainterLoft_EventScript_MegaWon::
	msgbox PainterLoft_Text_MegaWon
	textcolor NPC_TEXT_COLOR_NEUTRAL
	giveitem ITEM_SMEARGLITE
	setflag FLAG_PAINTER_MEGA_DONE
	textcolor NPC_TEXT_COLOR_FEMALE
	msgbox PainterLoft_Text_MegaExplain
	releaseall
	end

PainterLoft_EventScript_MegaLost::
	msgbox PainterLoft_Text_MegaLost
	releaseall
	end

@ ---- inside a painting: the figure ----
PaintWorld_EventScript_Figure::
	lock
	faceplayer
	switch VAR_PAINTING_MODE
	case PAINTING_MODE_SKETCH, PaintWorld_EventScript_FigureSketch
	case PAINTING_MODE_MEGA, PaintWorld_EventScript_FigureMega
	msgbox PaintWorld_Text_FigureFirst
	goto PaintWorld_EventScript_FigureBattle

PaintWorld_EventScript_FigureSketch::
	msgbox PaintWorld_Text_FigureSketch
	goto PaintWorld_EventScript_FigureBattle

PaintWorld_EventScript_FigureMega::
	msgbox PaintWorld_Text_FigureMega
PaintWorld_EventScript_FigureBattle::
	closemessage
	special Painter_SetupFigure
	cleartrainerflag TRAINER_GYM_CHALLENGER
	setflag FLAG_GYM_CHALLENGE_BATTLE
	trainerbattle_no_intro TRAINER_GYM_CHALLENGER, PaintWorld_Text_FigureDefeat
	clearflag FLAG_GYM_CHALLENGE_BATTLE
	specialvar VAR_RESULT, GetBattleOutcome
	special Painter_RecordBattle
	msgbox PaintWorld_Text_Dissolve
	closemessage
	fadescreen FADE_TO_WHITE
	warpsilent MAP_PEWTER_CITY_PAINTER_LOFT, 8, 4
	waitstate
	release
	end

@ ---- drafts ----
PainterLoft_Text_NoBadge::
	.string "Oh! A visitor. Mind the wet paint.\p"
	.string "I paint trainers. Well, I paint the\n"
	.string "places trainers battle in.\p"
	.string "You don't have a BADGE yet, do you?\n"
	.string "Come back once you've won one.\p"
	.string "I might have something for you.$"

PainterLoft_Text_FirstIntro::
	.string "A BADGE! Then you've battled a GYM\n"
	.string "LEADER. Good. Look at my SMEARGLE.\p"
	.string "Its tail's bare. It's never painted\n"
	.string "in a color of its own.\p"
	.string "A SMEARGLE learns by watching, and\n"
	.string "it's only ever watched me paint.\l"
	.string "It needs to see a real trainer\l"
	.string "battle. Up close. From the inside.\p"
	.string "My paintings are… a little more than\n"
	.string "paint. Step into one with SMEARGLE.\l"
	.string "Someone in there will battle you.$"

PainterLoft_Text_FirstAsk::
	.string "Will you take SMEARGLE in with you?$"

PainterLoft_Text_FirstGo::
	.string "Wonderful! I'll set a fresh canvas on\n"
	.string "the easel. Touch it when you're ready.\p"
	.string "SMEARGLE goes in first. Let it watch\n"
	.string "how you do things.$"

PainterLoft_Text_NoRoom::
	.string "Your party's full. Make room for\n"
	.string "SMEARGLE first. A PC will do.$"

PainterLoft_Text_Later::
	.string "No hurry. The paint will keep.$"

PainterLoft_Text_Ready::
	.string "The canvas is ready. Step up to the\n"
	.string "easel whenever you like.$"

PainterLoft_Text_NeedSmeargle::
	.string "Where's SMEARGLE? Bring it along;\n"
	.string "it's the one doing the learning.$"

PainterLoft_Text_TooFew::
	.string "It's a double battle in there.\n"
	.string "Bring another POKéMON along too.$"

PainterLoft_Text_Nothing::
	.string "SMEARGLE needs something new to see.\p"
	.string "Win another BADGE, then come back.\n"
	.string "New places, new paintings.$"

PainterLoft_Text_SketchIntro::
	.string "Another BADGE! SMEARGLE's been\n"
	.string "watching you. It's itching to learn.\p"
	.string "I'll paint QUICK SKETCH onto it. In\n"
	.string "the painting, it can copy any one\l"
	.string "move the figure knows. Win, and the\l"
	.string "move is its own. Lose, and the\l"
	.string "paint washes off.$"

PainterLoft_Text_SketchAsk::
	.string "Shall we paint a new move?$"

PainterLoft_Text_WhichType::
	.string "What kind of move?$"

PainterLoft_Text_WhichSlot::
	.string "Which move should QUICK SKETCH be\n"
	.string "painted over?$"

PainterLoft_Text_ConfirmSlot::
	.string "QUICK SKETCH goes over {STR_VAR_1}'s\n"
	.string "{STR_VAR_2}. If it doesn't keep a\l"
	.string "new move, {STR_VAR_2} comes back.\l"
	.string "All right?$"

PainterLoft_Text_SketchGo::
	.string "Done. The canvas is on the easel.\n"
	.string "Use QUICK SKETCH on the figure and\l"
	.string "pick whichever of its moves you like.$"

PainterLoft_Text_MegaIntro::
	.string "The POKéMON LEAGUE! You've seen\n"
	.string "everything a trainer can see.\p"
	.string "There's one painting I've never let\n"
	.string "anyone into. Someone in it carries\l"
	.string "a stone in every color there is.$"

PainterLoft_Text_MegaAsk::
	.string "Want to go and win it?$"

PainterLoft_Text_MegaGo::
	.string "It's on the easel. Take SMEARGLE.\n"
	.string "It should see this.$"

PainterLoft_Text_BothAsk::
	.string "What shall we paint today?$"

PainterLoft_Text_ANewMove::
	.string "A NEW MOVE$"

PainterLoft_Text_TheMega::
	.string "THE STONE$"

PainterLoft_Text_EaselBlank::
	.string "A canvas on an easel. The paint is\n"
	.string "still wet.$"

PainterLoft_Text_EaselAsk::
	.string "The paint on the canvas is moving…\n"
	.string "Step into the painting?$"

PainterLoft_Text_Lend::
	.string "Here, SMEARGLE. Go with them.$"

PainterLoft_Text_Borrowed::
	.string "{PLAYER} borrowed SMEARGLE!$"

PainterLoft_Text_SteppingIn::
	.string "{PLAYER} stepped into the painting!$"

PainterLoft_Text_Smeargle::
	.string "SMEARGLE: Smea…$"

PainterLoft_Text_Canvases::
	.string "Canvases, stacked against the wall.\n"
	.string "Every one is a different place.$"

PainterLoft_Text_Paint::
	.string "Spilled paint. It's still wet.$"

PainterLoft_Text_PaintPot::
	.string "A pot of paint, with a brush in it.$"

PainterLoft_Text_CameBackEarly::
	.string "Back already? The painting will be\n"
	.string "there whenever you want to go in.$"

PainterLoft_Text_FirstWon::
	.string "You won! And look at SMEARGLE's tail!\p"
	.string "It's found its color… and it's yours.\n"
	.string "It was watching you, not the battle.\p"
	.string "Then it should stay with you. Take\n"
	.string "it. Take good care of my SMEARGLE.$"

PainterLoft_Text_ReceivedSmeargle::
	.string "{PLAYER} received SMEARGLE!$"

PainterLoft_Text_GiftToPC::
	.string "SMEARGLE was sent to the PC.$"

PainterLoft_Text_GiftNoRoom::
	.string "Oh, there's no room at all. Make\n"
	.string "some, and I'll have it ready.$"

PainterLoft_Text_FirstExplain::
	.string "It knows MASTERSTROKE: it paints\n"
	.string "with its own color, your type.\p"
	.string "After every BADGE, come back. I'll\n"
	.string "paint QUICK SKETCH onto it, and it\l"
	.string "can learn something new in a\l"
	.string "painting.$"

PainterLoft_Text_FirstLost::
	.string "Oh dear. That figure paints hard.\p"
	.string "SMEARGLE still saw plenty, though.\n"
	.string "Rest up and try again.$"

PainterLoft_Text_Sketched::
	.string "SMEARGLE kept {STR_VAR_1}!\n"
	.string "It's in its colors for good now.$"

PainterLoft_Text_NoSketch::
	.string "You won, but SMEARGLE didn't sketch\n"
	.string "a thing! The old move's back.\l"
	.string "Try again whenever you like.$"

PainterLoft_Text_SketchLost::
	.string "The paint washed off. Its old move's\n"
	.string "back. Try again whenever you like.$"

PainterLoft_Text_MegaWon::
	.string "You brought it back! The stone in\n"
	.string "every color. Here, it's yours.$"

PainterLoft_Text_MegaExplain::
	.string "Keep it in your BAG, and SMEARGLE\n"
	.string "can MEGA EVOLVE. No need to hold it:\l"
	.string "it's not a stone you wear. It's one\l"
	.string "you carry.$"

PainterLoft_Text_MegaLost::
	.string "That one's hard to paint over. Rest\n"
	.string "up. The painting's still there.$"

PaintWorld_Text_FigureFirst::
	.string "… … …\p"
	.string "A figure made of paint turns to\n"
	.string "face you. It raises a brush.$"

PaintWorld_Text_FigureSketch::
	.string "… … …\p"
	.string "The figure's colors swirl.\n"
	.string "It's waiting for you.$"

PaintWorld_Text_FigureMega::
	.string "… … …\p"
	.string "The figure is holding a stone that\n"
	.string "shines in every color.$"

PaintWorld_Text_FigureDefeat::
	.string "… …$"

PaintWorld_Text_Dissolve::
	.string "The figure runs like wet paint…\n"
	.string "The world around it does too.$"

PainterLoft_Text_Normal::
	.string "NORMAL$"
PainterLoft_Text_Fire::
	.string "FIRE$"
PainterLoft_Text_Water::
	.string "WATER$"
PainterLoft_Text_Grass::
	.string "GRASS$"
PainterLoft_Text_Electric::
	.string "ELECTRIC$"
PainterLoft_Text_Ice::
	.string "ICE$"
PainterLoft_Text_Fighting::
	.string "FIGHTING$"
PainterLoft_Text_Poison::
	.string "POISON$"
PainterLoft_Text_Ground::
	.string "GROUND$"
PainterLoft_Text_Flying::
	.string "FLYING$"
PainterLoft_Text_Psychic::
	.string "PSYCHIC$"
PainterLoft_Text_Bug::
	.string "BUG$"
PainterLoft_Text_Rock::
	.string "ROCK$"
PainterLoft_Text_Ghost::
	.string "GHOST$"
PainterLoft_Text_Dragon::
	.string "DRAGON$"
PainterLoft_Text_Dark::
	.string "DARK$"
PainterLoft_Text_Steel::
	.string "STEEL$"
PainterLoft_Text_Fairy::
	.string "FAIRY$"
