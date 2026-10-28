@ Pokémon Canvas: the Incense burned out (src/incense.c); like a Repel, another can be lit right away
EventScript_IncenseWoreOff::
	lock
	checkitem ITEM_INCENSE, 1
	goto_if_eq VAR_RESULT, FALSE, EventScript_IncenseWoreOffMessage
	msgbox Text_IncenseWoreOffAskAnother, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_IncenseRelease
	special Incense_UseAnother
	playse SE_REPEL
	waitse
	msgbox Text_IncenseLitAnother, MSGBOX_SIGN
EventScript_IncenseRelease:
	closemessage
	releaseall
	end

EventScript_IncenseWoreOffMessage:
	msgbox Text_IncenseWoreOff, MSGBOX_SIGN
	releaseall
	end

Text_IncenseWoreOff:
	.string "The INCENSE burned out.$"

Text_IncenseWoreOffAskAnother:
	.string "The INCENSE burned out.\n"
	.string "Light another one?$"

Text_IncenseLitAnother:
	.string "{PLAYER} lit another INCENSE.$"
