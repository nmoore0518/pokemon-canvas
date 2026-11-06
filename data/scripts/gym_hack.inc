@ Gym Trainer hack: shared scripts (see src/gym_hack.c)

@ Runs instead of a gym leader or rival battle when the team breaks the rules.
@ VAR_RESULT holds the GYM_HACK_TEAM_* result and the names are already buffered.
EventScript_GymHack_TeamNotAllowed::
	lockall
	textcolor NPC_TEXT_COLOR_NEUTRAL
	call EventScript_GymHack_ShowTeamRuleMessage
	closemessage
	@ Reload the map so anyone who walked up for a battle goes back to their
	@ spot, ready for when the team is fixed.
	special GymHack_WarpInPlace
	waitstate
	end

@ Shows the message for the GYM_HACK_TEAM_* result in VAR_RESULT.
EventScript_GymHack_ShowTeamRuleMessage::
	call_if_eq VAR_RESULT, GYM_HACK_TEAM_WRONG_TYPE, EventScript_GymHack_MsgWrongType
	call_if_eq VAR_RESULT, GYM_HACK_TEAM_DUPLICATE, EventScript_GymHack_MsgDuplicate
	call_if_eq VAR_RESULT, GYM_HACK_TEAM_DUPLICATE_ITEM, EventScript_GymHack_MsgDuplicateItem
	return

EventScript_GymHack_MsgDuplicateItem::
	msgbox Text_GymHack_DuplicateItem
	return

EventScript_GymHack_MsgWrongType::
	msgbox Text_GymHack_WrongType
	return

EventScript_GymHack_MsgDuplicate::
	msgbox Text_GymHack_Duplicate
	return

Text_GymHack_WrongType::
	.string "LEAGUE RULES: Only {STR_VAR_3}-type\n"
	.string "POKéMON may be on your team.\p"
	.string "{STR_VAR_1} is {STR_VAR_2}-type!\p"
	.string "Leave it in a PC and come back.$"

Text_GymHack_Duplicate::
	.string "LEAGUE RULES: No two POKéMON on\n"
	.string "your team may be the same species.\p"
	.string "You have more than one\n"
	.string "{STR_VAR_1}!\p"
	.string "Leave one in a PC and come back.$"

Text_GymHack_DuplicateItem::
	.string "LEAGUE RULES: No two POKéMON on\n"
	.string "your team may hold the same item.\p"
	.string "More than one is holding\n"
	.string "{STR_VAR_1}!\p"
	.string "Take one off and come back.$"

@ ---------------------------------------------------------------------------
@ HM key items. "...WithItem" scripts run when walking up to a tree, boulder or
@ water with no Pokemon that knows the move; "Use..." scripts run when the
@ item is used from the bag.
@ ---------------------------------------------------------------------------

@ Cut: Hedge Clippers
EventScript_GymHack_CutTreeWithItem::
	bufferitemname STR_VAR_2, ITEM_HEDGE_CLIPPERS
	msgbox Text_GymHack_WantToCutWithItem, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_CancelCut
	goto EventScript_GymHack_CutWithItem

EventScript_GymHack_UseHedgeClippers::
	lockall
	bufferitemname STR_VAR_2, ITEM_HEDGE_CLIPPERS
EventScript_GymHack_CutWithItem::
	msgbox Text_GymHack_PlayerUsedItem, MSGBOX_DEFAULT
	closemessage
	playse SE_M_CUT
	setvar VAR_0x8004, FALSE
	goto EventScript_CutTreeDown

@ Strength: Can of Spinach
EventScript_GymHack_StrengthWithItem::
	bufferitemname STR_VAR_2, ITEM_CAN_OF_SPINACH
	msgbox Text_GymHack_WantToStrengthWithItem, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_CancelStrength
	goto EventScript_GymHack_EatSpinach

EventScript_GymHack_UseCanOfSpinach::
	lockall
	bufferitemname STR_VAR_2, ITEM_CAN_OF_SPINACH
EventScript_GymHack_EatSpinach::
	setflag FLAG_SYS_USE_STRENGTH
	playse SE_M_STRENGTH
	msgbox Text_GymHack_AteSpinach, MSGBOX_DEFAULT
	releaseall
	end

@ Surf: Boogie Board
EventScript_GymHack_SurfWithItem::
	lockall
	msgbox gText_WantToUseSurf, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_ReleaseUseSurf
	goto EventScript_GymHack_SurfOnBoard

EventScript_GymHack_UseBoogieBoard::
	lockall
EventScript_GymHack_SurfOnBoard::
	bufferitemname STR_VAR_2, ITEM_BOOGIE_BOARD
	msgbox Text_GymHack_PlayerUsedItem, MSGBOX_DEFAULT
	closemessage
	setfieldeffectargument 0, 0
	special GymHack_SkipNextShowMon
	dofieldeffect FLDEFF_USE_SURF
	releaseall
	end

@ Flash: Flashlight
EventScript_GymHack_UseFlashlight::
	lockall
	bufferitemname STR_VAR_2, ITEM_FLASHLIGHT
	msgbox Text_GymHack_SwitchedOnFlashlight, MSGBOX_DEFAULT
	closemessage
	playse SE_M_REFLECT
	setflag FLAG_SYS_USE_FLASH
	animateflash 1
	setflashlevel 1
	releaseall
	end

@ Rock Smash: Sledgehammer
EventScript_GymHack_SmashRockWithItem::
	bufferitemname STR_VAR_2, ITEM_SLEDGEHAMMER
	msgbox Text_GymHack_WantToSmashWithItem, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_CancelSmash
	goto EventScript_GymHack_SmashWithItem

EventScript_GymHack_UseSledgehammer::
	lockall
	bufferitemname STR_VAR_2, ITEM_SLEDGEHAMMER
EventScript_GymHack_SmashWithItem::
	msgbox Text_GymHack_PlayerUsedItem, MSGBOX_DEFAULT
	closemessage
	playse SE_M_ROCK_THROW
	setvar VAR_0x8004, FALSE
	goto EventScript_SmashRock

@ Waterfall: Rocket Boots
EventScript_GymHack_WaterfallWithItem::
	bufferitemname STR_VAR_2, ITEM_ROCKET_BOOTS
	msgbox Text_GymHack_WantToWaterfallWithItem, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_EndWaterfall
	goto EventScript_GymHack_ClimbWaterfall

EventScript_GymHack_UseRocketBoots::
	lockall
	bufferitemname STR_VAR_2, ITEM_ROCKET_BOOTS
EventScript_GymHack_ClimbWaterfall::
	msgbox Text_GymHack_PlayerUsedItem, MSGBOX_DEFAULT
	hidefollowernpc
	setfieldeffectargument 0, 0
	special GymHack_SkipNextShowMon
	dofieldeffect FLDEFF_USE_WATERFALL
	callnative FollowerNPC_WarpSetEnd
	goto EventScript_EndWaterfall

Text_GymHack_PlayerUsedItem::
	.string "{PLAYER} used the {STR_VAR_2}!$"

Text_GymHack_WantToCutWithItem::
	.string "This tree looks like it can be\n"
	.string "CUT down!\p"
	.string "Use the {STR_VAR_2}?$"

Text_GymHack_WantToStrengthWithItem::
	.string "It's a big boulder, but with a\n"
	.string "little STRENGTH it could be moved.\p"
	.string "Eat the {STR_VAR_2}?$"

Text_GymHack_AteSpinach::
	.string "{PLAYER} ate the {STR_VAR_2}!\p"
	.string "{PLAYER}'s STRENGTH made it\n"
	.string "possible to move boulders around!$"

Text_GymHack_WantToSmashWithItem::
	.string "It's a rugged rock, but it looks\n"
	.string "like it could be smashed.\p"
	.string "Use the {STR_VAR_2}?$"

Text_GymHack_WantToWaterfallWithItem::
	.string "It's a large waterfall.\n"
	.string "Use the {STR_VAR_2}?$"

Text_GymHack_SwitchedOnFlashlight::
	.string "{PLAYER} switched on the\n"
	.string "{STR_VAR_2}!$"

@ Said by the NPCs who give out HMs, right before the matching key item
Text_GymHack_AndTakeThisToo::
	.string "Oh, and take this, too!\p"
	.string "It does the same job outside of\n"
	.string "battle, no POKéMON needed.$"
