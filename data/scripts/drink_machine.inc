@ Pokémon Canvas: the Drink Machine, a gym decoration (src/drink_machine.c has the recipes).
@ Talking to it (src/field_control_avatar.c): a drink, then up to two berries, then MIX. Nothing's
@ used up unless the mix makes something. Text here is a draft: Nick writes the final lines.

DrinkMachine_EventScript_Use::
	lockall
	msgbox DrinkMachine_Text_Intro, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, DrinkMachine_EventScript_End
	special DrinkMachine_Begin
	special DrinkMachine_PushDrinks
	goto_if_eq VAR_RESULT, 0, DrinkMachine_EventScript_NoDrinks
	message DrinkMachine_Text_WhichDrink
	waitmessage
	dynmultistack 0, 0, FALSE, 4, FALSE, 0, NULL
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, DrinkMachine_EventScript_End
	special DrinkMachine_PickDrink
DrinkMachine_EventScript_AddBerry::
	special DrinkMachine_PushBerries
	goto_if_ne VAR_RESULT, 0, DrinkMachine_EventScript_ChooseBerry
	goto_if_eq VAR_0x8005, 0, DrinkMachine_EventScript_NoBerries
	goto DrinkMachine_EventScript_Mix

DrinkMachine_EventScript_ChooseBerry::
	call_if_eq VAR_0x8005, 0, DrinkMachine_EventScript_AskFirstBerry
	call_if_ne VAR_0x8005, 0, DrinkMachine_EventScript_AskNextBerry
	dynmultistack 0, 0, FALSE, 6, FALSE, 0, NULL
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, DrinkMachine_EventScript_Cancel
	special DrinkMachine_PickBerry
	goto_if_eq VAR_RESULT, FALSE, DrinkMachine_EventScript_AddBerry
DrinkMachine_EventScript_Mix::
	closemessage
	playse SE_VEND
	waitse
	special DrinkMachine_Mix
	switch VAR_RESULT
	case DRINK_MIX_MADE, DrinkMachine_EventScript_Made
	case DRINK_MIX_SODA, DrinkMachine_EventScript_Soda
	case DRINK_MIX_NO_ROOM, DrinkMachine_EventScript_NoRoom
	msgbox DrinkMachine_Text_Nothing
	goto DrinkMachine_EventScript_End

DrinkMachine_EventScript_AskFirstBerry::
	message DrinkMachine_Text_WhichBerry
	waitmessage
	return

DrinkMachine_EventScript_AskNextBerry::
	message DrinkMachine_Text_AnotherBerry
	waitmessage
	return

DrinkMachine_EventScript_Made::
	playfanfare MUS_OBTAIN_ITEM
	message DrinkMachine_Text_Made
	waitmessage
	waitfanfare
	msgbox DrinkMachine_Text_PutAway
	goto DrinkMachine_EventScript_End

DrinkMachine_EventScript_NoRoom::
	msgbox DrinkMachine_Text_NoRoom
	goto DrinkMachine_EventScript_End

DrinkMachine_EventScript_Soda::
	msgbox DrinkMachine_Text_SodaFizzes
	special ChoosePartyMon
	goto_if_ge VAR_0x8004, PARTY_SIZE, DrinkMachine_EventScript_SodaKept
	special DrinkMachine_ApplySoda
	switch VAR_RESULT
	case DRINK_SODA_DONE, DrinkMachine_EventScript_SodaDone
	case DRINK_SODA_FIRST_TYPE, DrinkMachine_EventScript_SodaFirstType
	case DRINK_SODA_ALREADY, DrinkMachine_EventScript_SodaAlready
	msgbox DrinkMachine_Text_SodaCant
	goto DrinkMachine_EventScript_End

DrinkMachine_EventScript_SodaDone::
	playfanfare MUS_LEVEL_UP
	message DrinkMachine_Text_SodaDone
	waitmessage
	waitfanfare
	msgbox DrinkMachine_Text_SodaDone2
	goto DrinkMachine_EventScript_End

DrinkMachine_EventScript_SodaFirstType::
	msgbox DrinkMachine_Text_SodaFirstType
	goto DrinkMachine_EventScript_End

DrinkMachine_EventScript_SodaAlready::
	msgbox DrinkMachine_Text_SodaAlready
	goto DrinkMachine_EventScript_End

DrinkMachine_EventScript_SodaKept::
	msgbox DrinkMachine_Text_SodaKept
	goto DrinkMachine_EventScript_End

DrinkMachine_EventScript_NoDrinks::
	msgbox DrinkMachine_Text_NoDrinks
	goto DrinkMachine_EventScript_End

DrinkMachine_EventScript_NoBerries::
	msgbox DrinkMachine_Text_NoBerries
	goto DrinkMachine_EventScript_End

DrinkMachine_EventScript_Cancel::
	msgbox DrinkMachine_Text_Cancel
DrinkMachine_EventScript_End::
	closemessage
	releaseall
	end

DrinkMachine_Text_Intro::
	.string "It's the DRINK MACHINE!\n"
	.string "Mix something?$"

DrinkMachine_Text_WhichDrink::
	.string "Put in which drink?$"

DrinkMachine_Text_WhichBerry::
	.string "Put in which BERRY?$"

DrinkMachine_Text_AnotherBerry::
	.string "Put in another BERRY?\n"
	.string "Or MIX it now?$"

DrinkMachine_Text_Nothing::
	.string "Whirr… clunk.\p"
	.string "Nothing came out. That mix\n"
	.string "didn't work!\p"
	.string "The machine gave everything back.$"

DrinkMachine_Text_Made::
	.string "Whirr… clunk!\n"
	.string "Out came a {STR_VAR_1}!$"

DrinkMachine_Text_PutAway::
	.string "{PLAYER} put the {STR_VAR_1}\n"
	.string "in the BAG.$"

DrinkMachine_Text_NoRoom::
	.string "There's no room in the BAG for a\n"
	.string "{STR_VAR_1}!\p"
	.string "The machine gave everything back.$"

DrinkMachine_Text_SodaFizzes::
	.string "Whirr… fizz!\n"
	.string "The SODA is fizzing like crazy!\p"
	.string "It looks {STR_VAR_1}-type now!\n"
	.string "Give it to which POKéMON?$"

DrinkMachine_Text_SodaDone::
	.string "{STR_VAR_1} drank the whole thing!\n"
	.string "{STR_VAR_1} became {STR_VAR_2}-type!$"

DrinkMachine_Text_SodaDone2::
	.string "{STR_VAR_2} is its second type now,\n"
	.string "even when it evolves!$"

DrinkMachine_Text_SodaFirstType::
	.string "{STR_VAR_1} is already\n"
	.string "{STR_VAR_2}-type!\p"
	.string "It won't drink it. The machine gave\n"
	.string "everything back.$"

DrinkMachine_Text_SodaAlready::
	.string "{STR_VAR_1}'s second type is\n"
	.string "already {STR_VAR_2}!\p"
	.string "The machine gave everything back.$"

DrinkMachine_Text_SodaCant::
	.string "That can't drink it!\p"
	.string "The machine gave everything back.$"

DrinkMachine_Text_SodaKept::
	.string "The machine gave everything back.$"

DrinkMachine_Text_NoDrinks::
	.string "There's no drink in the BAG to\n"
	.string "put in!$"

DrinkMachine_Text_NoBerries::
	.string "There aren't any BERRIES in the BAG\n"
	.string "that go with that!$"

DrinkMachine_Text_Cancel::
	.string "The machine gave everything back.$"
