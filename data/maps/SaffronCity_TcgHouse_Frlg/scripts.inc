@ Saffron City's card house: Grandmaster ROD and the Trading Card Game (src/tcg/).
@ The player picks one of 7 decks (VAR_0x8004: TCG_DECK_GRASS 1 .. TCG_DECK_COLORLESS 7, as in
@ include/constants/tcg_cards.h) and duels ROD's Legendary Dragonite deck; a win gives that deck's
@ energy card (ITEM_GRASS_ENERGY .. ITEM_DOUBLE_COLORLESS_ENERGY, in the same order). Rematches
@ are unlimited. FLAG_MET_ROD: he's introduced himself (after that, a shorter greeting).

SaffronCity_TcgHouse_Frlg_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, SaffronCity_TcgHouse_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, SaffronCity_TcgHouse_OnResume
	.byte 0

@ the house's music: the Pokémon Dome's, from the card game (src/tcg/sound.c)
SaffronCity_TcgHouse_OnTransition:
SaffronCity_TcgHouse_OnResume:
	special PlayTcgHouseMusic
	end

SaffronCity_TcgHouse_EventScript_Rod::
	lock
	faceplayer
	goto_if_set FLAG_MET_ROD, SaffronCity_TcgHouse_EventScript_RodAgain
	msgbox SaffronCity_TcgHouse_Text_Intro
	setflag FLAG_MET_ROD
	msgbox SaffronCity_TcgHouse_Text_WantToDuel, MSGBOX_YESNO
	goto SaffronCity_TcgHouse_EventScript_Answer

SaffronCity_TcgHouse_EventScript_RodAgain::
	msgbox SaffronCity_TcgHouse_Text_WantToDuelAgain, MSGBOX_YESNO
SaffronCity_TcgHouse_EventScript_Answer::
	goto_if_eq VAR_RESULT, NO, SaffronCity_TcgHouse_EventScript_Declined
	message SaffronCity_TcgHouse_Text_ChooseDeck
	waitmessage
	dynmultichoice 0, 0, FALSE, 7, 0, DYN_MULTICHOICE_CB_NONE, SaffronCity_TcgHouse_Text_DeckGrass, SaffronCity_TcgHouse_Text_DeckFire, SaffronCity_TcgHouse_Text_DeckWater, SaffronCity_TcgHouse_Text_DeckLightning, SaffronCity_TcgHouse_Text_DeckPsychic, SaffronCity_TcgHouse_Text_DeckFighting, SaffronCity_TcgHouse_Text_DeckColorless
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, SaffronCity_TcgHouse_EventScript_Declined
	copyvar VAR_0x8004, VAR_RESULT
	addvar VAR_0x8004, 1
	msgbox SaffronCity_TcgHouse_Text_LetsDuel
	closemessage
	special StartTcgDuel
	waitstate
	lock
	faceplayer
	goto_if_ne VAR_RESULT, 0, SaffronCity_TcgHouse_EventScript_Lost
	msgbox SaffronCity_TcgHouse_Text_YouWon
	copyvar VAR_0x8005, VAR_0x8004
	addvar VAR_0x8005, ITEM_GRASS_ENERGY - 1
	giveitem VAR_0x8005
	msgbox SaffronCity_TcgHouse_Text_AboutEnergy
	release
	end

SaffronCity_TcgHouse_EventScript_Lost::
	msgbox SaffronCity_TcgHouse_Text_YouLost
	release
	end

SaffronCity_TcgHouse_EventScript_Declined::
	msgbox SaffronCity_TcgHouse_Text_Declined
	release
	end

SaffronCity_TcgHouse_Text_Intro::
	.string "…Hm? A visitor.\p"
	.string "Most people walk right past this\n"
	.string "house. Their loss.\p"
	.string "The name's ROD. Grandmaster of the\n"
	.string "POKéMON TRADING CARD GAME.\p"
	.string "You battle with real POKéMON, right?\n"
	.string "Cute. I respect it.\p"
	.string "But out here on the table, it's all\n"
	.string "you. Sixty cards. Six PRIZES.\l"
	.string "Every draw is a choice you own.\p"
	.string "No luck in the world beats a deck\n"
	.string "that's built with heart.\p"
	.string "…Sorry. I get carried away.\n"
	.string "This game is kind of my whole life.$"

SaffronCity_TcgHouse_Text_WantToDuel::
	.string "So. Think you can take a PRIZE card\n"
	.string "off me? Want to duel?$"

SaffronCity_TcgHouse_Text_WantToDuelAgain::
	.string "ROD: Back for more?\n"
	.string "Good. The cards remember you.\p"
	.string "Want to duel?$"

SaffronCity_TcgHouse_Text_ChooseDeck::
	.string "Pick a deck from my collection.\n"
	.string "Every one of them is a classic.$"

SaffronCity_TcgHouse_Text_LetsDuel::
	.string "Nice pick. Shuffle up.\p"
	.string "I'll be playing my LEGENDARY\n"
	.string "DRAGONITE deck. Don't hold back!$"

SaffronCity_TcgHouse_Text_YouWon::
	.string "ROD: …Heh. You actually did it.\p"
	.string "Haven't had a duel like that in a\n"
	.string "long time. That was fun. Seriously.\p"
	.string "Here. A winner keeps one of the\n"
	.string "deck's ENERGY cards. House rule.$"

SaffronCity_TcgHouse_Text_AboutEnergy::
	.string "Those ENERGY cards aren't just\n"
	.string "paper, either.\p"
	.string "Use one on a POKéMON and it powers\n"
	.string "up one of its moves for good.\p"
	.string "Win again, and I'll hand over\n"
	.string "another. I'm always up for a duel.$"

SaffronCity_TcgHouse_Text_YouLost::
	.string "ROD: Good game. Really.\p"
	.string "You've got the instincts. Now just\n"
	.string "learn to trust your deck.\p"
	.string "Come back whenever you want a\n"
	.string "rematch. I'll be right here.$"

SaffronCity_TcgHouse_Text_Declined::
	.string "ROD: No rush.\n"
	.string "The table's not going anywhere.$"

SaffronCity_TcgHouse_Text_DeckGrass::
	.string "Flower Power$"

SaffronCity_TcgHouse_Text_DeckFire::
	.string "Fire Charge$"

SaffronCity_TcgHouse_Text_DeckWater::
	.string "Go Go Rain Dance$"

SaffronCity_TcgHouse_Text_DeckLightning::
	.string "Zapping Selfdestruct$"

SaffronCity_TcgHouse_Text_DeckPsychic::
	.string "Strange Psyshock$"

SaffronCity_TcgHouse_Text_DeckFighting::
	.string "First-Strike$"

SaffronCity_TcgHouse_Text_DeckColorless::
	.string "Colorless$"
