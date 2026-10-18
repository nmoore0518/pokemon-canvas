@ Gym Trainer hack: the Pallet Town Gym PC (buying decorations and decorating). See src/gym_decor.c.

GymDecor_EventScript_GymPC::
	lockall
	playse SE_PC_ON
	msgbox GymDecor_Text_BootedUpGymPC, MSGBOX_DEFAULT
GymDecor_EventScript_PCMenu::
	message GymDecor_Text_WhatToDo
	waitmessage
	dynmultichoice 0, 0, FALSE, 3, 0, DYN_MULTICHOICE_CB_NONE, GymDecor_Text_Buy, GymDecor_Text_Decorate, GymDecor_Text_LogOff
	switch VAR_RESULT
	case 0, GymDecor_EventScript_Buy
	case 1, GymDecor_EventScript_Decorate
	goto GymDecor_EventScript_LogOff
	end

GymDecor_EventScript_LogOff::
	playse SE_PC_OFF
	closemessage
	releaseall
	end

@ ---- Buying -------------------------------------------------------------
GymDecor_EventScript_Buy::
	setvar VAR_0x8005, 0
	showmoneybox 0, 0
GymDecor_EventScript_BuyLoop::
	special GymDecor_PushShopList
	message GymDecor_Text_WhichToBuy
	waitmessage
	dynmultistack 15, 0, FALSE, 5, FALSE, VAR_0x8005, DYN_MULTICHOICE_CB_GYM_DECOR_SHOP
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, GymDecor_EventScript_BuyDone
	copyvar VAR_0x8004, VAR_RESULT
	copyvar VAR_0x8005, VAR_RESULT
	special GymDecor_BufferDecorInfo
	msgbox GymDecor_Text_ConfirmBuy, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, GymDecor_EventScript_BuyLoop
	special GymDecor_TryBuy
	goto_if_eq VAR_RESULT, 1, GymDecor_EventScript_NotEnoughMoney
	goto_if_eq VAR_RESULT, 2, GymDecor_EventScript_StorageFull
	updatemoneybox
	special GymDecor_BufferDecorInfo
	playse SE_SHOP
	msgbox GymDecor_Text_Bought, MSGBOX_DEFAULT
	goto GymDecor_EventScript_BuyLoop
	end

GymDecor_EventScript_NotEnoughMoney::
	msgbox GymDecor_Text_NotEnoughMoney, MSGBOX_DEFAULT
	goto GymDecor_EventScript_BuyLoop
	end

GymDecor_EventScript_StorageFull::
	msgbox GymDecor_Text_StorageFull, MSGBOX_DEFAULT
	goto GymDecor_EventScript_BuyLoop
	end

GymDecor_EventScript_BuyDone::
	hidemoneybox
	goto GymDecor_EventScript_PCMenu
	end

@ ---- Decorating ---------------------------------------------------------
GymDecor_EventScript_Decorate::
	special GymDecor_PushOwnedList
	goto_if_eq VAR_RESULT, 0, GymDecor_EventScript_NothingOwned
	message GymDecor_Text_WhichToPlace
	waitmessage
	dynmultistack 15, 0, FALSE, 5, FALSE, 0, DYN_MULTICHOICE_CB_GYM_DECOR_OWNED
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, GymDecor_EventScript_PCMenu
	copyvar VAR_0x8004, VAR_RESULT
	msgbox GymDecor_Text_HowToDecorate, MSGBOX_DEFAULT
	closemessage
	special GymDecor_StartEditing
	releaseall
	end

GymDecor_EventScript_NothingOwned::
	msgbox GymDecor_Text_NothingOwned, MSGBOX_DEFAULT
	goto GymDecor_EventScript_PCMenu
	end

GymDecor_EventScript_EditDone::
	lockall
	msgbox GymDecor_Text_LayoutSaved, MSGBOX_DEFAULT
	releaseall
	end

@ The player boxed themselves in (or stands in a decoration): bring them back to the PC
GymDecor_EventScript_EditDoneReturnToPC::
	lockall
	msgbox GymDecor_Text_LayoutSavedBackToPC, MSGBOX_DEFAULT
	closemessage
	warp MAP_PALLET_TOWN_GYM, GYM_PC_FRONT_X, GYM_PC_FRONT_Y
	waitstate
	releaseall
	end

@ ---- Text ---------------------------------------------------------------
GymDecor_Text_BootedUpGymPC::
	.string "{PLAYER} booted up the GYM PC.$"

GymDecor_Text_WhatToDo::
	.string "What would you like to do?$"

GymDecor_Text_Buy::
	.string "BUY DECORATIONS$"

GymDecor_Text_Decorate::
	.string "DECORATE$"

GymDecor_Text_LogOff::
	.string "LOG OFF$"

GymDecor_Text_WhichToBuy::
	.string "Which decoration would you\n"
	.string "like to buy?$"

GymDecor_Text_ConfirmBuy::
	.string "{STR_VAR_1}?\n"
	.string "That'll be ¥{STR_VAR_2}. OK?$"

GymDecor_Text_Bought::
	.string "The {STR_VAR_1} was sent to\n"
	.string "the GYM's storage.$"

GymDecor_Text_NotEnoughMoney::
	.string "You don't have enough money.$"

GymDecor_Text_StorageFull::
	.string "The GYM can't store any more\n"
	.string "of those.$"

GymDecor_Text_WhichToPlace::
	.string "Which decoration will you\n"
	.string "start with?$"

GymDecor_Text_HowToDecorate::
	.string "Walk anywhere in the GYM. The\n"
	.string "decoration appears in front of you.\p"
	.string "A: place it.\n"
	.string "SELECT: put away the one in front.\p"
	.string "L and R: switch decorations.\n"
	.string "B: finish decorating.$"

GymDecor_Text_NothingOwned::
	.string "There aren't any decorations in\n"
	.string "storage. Buy some first!$"

GymDecor_Text_LayoutSaved::
	.string "The GYM's new look is saved!$"

GymDecor_Text_LayoutSavedBackToPC::
	.string "The GYM's new look is saved!\p"
	.string "Better head back to the PC from\n"
	.string "here…$"
