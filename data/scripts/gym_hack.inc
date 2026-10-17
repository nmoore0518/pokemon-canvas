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
