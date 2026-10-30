LavenderTown_House2_Frlg_MapScripts::
	.byte 0

LavenderTown_House2_EventScript_NameRater::
	lock
	faceplayer
	goto_if_unset FLAG_NAME_RATER_TIP, LavenderTown_House2_EventScript_Tip
	msgbox LavenderTown_House2_Text_WantMeToRateNicknames, MSGBOX_YESNO
LavenderTown_House2_EventScript_Answer::
	goto_if_eq VAR_RESULT, YES, LavenderTown_House2_EventScript_ChooseMon
	goto_if_eq VAR_RESULT, NO, LavenderTown_House2_EventScript_DontRateNickname
	end

@ Pokémon Canvas: the first visit, he tells you about RENAME on the summary screen
LavenderTown_House2_EventScript_Tip::
	msgbox LavenderTown_House2_Text_Tip
	setflag FLAG_NAME_RATER_TIP
	msgbox LavenderTown_House2_Text_StillWantRating, MSGBOX_YESNO
	goto LavenderTown_House2_EventScript_Answer

LavenderTown_House2_EventScript_ChooseMon::
	msgbox LavenderTown_House2_Text_CritiqueWhichMonsNickname
	special ChoosePartyMon
	goto_if_lt VAR_0x8004, PARTY_SIZE, LavenderTown_House2_EventScript_CheckCanRateMon
	goto_if_ge VAR_0x8004, PARTY_SIZE, LavenderTown_House2_EventScript_DontRateNickname
	end

LavenderTown_House2_EventScript_DontRateNickname::
	msgbox LavenderTown_House2_Text_ISeeComeVisitAgain
	release
	end

LavenderTown_House2_EventScript_CheckCanRateMon::
	specialvar VAR_RESULT, ScriptGetPartyMonSpecies
	goto_if_eq VAR_RESULT, SPECIES_EGG, LavenderTown_House2_EventScript_CantNicknameEgg
	special BufferMonNickname
	special IsMonOTIDNotPlayers
	goto_if_eq VAR_RESULT, TRUE, LavenderTown_House2_EventScript_CantNicknameTradeMon
	specialvar VAR_RESULT, MonOTNameNotPlayer
	special BufferMonNickname
	goto_if_eq VAR_RESULT, TRUE, LavenderTown_House2_EventScript_CantNicknameTradeMon
	msgbox LavenderTown_House2_Text_GiveItANicerName, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, LavenderTown_House2_EventScript_ChooseNewNickname
	goto_if_eq VAR_RESULT, NO, LavenderTown_House2_EventScript_DontRateNickname
	end

LavenderTown_House2_EventScript_CantNicknameEgg::
	msgbox LavenderTown_House2_Text_ThatIsMerelyAnEgg
	release
	end

LavenderTown_House2_EventScript_CantNicknameTradeMon::
	msgbox LavenderTown_House2_Text_TrulyImpeccableName
	release
	end

LavenderTown_House2_EventScript_ChooseNewNickname::
	msgbox LavenderTown_House2_Text_WhatShallNewNicknameBe
	call Common_EventScript_NameReceivedPartyMon
	specialvar VAR_RESULT, TryPutNameRaterShowOnTheAir
	special BufferMonNickname
	goto_if_eq VAR_RESULT, TRUE, LavenderTown_House2_EventScript_ChoseNewNickname
	msgbox LavenderTown_House2_Text_FromNowOnShallBeKnownAsSameName
	release
	end

LavenderTown_House2_EventScript_ChoseNewNickname::
	msgbox LavenderTown_House2_Text_FromNowOnShallBeKnownAsName
	release
	end

LavenderTown_House2_Text_WantMeToRateNicknames::
	.string "Hello, hello!\n"
	.string "I am the official NAME RATER!\p"
	.string "Want me to rate the nicknames of\n"
	.string "your POKéMON?$"

LavenderTown_House2_Text_CritiqueWhichMonsNickname::
	.string "Which POKéMON's nickname should\n"
	.string "I critique?$"

LavenderTown_House2_Text_GiveItANicerName::
	.string "{STR_VAR_1}, is it?\n"
	.string "That is a decent nickname!\p"
	.string "But, would you like me to give it\n"
	.string "a nicer name?\p"
	.string "How about it?$"

LavenderTown_House2_Text_WhatShallNewNicknameBe::
	.string "Ah, good. Then, what shall the new\n"
	.string "nickname be?$"

LavenderTown_House2_Text_FromNowOnShallBeKnownAsName::
	.string "Done! From now on, this POKéMON\n"
	.string "shall be known as {STR_VAR_1}!\p"
	.string "It is a better name than before!\n"
	.string "How fortunate for you!$"

LavenderTown_House2_Text_ISeeComeVisitAgain::
	.string "I see.\n"
	.string "Do come visit again.$"

LavenderTown_House2_Text_FromNowOnShallBeKnownAsSameName::
	.string "Done! From now on, this POKéMON\n"
	.string "shall be known as {STR_VAR_1}!\p"
	.string "It looks no different from before,\n"
	.string "and yet, this is vastly superior!\p"
	.string "How fortunate for you!$"

LavenderTown_House2_Text_TrulyImpeccableName::
	.string "{STR_VAR_1}, is it?\n"
	.string "That is a truly impeccable name!\p"
	.string "Take good care of {STR_VAR_1}!$"

LavenderTown_House2_Text_ThatIsMerelyAnEgg::
	.string "Now, now.\n"
	.string "That is merely an EGG!$"


LavenderTown_House2_Text_Tip::
	.string "Hello, hello!\n"
	.string "I am the official NAME RATER!\p"
	.string "Before we begin, a little secret.\n"
	.string "You don't need me to rename\l"
	.string "your POKéMON!\p"
	.string "You may give a POKéMON a new\n"
	.string "nickname whenever you like.\p"
	.string "Just open its SUMMARY, and press\n"
	.string "A on the first page to RENAME it.\p"
	.string "Of course, a POKéMON from another\n"
	.string "TRAINER keeps the name it has.\p"
	.string "Now then!$"

LavenderTown_House2_Text_StillWantRating::
	.string "Want me to rate the nicknames of\n"
	.string "your POKéMON all the same?$"
