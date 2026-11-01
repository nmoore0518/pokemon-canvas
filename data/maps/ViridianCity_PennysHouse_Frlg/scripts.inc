@ Pokémon Canvas: Penny's house (tools/gym_hack/build_pennys_house.py; src/penny.c). Nothing here is
@ needed for the story. She asks for an Eevee; shown one, she says how Eevee evolves into the gym's
@ type and gives the item; once the gym is the player's (VAR_GYM_FINALE), any type, from a menu.
@ (Her lines are drafts.)
ViridianCity_PennysHouse_Frlg_MapScripts::
	.byte 0

ViridianCity_PennysHouse_EventScript_Penny::
	lock
	faceplayer
	goto_if_set FLAG_PENNY_MET, ViridianCity_PennysHouse_EventScript_PennyAgain
	msgbox ViridianCity_PennysHouse_Text_Intro
	setflag FLAG_PENNY_MET
	goto ViridianCity_PennysHouse_EventScript_FirstEevee

ViridianCity_PennysHouse_EventScript_PennyAgain::
	goto_if_unset FLAG_PENNY_FIRST_GIFT, ViridianCity_PennysHouse_EventScript_FirstEevee
	goto_if_lt VAR_GYM_FINALE, GYM_FINALE_DONE, ViridianCity_PennysHouse_EventScript_NotYet
	special Penny_HasEevee
	goto_if_eq VAR_RESULT, FALSE, ViridianCity_PennysHouse_EventScript_BringAnother
	msgbox ViridianCity_PennysHouse_Text_AnotherEevee, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, ViridianCity_PennysHouse_EventScript_Maybe
	message ViridianCity_PennysHouse_Text_WhichType
	waitmessage
	call ViridianCity_PennysHouse_EventScript_PushTypes
	dynmultistack 0, 0, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, ViridianCity_PennysHouse_EventScript_Maybe
	copyvar VAR_0x8004, VAR_RESULT
	closemessage
	special Penny_WasGiven
	goto_if_eq VAR_RESULT, TRUE, ViridianCity_PennysHouse_EventScript_Already
	call ViridianCity_PennysHouse_EventScript_Teach
	release
	end

@ (the first time: an Eevee in the party, and it's the gym's type)
ViridianCity_PennysHouse_EventScript_FirstEevee::
	special Penny_HasEevee
	goto_if_eq VAR_RESULT, FALSE, ViridianCity_PennysHouse_EventScript_StillLooking
	msgbox ViridianCity_PennysHouse_Text_YouFoundOne
	copyvar VAR_0x8004, VAR_PLAYER_TYPE
	call ViridianCity_PennysHouse_EventScript_Teach
	goto_if_eq VAR_RESULT, FALSE, ViridianCity_PennysHouse_EventScript_End
	setflag FLAG_PENNY_FIRST_GIFT
	msgbox ViridianCity_PennysHouse_Text_ComeBackLater
	release
	end

@ VAR_0x8004 = the type: how Eevee becomes it, and the item. VAR_RESULT = FALSE if the bag was full
ViridianCity_PennysHouse_EventScript_Teach::
	special Penny_BufferEvo
	goto_if_eq VAR_RESULT, PENNY_EVO_FRIENDSHIP, ViridianCity_PennysHouse_EventScript_TeachFairy
	msgbox ViridianCity_PennysHouse_Text_ForThisType
	giveitem VAR_0x8005
	goto_if_eq VAR_RESULT, FALSE, ViridianCity_PennysHouse_EventScript_BagFull
	special Penny_MarkGiven
	special Penny_BufferEvo
	goto_if_eq VAR_RESULT, PENNY_EVO_USE, ViridianCity_PennysHouse_EventScript_TeachUse
	msgbox ViridianCity_PennysHouse_Text_HowHold
	setvar VAR_RESULT, TRUE
	return

ViridianCity_PennysHouse_EventScript_TeachUse::
	msgbox ViridianCity_PennysHouse_Text_HowUse
	setvar VAR_RESULT, TRUE
	return

ViridianCity_PennysHouse_EventScript_TeachFairy::
	msgbox ViridianCity_PennysHouse_Text_HowFairy
	special Penny_MarkGiven
	setvar VAR_RESULT, TRUE
	return

ViridianCity_PennysHouse_EventScript_BagFull::
	msgbox ViridianCity_PennysHouse_Text_BagFull
	setvar VAR_RESULT, FALSE
	return

ViridianCity_PennysHouse_EventScript_Already::
	special Penny_BufferEvo
	goto_if_eq VAR_RESULT, PENNY_EVO_FRIENDSHIP, ViridianCity_PennysHouse_EventScript_AlreadyFairy
	msgbox ViridianCity_PennysHouse_Text_Already
	release
	end

ViridianCity_PennysHouse_EventScript_AlreadyFairy::
	msgbox ViridianCity_PennysHouse_Text_HowFairy
	release
	end

ViridianCity_PennysHouse_EventScript_StillLooking::
	msgbox ViridianCity_PennysHouse_Text_StillLooking
	release
	end

ViridianCity_PennysHouse_EventScript_NotYet::
	msgbox ViridianCity_PennysHouse_Text_NotYet
	release
	end

ViridianCity_PennysHouse_EventScript_BringAnother::
	msgbox ViridianCity_PennysHouse_Text_BringAnother
	release
	end

ViridianCity_PennysHouse_EventScript_Maybe::
	msgbox ViridianCity_PennysHouse_Text_Maybe
ViridianCity_PennysHouse_EventScript_End::
	release
	end

ViridianCity_PennysHouse_EventScript_PushTypes::
	dynmultipush GymTera_Text_NORMAL, TYPE_NORMAL
	dynmultipush GymTera_Text_FIRE, TYPE_FIRE
	dynmultipush GymTera_Text_WATER, TYPE_WATER
	dynmultipush GymTera_Text_GRASS, TYPE_GRASS
	dynmultipush GymTera_Text_ELECTRIC, TYPE_ELECTRIC
	dynmultipush GymTera_Text_ICE, TYPE_ICE
	dynmultipush GymTera_Text_FIGHTING, TYPE_FIGHTING
	dynmultipush GymTera_Text_POISON, TYPE_POISON
	dynmultipush GymTera_Text_GROUND, TYPE_GROUND
	dynmultipush GymTera_Text_FLYING, TYPE_FLYING
	dynmultipush GymTera_Text_PSYCHIC, TYPE_PSYCHIC
	dynmultipush GymTera_Text_BUG, TYPE_BUG
	dynmultipush GymTera_Text_ROCK, TYPE_ROCK
	dynmultipush GymTera_Text_GHOST, TYPE_GHOST
	dynmultipush GymTera_Text_DRAGON, TYPE_DRAGON
	dynmultipush GymTera_Text_DARK, TYPE_DARK
	dynmultipush GymTera_Text_STEEL, TYPE_STEEL
	dynmultipush GymTera_Text_FAIRY, TYPE_FAIRY
	return

ViridianCity_PennysHouse_EventScript_Plush::
	msgbox ViridianCity_PennysHouse_Text_Plush, MSGBOX_SIGN
	end

@ ---- Penny (drafts) ----
ViridianCity_PennysHouse_Text_Intro::
	.string "Oh! Um… hi. Sorry. I don't really\n"
	.string "get visitors.\p"
	.string "…Wait. You're one of Professor\n"
	.string "OAK's gym candidates, right?\p"
	.string "I read about the new gym. A whole\n"
	.string "gym built around one type…\p"
	.string "That's… actually really cool.\n"
	.string "Like a perfectly optimized build.\p"
	.string "You know what'd be perfect for a\n"
	.string "gym trainer? EEVEE.\p"
	.string "It can evolve into a POKéMON of\n"
	.string "any type. Every type. It's like\l"
	.string "the ultimate wildcard.\p"
	.string "You can find them on ROUTE 1, just\n"
	.string "south of here. They're rare, though.\p"
	.string "If you bring me one, I'll tell you\n"
	.string "how it evolves into your gym's type.\p"
	.string "…Promise.$"

ViridianCity_PennysHouse_Text_StillLooking::
	.string "Still looking for an EEVEE?\p"
	.string "ROUTE 1, in the tall grass. They're\n"
	.string "rare, so… don't give up, okay?$"

ViridianCity_PennysHouse_Text_YouFoundOne::
	.string "Is that… an EEVEE? You found one!\p"
	.string "Can I… um, can I see it for a sec?\p"
	.string "…So fluffy. Okay. Okay, focus.\n"
	.string "A deal's a deal.$"

ViridianCity_PennysHouse_Text_ForThisType::
	.string "For a {STR_VAR_1}-type gym, you want\n"
	.string "{STR_VAR_3}. Here, take this.$"

ViridianCity_PennysHouse_Text_HowHold::
	.string "Have your EEVEE hold the\n"
	.string "{STR_VAR_2} and level it up.\p"
	.string "That's it. It'll evolve into\n"
	.string "{STR_VAR_3}. Easy.$"

ViridianCity_PennysHouse_Text_HowUse::
	.string "Use the {STR_VAR_2} on your\n"
	.string "EEVEE, and it'll evolve into\l"
	.string "{STR_VAR_3} right away.$"

ViridianCity_PennysHouse_Text_HowFairy::
	.string "{STR_VAR_3} doesn't need an item.\p"
	.string "Your EEVEE has to really trust you,\n"
	.string "and know a {STR_VAR_1}-type move.\p"
	.string "Then just level it up. That's the\n"
	.string "whole trick.$"

ViridianCity_PennysHouse_Text_BagFull::
	.string "Oh… your BAG's full. Come back when\n"
	.string "you've made some room, okay?$"

ViridianCity_PennysHouse_Text_ComeBackLater::
	.string "Once you've got a gym of your own,\n"
	.string "bring me another EEVEE.\p"
	.string "I'll help you with the other types\n"
	.string "too. …If you want.$"

ViridianCity_PennysHouse_Text_NotYet::
	.string "How's your EEVEE doing?\p"
	.string "When the gym's yours, come back\n"
	.string "with another one. I'll be here.\l"
	.string "I'm always here.$"

ViridianCity_PennysHouse_Text_BringAnother::
	.string "A real GYM LEADER… that's so cool.\p"
	.string "Bring me another EEVEE, and I'll\n"
	.string "tell you how it evolves into any\l"
	.string "type you want.$"

ViridianCity_PennysHouse_Text_AnotherEevee::
	.string "Oh, another EEVEE!\p"
	.string "Do you want to know how EEVEE\n"
	.string "evolves into another type?$"

ViridianCity_PennysHouse_Text_WhichType::
	.string "Which type?$"

ViridianCity_PennysHouse_Text_Already::
	.string "I already gave you that one. Have\n"
	.string "EEVEE use or hold the {STR_VAR_2}.\p"
	.string "It'll become {STR_VAR_3}.$"

ViridianCity_PennysHouse_Text_Maybe::
	.string "Okay. Maybe later, then.$"

ViridianCity_PennysHouse_Text_Plush::
	.string "It's an EEVEE-family plush.\n"
	.string "It's super soft.$"
