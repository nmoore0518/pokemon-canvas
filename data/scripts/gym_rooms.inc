@ Pokémon Canvas: the gym's extra rooms (src/gym_room.c, tools/gym_hack/build_gym_rooms.py).
@ Oak has one built for every 100 kinds of POKéMON caught, once the gym's open: a doorway opens in
@ the gym's back wall (100), then its left wall (200) and its right wall (300). Each room has its
@ own PC, floors and walls; the decorations come from the gym's storage.
@ (Claude's draft of Oak's words, Nov 6: Nick may rewrite them.)

@ (each room's ON_LOAD) the room, as the player has decorated it
GymRooms_EventScript_OnLoad::
	special GymDecor_DrawRoom
	end

@ (Oak in his lab, before anything else he has to say) a room earned?
GymRooms_EventScript_LabOak::
	special GymRooms_OakReward
	goto_if_eq VAR_RESULT, 0, GymRooms_EventScript_Return
	copyvar VAR_0x8004, VAR_RESULT
	msgbox GymRooms_Text_OakCaughtSoMany
	switch VAR_0x8004
	case 1, GymRooms_EventScript_BackRoom
	case 2, GymRooms_EventScript_LeftRoom
	msgbox GymRooms_Text_OakRightRoom
	goto GymRooms_EventScript_Give

GymRooms_EventScript_BackRoom::
	msgbox GymRooms_Text_OakBackRoom
	goto GymRooms_EventScript_Give

GymRooms_EventScript_LeftRoom::
	msgbox GymRooms_Text_OakLeftRoom
GymRooms_EventScript_Give::
	special GymRooms_GiveReward
	textcolor NPC_TEXT_COLOR_NEUTRAL
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	message GymRooms_Text_DoorwayOpened
	waitmessage
	waitfanfare
	call EventScript_RestorePrevTextColor
	msgbox GymRooms_Text_OakShowItOff
	release
	end

GymRooms_EventScript_Return::
	return

GymRooms_Text_OakCaughtSoMany::
	.string "Ah, {PLAYER}! Let me see your\n"
	.string "POKéDEX…\p"
	.string "{STR_VAR_1} kinds of POKéMON\n"
	.string "caught! Splendid work!\p"
	.string "A TRAINER who's met so many\n"
	.string "POKéMON deserves a place to\l"
	.string "show off their memories.$"

GymRooms_Text_OakBackRoom::
	.string "So I've had a room built onto your\n"
	.string "GYM, behind its back wall.\p"
	.string "It's a little smaller than the GYM,\n"
	.string "but it has a PC of its own.\p"
	.string "Decorate it any way you like! Your\n"
	.string "trophies would look grand in there.$"

GymRooms_Text_OakLeftRoom::
	.string "So I've had another room built\n"
	.string "onto your GYM, through its left\l"
	.string "wall.\p"
	.string "Make it whatever you like. A room\n"
	.string "for your favorite type, perhaps?$"

GymRooms_Text_OakRightRoom::
	.string "So I've had one more room built\n"
	.string "onto your GYM, through its right\l"
	.string "wall.\p"
	.string "That's every wall I can knock\n"
	.string "through, I'm afraid! Ho ho!$"

GymRooms_Text_DoorwayOpened::
	.string "A new doorway opened in\n"
	.string "{PLAYER}'s GYM!$"

GymRooms_Text_OakShowItOff::
	.string "Every room shares the GYM's\n"
	.string "storage, so decorate away!\p"
	.string "Each one has its own floors and\n"
	.string "walls, too.$"
