@ Saffron City's card house: Grandmaster Rod and the Trading Card Game (src/tcg/).
@ The player picks one of 7 decks (VAR_0x8004: TCG_DECK_GRASS 1 .. TCG_DECK_COLORLESS 7, as in
@ include/constants/tcg_cards.h) and duels Rod's Legendary Dragonite deck; a win gives that deck's
@ energy card (ITEM_GRASS_ENERGY .. ITEM_DOUBLE_COLORLESS_ENERGY, in the same order). Rematches
@ are unlimited.
@ PLACEHOLDER TEXT: every _Text_ below is a stand-in for Nick's dialogue.

SaffronCity_TcgHouse_Frlg_MapScripts::
	.byte 0

SaffronCity_TcgHouse_EventScript_Rod::
	lock
	faceplayer
	msgbox SaffronCity_TcgHouse_Text_WantToDuel, MSGBOX_YESNO
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
	msgbox SaffronCity_TcgHouse_Text_ComeAgain
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

SaffronCity_TcgHouse_Text_WantToDuel::
	.string "ROD: PLACEHOLDER: I am ROD, a\n"
	.string "Grandmaster of the card game.\p"
	.string "Will you duel me?$"

SaffronCity_TcgHouse_Text_ChooseDeck::
	.string "PLACEHOLDER: Which deck will you use?$"

SaffronCity_TcgHouse_Text_LetsDuel::
	.string "PLACEHOLDER: Then let us begin!$"

SaffronCity_TcgHouse_Text_YouWon::
	.string "PLACEHOLDER: Splendid! Take this\n"
	.string "card as proof of your victory.$"

SaffronCity_TcgHouse_Text_YouLost::
	.string "PLACEHOLDER: A fine effort.\n"
	.string "Come and duel me again any time.$"

SaffronCity_TcgHouse_Text_ComeAgain::
	.string "PLACEHOLDER: I will be here whenever\n"
	.string "you want another duel.$"

SaffronCity_TcgHouse_Text_Declined::
	.string "PLACEHOLDER: Come back when you\n"
	.string "are ready.$"

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
