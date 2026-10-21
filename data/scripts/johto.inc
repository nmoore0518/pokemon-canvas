@ Johto (the maps come from Pokémon HnS: tools/gym_hack/import_johto.py)

Johto_EventScript_Nurse::
	lock
	faceplayer
	call EventScript_PkmnCenterNurse_Frlg
	release
	end

Johto_EventScript_MartClerk::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart Johto_Mart_Items
	msgbox gText_PleaseComeAgain
	release
	end

	.align 2
Johto_Mart_Items::
	.2byte ITEM_ULTRA_BALL
	.2byte ITEM_GREAT_BALL
	.2byte ITEM_POKE_BALL
	.2byte ITEM_HYPER_POTION
	.2byte ITEM_SUPER_POTION
	.2byte ITEM_FULL_HEAL
	.2byte ITEM_REVIVE
	.2byte ITEM_ESCAPE_ROPE
	.2byte ITEM_MAX_REPEL
	.2byte ITEM_NONE
	release
	end

@ ---------------------------------------------------------------------------
@ The convention greeter at the Olivine ferry dock: the goodie bag (the convention badge, Johto on
@ the TOWN MAP, supplies). VAR_JOHTO_STATE goes JOHTO_NOT_VISITED -> JOHTO_GOT_GOODIE_BAG.
@ ---------------------------------------------------------------------------
Johto_EventScript_GreeterStops::
	lockall
	applymovement LOCALID_OLIVINE_PORT_GREETER, Common_Movement_ExclamationMark
	waitmovement 0
	applymovement LOCALID_OLIVINE_PORT_GREETER, Common_Movement_FacePlayer
	applymovement LOCALID_PLAYER, Common_Movement_FaceUp
	waitmovement 0
	call Johto_EventScript_GoodieBag
	releaseall
	end

Johto_EventScript_Greeter::
	lock
	faceplayer
	goto_if_ge VAR_JOHTO_STATE, JOHTO_GOT_GOODIE_BAG, Johto_EventScript_GreeterAgain
	call Johto_EventScript_GoodieBag
	release
	end

Johto_EventScript_GreeterAgain::
	msgbox Johto_Text_GreeterAgain
	release
	end

Johto_EventScript_GoodieBag::
	msgbox Johto_Text_Welcome
	msgbox Johto_Text_HereIsYourBag
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	message Johto_Text_ReceivedBag
	waitfanfare
	waitmessage
	msgbox Johto_Text_BagContents
	checkitem ITEM_TOWN_MAP
	call_if_eq VAR_RESULT, FALSE, Johto_EventScript_GiveTownMap
	setflag FLAG_JOHTO_MAP
	giveitem ITEM_ULTRA_BALL, 5
	giveitem ITEM_FULL_RESTORE, 3
	setvar VAR_JOHTO_STATE, JOHTO_GOT_GOODIE_BAG
	msgbox Johto_Text_BadgeAndMap
	return

Johto_EventScript_GiveTownMap::
	giveitem ITEM_TOWN_MAP
	return

Johto_Text_Welcome::
	.string "Welcome to JOHTO!\p"
	.string "Oh, you must be {PLAYER}, the new\n"
	.string "GYM LEADER from KANTO!\p"
	.string "We've been expecting you. The GYM\n"
	.string "LEADER CONVENTION is being held in\l"
	.string "GOLDENROD CITY's RADIO TOWER.$"

Johto_Text_HereIsYourBag::
	.string "Every LEADER gets a welcome bag.\n"
	.string "Here's yours!$"

Johto_Text_ReceivedBag::
	.string "{PLAYER} received the\n"
	.string "CONVENTION goodie bag!$"

Johto_Text_BagContents::
	.string "There's a GYM LEADER CONVENTION\n"
	.string "badge, a JOHTO map for your TOWN\l"
	.string "MAP, and some supplies inside.$"

Johto_Text_BadgeAndMap::
	.string "Wear the badge, and folks around\n"
	.string "here will know you're a LEADER.\p"
	.string "The part of JOHTO that's open to\n"
	.string "convention guests is lit up on\l"
	.string "your TOWN MAP.\p"
	.string "You can FLY to the towns there\n"
	.string "once you've visited them. Press R\l"
	.string "on the FLY map to switch between\l"
	.string "KANTO and JOHTO.\p"
	.string "To get to GOLDENROD, take ROUTE 39\n"
	.string "north to ECRUTEAK, then head south.\p"
	.string "Enjoy the convention!$"

Johto_Text_GreeterAgain::
	.string "The GYM LEADER CONVENTION is in\n"
	.string "GOLDENROD's RADIO TOWER.\p"
	.string "Take ROUTE 39 north to ECRUTEAK,\n"
	.string "then head south. Enjoy!$"

@ ---------------------------------------------------------------------------
@ Guards where Johto goes on past the convention area (tools/gym_hack/import_johto.py puts them,
@ and a trigger line in front of them). VAR_0x8004: the guard; VAR_0x8005: the way the player
@ is sent back.
@ ---------------------------------------------------------------------------
Johto_EventScript_BorderGuard::
	lockall
	applymovement VAR_0x8004, Common_Movement_ExclamationMark
	waitmovement 0
	applymovement VAR_0x8004, Common_Movement_FacePlayer
	waitmovement 0
	goto_if_eq VAR_TEMP_2, 1, Johto_EventScript_BorderGuardAgain
	msgbox Johto_Text_GuardStop
	setvar VAR_TEMP_2, 1
	goto Johto_EventScript_BorderGuardSendBack

Johto_EventScript_BorderGuardAgain::
	msgbox Johto_Text_GuardStopAgain
Johto_EventScript_BorderGuardSendBack::
	closemessage
	call_if_eq VAR_0x8005, DIR_NORTH, Johto_EventScript_SendBackNorth
	call_if_eq VAR_0x8005, DIR_SOUTH, Johto_EventScript_SendBackSouth
	call_if_eq VAR_0x8005, DIR_WEST, Johto_EventScript_SendBackWest
	call_if_eq VAR_0x8005, DIR_EAST, Johto_EventScript_SendBackEast
	waitmovement 0
	releaseall
	end

Johto_EventScript_SendBackNorth::
	applymovement LOCALID_PLAYER, Common_Movement_WalkUp
	return

Johto_EventScript_SendBackSouth::
	applymovement LOCALID_PLAYER, Common_Movement_WalkDown
	return

Johto_EventScript_SendBackWest::
	applymovement LOCALID_PLAYER, Common_Movement_WalkLeft
	return

Johto_EventScript_SendBackEast::
	applymovement LOCALID_PLAYER, Common_Movement_WalkRight
	return

Johto_EventScript_GuardTalk::
	lock
	faceplayer
	msgbox Johto_Text_GuardTalk
	release
	end

Johto_Text_GuardStop::
	.string "Hey! You're not from around here.\n"
	.string "Who are you?\p"
	.string "…Oh! A GYM LEADER CONVENTION\n"
	.string "badge!\p"
	.string "You'd better get back there,\n"
	.string "LEADER. The convention's in\l"
	.string "GOLDENROD's RADIO TOWER!$"

Johto_Text_GuardStopAgain::
	.string "The convention's the other way,\n"
	.string "LEADER!$"

Johto_Text_GuardTalk::
	.string "Sorry, no one gets past here\n"
	.string "during the convention.\p"
	.string "Everything a LEADER needs is in\n"
	.string "GOLDENROD's RADIO TOWER!$"
