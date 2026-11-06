@ Gym Trainer hack: the Pallet Town Gym PC (buying decorations and decorating). See src/gym_decor.c.

GymDecor_EventScript_GymPC::
	lockall
	playse SE_PC_ON
	msgbox GymDecor_Text_BootedUpGymPC, MSGBOX_DEFAULT
GymDecor_EventScript_PCMenu::
	message GymDecor_Text_WhatToDo
	waitmessage
	dynmultichoice 0, 0, FALSE, 5, 0, DYN_MULTICHOICE_CB_NONE, GymDecor_Text_Buy, GymDecor_Text_Sell, GymDecor_Text_Decorate, GymDecor_Text_PutAllAway, GymDecor_Text_LogOff
	switch VAR_RESULT
	case 0, GymDecor_EventScript_Buy
	case 1, GymDecor_EventScript_Sell
	case 2, GymDecor_EventScript_Decorate
	case 3, GymDecor_EventScript_PutAllAway
	goto GymDecor_EventScript_LogOff
	end

GymDecor_EventScript_LogOff::
	playse SE_PC_OFF
	closemessage
	releaseall
	end

@ ---- Buying -------------------------------------------------------------
GymDecor_EventScript_Buy::
	setvar VAR_0x8007, 0
	showmoneybox 0, 0
GymDecor_EventScript_BuyCategory::
	special GymDecor_PushShopCategories
	message GymDecor_Text_WhichKind
	waitmessage
	dynmultistack 18, 0, FALSE, 6, FALSE, VAR_0x8007, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, GymDecor_EventScript_BuyDone
	copyvar VAR_0x8006, VAR_RESULT
	copyvar VAR_0x8007, VAR_RESULT
	setvar VAR_0x8005, 0
GymDecor_EventScript_BuyLoop::
	special GymDecor_PushShopList
	message GymDecor_Text_WhichToBuy
	waitmessage
	dynmultistack 15, 0, FALSE, 5, FALSE, VAR_0x8005, DYN_MULTICHOICE_CB_GYM_DECOR_SHOP
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, GymDecor_EventScript_BuyCategory
	copyvar VAR_0x8004, VAR_RESULT
	copyvar VAR_0x8005, VAR_RESULT
	setvar VAR_0x8009, 1
	special GymDecor_GetKind
	goto_if_ge VAR_RESULT, 3, GymDecor_EventScript_ConfirmBuy   @ (floors and wall styles: bought once)
	special GymDecor_BufferDecorInfo
	message GymDecor_Text_HowMany
	waitmessage
	special GymDecor_ChooseQuantity
	waitstate
	goto_if_eq VAR_RESULT, 0, GymDecor_EventScript_BuyLoop
	copyvar VAR_0x8009, VAR_RESULT
GymDecor_EventScript_ConfirmBuy::
	special GymDecor_BufferDecorInfo
	msgbox GymDecor_Text_ConfirmBuy, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, GymDecor_EventScript_BuyLoop
	special GymDecor_TryBuy
	goto_if_eq VAR_RESULT, 1, GymDecor_EventScript_NotEnoughMoney
	goto_if_eq VAR_RESULT, 2, GymDecor_EventScript_StorageFull
	goto_if_eq VAR_RESULT, 3, GymDecor_EventScript_AlreadyHave
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

GymDecor_EventScript_AlreadyHave::
	msgbox GymDecor_Text_AlreadyHave, MSGBOX_DEFAULT
	goto GymDecor_EventScript_BuyLoop
	end

GymDecor_EventScript_BuyDone::
	hidemoneybox
	goto GymDecor_EventScript_PCMenu
	end

@ ---- Selling (what's in storage, not placed; half price) ------------------
GymDecor_EventScript_Sell::
	setvar VAR_0x8007, 0
	showmoneybox 0, 0
GymDecor_EventScript_SellCategory::
	special GymDecor_PushSellCategories
	goto_if_eq VAR_RESULT, 0, GymDecor_EventScript_NothingToSell
	message GymDecor_Text_WhichKindToSell
	waitmessage
	dynmultistack 18, 0, FALSE, 6, FALSE, VAR_0x8007, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, GymDecor_EventScript_SellDone
	copyvar VAR_0x8006, VAR_RESULT
	copyvar VAR_0x8007, VAR_RESULT
	setvar VAR_0x8005, 0
GymDecor_EventScript_SellLoop::
	special GymDecor_PushSellList
	goto_if_eq VAR_RESULT, 0, GymDecor_EventScript_SellCategory
	message GymDecor_Text_WhichToSell
	waitmessage
	dynmultistack 15, 0, FALSE, 5, FALSE, VAR_0x8005, DYN_MULTICHOICE_CB_GYM_DECOR_SELL
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, GymDecor_EventScript_SellCategory
	copyvar VAR_0x8004, VAR_RESULT
	copyvar VAR_0x8005, VAR_RESULT
	setvar VAR_0x8009, 1
	special GymDecor_BufferSellInfo
	message GymDecor_Text_HowManyToSell
	waitmessage
	special GymDecor_ChooseSellQuantity
	waitstate
	goto_if_eq VAR_RESULT, 0, GymDecor_EventScript_SellLoop
	copyvar VAR_0x8009, VAR_RESULT
	special GymDecor_BufferSellInfo
	msgbox GymDecor_Text_ConfirmSell, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, GymDecor_EventScript_SellLoop
	special GymDecor_Sell
	updatemoneybox
	playse SE_SHOP
	msgbox GymDecor_Text_Sold, MSGBOX_DEFAULT
	goto GymDecor_EventScript_SellLoop
	end

GymDecor_EventScript_NothingToSell::
	msgbox GymDecor_Text_NothingToSell, MSGBOX_DEFAULT
GymDecor_EventScript_SellDone::
	hidemoneybox
	goto GymDecor_EventScript_PCMenu
	end

@ ---- Putting everything away ---------------------------------------------
GymDecor_EventScript_PutAllAway::
	msgbox GymDecor_Text_ConfirmPutAllAway, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, GymDecor_EventScript_PCMenu
	special GymDecor_PutAllAway
	playse SE_CLICK
	msgbox GymDecor_Text_AllPutAway, MSGBOX_DEFAULT
	goto GymDecor_EventScript_PCMenu
	end

@ ---- Decorating ---------------------------------------------------------
GymDecor_EventScript_Decorate::
	special GymDecor_PushOwnedCategories
	goto_if_eq VAR_RESULT, 0, GymDecor_EventScript_NothingOwned
	message GymDecor_Text_WhichKindToPlace
	waitmessage
	dynmultistack 18, 0, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, GymDecor_EventScript_PCMenu
	copyvar VAR_0x8006, VAR_RESULT
	special GymDecor_PushOwnedList
	message GymDecor_Text_WhichToPlace
	waitmessage
	dynmultistack 15, 0, FALSE, 5, FALSE, 0, DYN_MULTICHOICE_CB_GYM_DECOR_OWNED
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, GymDecor_EventScript_Decorate
	copyvar VAR_0x8004, VAR_RESULT
	special GymDecor_GetKind   @ (GYM_DECOR_KIND_*: 3 a floor, 4 a wall style)
	goto_if_eq VAR_RESULT, 4, GymDecor_EventScript_SetWallStyle
	goto_if_eq VAR_RESULT, 3, GymDecor_EventScript_HowToPaint
	msgbox GymDecor_Text_HowToDecorate, MSGBOX_DEFAULT
GymDecor_EventScript_StartEditing::
	closemessage
	releaseall   @ before the HUD opens: releaseall clears the message box area, where the HUD goes
	special GymDecor_StartEditing
	end

GymDecor_EventScript_HowToPaint::
	msgbox GymDecor_Text_HowToPaint, MSGBOX_DEFAULT
	goto GymDecor_EventScript_StartEditing
	end

@ A wall style: all the GYM's walls, at once
GymDecor_EventScript_SetWallStyle::
	special GymDecor_ApplyWallStyle
	special GymDecor_BufferDecorInfo
	playse SE_CLICK
	msgbox GymDecor_Text_WallsChanged, MSGBOX_DEFAULT
	goto GymDecor_EventScript_PCMenu
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
	special GymDecor_WarpToPC
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

GymDecor_Text_Sell::
	.string "SELL DECORATIONS$"

GymDecor_Text_PutAllAway::
	.string "PUT ALL AWAY$"

GymDecor_Text_WhichKindToSell::
	.string "What kind of decoration will\n"
	.string "you sell?$"

GymDecor_Text_WhichToSell::
	.string "Which decoration will you\n"
	.string "sell? (Only ones in storage.)$"

GymDecor_Text_HowManyToSell::
	.string "{STR_VAR_1}? How many?$"

GymDecor_Text_ConfirmSell::
	.string "{STR_VAR_1}{STR_VAR_3}?\n"
	.string "You'll get ¥{STR_VAR_2}. OK?$"

GymDecor_Text_Sold::
	.string "Sold {STR_VAR_1}{STR_VAR_3}\n"
	.string "for ¥{STR_VAR_2}!$"

GymDecor_Text_NothingToSell::
	.string "There's nothing in storage to\n"
	.string "sell. (Placed pieces have to be\l"
	.string "put away first.)$"

GymDecor_Text_ConfirmPutAllAway::
	.string "Put all the decorations back in\n"
	.string "storage? Floors and walls stay.$"

GymDecor_Text_AllPutAway::
	.string "Everything's back in storage.$"

GymDecor_Text_LogOff::
	.string "LOG OFF$"

GymDecor_Text_WhichKind::
	.string "What kind of decoration?$"

GymDecor_Text_WhichKindToPlace::
	.string "What kind of decoration will\n"
	.string "you start with?$"

GymDecor_Text_WhichToBuy::
	.string "Which decoration would you\n"
	.string "like to buy?$"

GymDecor_Text_HowMany::
	.string "{STR_VAR_1}? How many?$"

GymDecor_Text_ConfirmBuy::
	.string "{STR_VAR_1}{STR_VAR_3}?\n"
	.string "That'll be ¥{STR_VAR_2}. OK?$"

GymDecor_Text_Bought::
	.string "Sent {STR_VAR_1}{STR_VAR_3}\n"
	.string "to the GYM's storage!$"

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
	.string "decoration appears in front of you,\l"
	.string "in red where it can't go.\p"
	.string "A: place it. START: turn it.\n"
	.string "SELECT: pick up the one in front,\l"
	.string "to move it (L/R: back to storage).\p"
	.string "L and R: switch decorations.\n"
	.string "B: finish decorating.$"

GymDecor_Text_HowToPaint::
	.string "Walk anywhere in the GYM. A paints\n"
	.string "the square in front of you.\p"
	.string "Hold A down as you walk to paint\n"
	.string "every square you face.\p"
	.string "SELECT: fill the whole area of\n"
	.string "floor in front with it (or pick\l"
	.string "up a piece in front).\p"
	.string "L and R: switch to a decoration\n"
	.string "or another floor. B: finish.$"

GymDecor_Text_WallsChanged::
	.string "The GYM's walls are now\n"
	.string "{STR_VAR_1}!$"

GymDecor_Text_AlreadyHave::
	.string "You already have that one. It\n"
	.string "can be used as much as you like!$"

GymDecor_Text_NothingOwned::
	.string "There aren't any decorations in\n"
	.string "storage. Buy some first!$"

GymDecor_Text_LayoutSaved::
	.string "The GYM's new look is saved!$"

GymDecor_Text_LayoutSavedBackToPC::
	.string "The GYM's new look is saved!\p"
	.string "Better head back to the PC from\n"
	.string "here…$"
