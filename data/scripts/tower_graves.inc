@ Pokémon Canvas: the Pokémon Tower's graves and the ghost girl (src/tower_graves.c;
@ tools/gym_hack/build_tower_graves.py places the graves).
@ Each grave shows its Pokémon, its name and its trainer. Ten were Pokémon that could have evolved
@ and never did (VAR_TOWER_GRAVES: one bit each); reading the tenth brings the ghost girl and her
@ SCYTHER, and the game's one Eviolate (FLAG_GOT_EVIOLATE).
@ (Claude's draft of the girl's words, Nov 5: Nick may rewrite them.)

@ (2F-4F's ON_TRANSITION) the girl and her SCYTHER stay hidden until they're called
TowerGraves_EventScript_HideGhosts::
	setflag FLAG_TEMP_1F
	end

@ VAR_0x8004 = the grave
TowerGraves_EventScript_Read::
	lockall
	special TowerGraves_Read
	copyvar VAR_0x8006, VAR_RESULT
	showmonpic VAR_0x8005, 10, 3
	msgbox TowerGraves_Text_Grave
	hidemonpic
	goto_if_eq VAR_0x8006, TRUE, TowerGraves_EventScript_GhostGirl
	releaseall
	end

TowerGraves_EventScript_GhostGirl::
	delay 30
	playse SE_M_NIGHTMARE
	special TowerGraves_SummonGhosts
	waitstate
	playmoncry SPECIES_SCYTHER, CRY_MODE_WEAK
	waitmoncry
	msgbox TowerGraves_Text_GirlLament
	checkitemspace ITEM_EVIOLATE
	goto_if_eq VAR_RESULT, FALSE, TowerGraves_EventScript_NoRoom
	giveitem ITEM_EVIOLATE
	setflag FLAG_GOT_EVIOLATE
	msgbox TowerGraves_Text_GirlKeepThemAsTheyAre
	closemessage
	special TowerGraves_DismissGhosts
	waitstate
	msgbox TowerGraves_Text_Gone
	releaseall
	end

TowerGraves_EventScript_NoRoom::
	msgbox TowerGraves_Text_GirlNoRoom
	closemessage
	special TowerGraves_DismissGhosts
	waitstate
	releaseall
	end

@ (the girl or her SCYTHER, if ever spoken to: they're only there while she speaks)
TowerGraves_EventScript_Ghost::
	end

TowerGraves_Text_Grave::
	.string "{STR_VAR_1}\n"
	.string "Trainer: {STR_VAR_2}$"

TowerGraves_Text_GirlLament::
	.string "…You found all ten of them.\p"
	.string "Ten little ones who never grew up.\n"
	.string "Nobody ever reads their names.\p"
	.string "Everyone wants their POKéMON\n"
	.string "to evolve. Bigger. Stronger.\p"
	.string "But I always wondered…\p"
	.string "When it evolves, what happens\n"
	.string "to the little one it used to be?\p"
	.string "Is it still in there somewhere?\n"
	.string "Or does it just… go away?\p"
	.string "…Does it die?\p"
	.string "My SCYTHER never changed.\n"
	.string "Not once. And it's still here.\l"
	.string "Still with me.\p"
	.string "You remembered them as they were.\n"
	.string "So this is yours now.$"

TowerGraves_Text_GirlKeepThemAsTheyAre::
	.string "Let a POKéMON that hasn't evolved\n"
	.string "hold it. It'll hit so much harder.\p"
	.string "Keep your POKéMON as it is.\p"
	.string "It's powerful enough.\n"
	.string "It always was…$"

TowerGraves_Text_GirlNoRoom::
	.string "…Your bag is too full to hold it.\p"
	.string "Come back to them.\n"
	.string "I'll be waiting…$"

TowerGraves_Text_Gone::
	.string "…The girl and her SCYTHER were gone.$"
