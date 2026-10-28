@ The Exotic Pokémon Safari's hall (tools/gym_hack/build_exotic_safari.py, src/exotic_safari.c):
@ one door per region along the back wall, in its games' colors, its name above it. A counter
@ splits the hall; the attendant stands in its gap. Once the player is a Gym Leader, the attendant
@ sells them the EXOTIC PASS (a one-time special) and steps aside; an open zone's door then slides
@ open when they stand in front of it. The kiosk on the counter sells supplies.
ExoticSafari_Hall_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_Hall_OnLoad
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_Hall_OnTransition
	.byte 0

ExoticSafari_Hall_OnLoad::
	special ExoticSafari_OnLoad
	end

@ The attendant: in the counter's gap until the pass is bought, then behind the counter
ExoticSafari_Hall_OnTransition::
	setflag FLAG_HIDE_EXOTIC_SAFARI_DESK
	goto_if_unset FLAG_EXOTIC_SAFARI_PASS, ExoticSafari_Hall_OnTransitionEnd
	clearflag FLAG_HIDE_EXOTIC_SAFARI_DESK
ExoticSafari_Hall_OnTransitionEnd::
	end

ExoticSafari_Hall_EventScript_Attendant::
	lock
	faceplayer
	goto_if_unset FLAG_PALLET_GYM_AWARDED, ExoticSafari_Hall_EventScript_NotOpenYet
	msgbox ExoticSafari_Hall_Text_Pitch
	showmoneybox 0, 0
	msgbox ExoticSafari_Hall_Text_Offer, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, ExoticSafari_Hall_EventScript_NoThanks
	checkmoney EXOTIC_SAFARI_PASS_PRICE
	goto_if_eq VAR_RESULT, FALSE, ExoticSafari_Hall_EventScript_NoMoney
	removemoney EXOTIC_SAFARI_PASS_PRICE
	updatemoneybox
	playse SE_SHOP
	msgbox ExoticSafari_Hall_Text_Thanks
	hidemoneybox
	giveitem ITEM_EXOTIC_PASS
	setflag FLAG_EXOTIC_SAFARI_PASS
	msgbox ExoticSafari_Hall_Text_TakeALook
	closemessage
	@ The camera goes up to the doors and back
	special SpawnCameraObject
	applymovement LOCALID_CAMERA, ExoticSafari_Hall_Movement_CameraUp
	waitmovement 0
	delay 60
	applymovement LOCALID_CAMERA, ExoticSafari_Hall_Movement_CameraDown
	waitmovement 0
	special RemoveCameraObject
	msgbox ExoticSafari_Hall_Text_GoOnThrough
	closemessage
	applymovement LOCALID_EXOTIC_SAFARI_ATTENDANT, ExoticSafari_Hall_Movement_StepAside
	waitmovement 0
	removeobject LOCALID_EXOTIC_SAFARI_ATTENDANT
	clearflag FLAG_HIDE_EXOTIC_SAFARI_DESK
	addobject LOCALID_EXOTIC_SAFARI_DESK
	release
	end

ExoticSafari_Hall_EventScript_NotOpenYet::
	msgbox ExoticSafari_Hall_Text_NotOpenYet
	release
	end

ExoticSafari_Hall_EventScript_NoThanks::
	hidemoneybox
	msgbox ExoticSafari_Hall_Text_NoThanks
	release
	end

ExoticSafari_Hall_EventScript_NoMoney::
	hidemoneybox
	msgbox ExoticSafari_Hall_Text_NoMoney
	release
	end

ExoticSafari_Hall_Movement_CameraUp:
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	step_end

ExoticSafari_Hall_Movement_CameraDown:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	step_end

ExoticSafari_Hall_Movement_StepAside:
	walk_up
	walk_left
	face_down
	step_end

@ Behind the counter, once the pass is bought
ExoticSafari_Hall_EventScript_AttendantDesk::
	msgbox ExoticSafari_Hall_Text_Enjoy, MSGBOX_NPC
	end

ExoticSafari_Hall_EventScript_Kiosk::
	lock
	faceplayer
	message ExoticSafari_Hall_Text_Kiosk
	waitmessage
	special ExoticSafari_Kiosk
	waitstate
	msgbox gText_PleaseComeAgain
	release
	end

@ The evolution items' kiosk: what's been found in the zones
ExoticSafari_Hall_EventScript_EvoKiosk::
	lock
	faceplayer
	message ExoticSafari_Hall_Text_EvoKiosk
	waitmessage
	special ExoticSafari_EvoKiosk
	goto_if_eq VAR_RESULT, FALSE, ExoticSafari_Hall_EventScript_EvoKioskEmpty
	waitstate
	msgbox gText_PleaseComeAgain
	release
	end

ExoticSafari_Hall_EventScript_EvoKioskEmpty::
	msgbox ExoticSafari_Hall_Text_EvoKioskEmpty
	release
	end

@ The doors (signs on them: an open door slides open, so only the shut ones say anything)
ExoticSafari_Hall_EventScript_DoorJohto::
	bufferstring STR_VAR_1, ExoticSafari_Hall_Text_Johto
	goto ExoticSafari_Hall_EventScript_ShutSign
ExoticSafari_Hall_EventScript_DoorHoenn::
	bufferstring STR_VAR_1, ExoticSafari_Hall_Text_Hoenn
	goto ExoticSafari_Hall_EventScript_ShutSign
ExoticSafari_Hall_EventScript_DoorSinnoh::
	bufferstring STR_VAR_1, ExoticSafari_Hall_Text_Sinnoh
	goto ExoticSafari_Hall_EventScript_ShutSign
ExoticSafari_Hall_EventScript_DoorUnova::
	end
ExoticSafari_Hall_EventScript_DoorKalos::
	end
ExoticSafari_Hall_EventScript_DoorAlola::
	end
ExoticSafari_Hall_EventScript_DoorGalar::
	end
ExoticSafari_Hall_EventScript_DoorPaldea::
	end

ExoticSafari_Hall_EventScript_ShutSign::
	msgbox ExoticSafari_Hall_Text_Shut, MSGBOX_SIGN
	end

ExoticSafari_Hall_Text_Johto::
	.string "JOHTO$"
ExoticSafari_Hall_Text_Hoenn::
	.string "HOENN$"
ExoticSafari_Hall_Text_Sinnoh::
	.string "SINNOH$"
ExoticSafari_Hall_Text_Unova::
	.string "UNOVA$"
ExoticSafari_Hall_Text_Kalos::
	.string "KALOS$"
ExoticSafari_Hall_Text_Alola::
	.string "ALOLA$"
ExoticSafari_Hall_Text_Galar::
	.string "GALAR$"

ExoticSafari_Hall_Text_Shut::
	.string "The door is locked.\n"
	.string "“{STR_VAR_1} ZONE: COMING SOON!”$"

ExoticSafari_Hall_Text_NotOpenYet::
	.string "Welcome to the EXOTIC POKéMON\n"
	.string "SAFARI!\p"
	.string "Each of those doors leads to a zone\n"
	.string "full of POKéMON from a faraway\l"
	.string "region.\p"
	.string "We're not quite open yet, though.\n"
	.string "The zones are still being built!\p"
	.string "Please come back later!$"

ExoticSafari_Hall_Text_Pitch::
	.string "Welcome to the EXOTIC POKéMON\n"
	.string "SAFARI! We just opened!\p"
	.string "Each of those doors leads to a zone\n"
	.string "full of POKéMON from a faraway\l"
	.string "region. Nobody in KANTO has ever\l"
	.string "seen most of them!\p"
	.string "Hm? Wait a second…\n"
	.string "You're LEADER {PLAYER}!\l"
	.string "From the PALLET TOWN GYM!\p"
	.string "Listen. Once trainers see a GYM\n"
	.string "LEADER using exotic POKéMON,\l"
	.string "they'll ALL want to catch their own!\p"
	.string "So for you, a one-time special:\n"
	.string "an UNLIMITED PASS to every zone,\l"
	.string "for just ¥10,000!$"

ExoticSafari_Hall_Text_Offer::
	.string "Would you like an EXOTIC PASS?$"

ExoticSafari_Hall_Text_Thanks::
	.string "Thank you! Here's your pass.\n"
	.string "Show those POKéMON off!$"

ExoticSafari_Hall_Text_TakeALook::
	.string "Take a look! A door for every\n"
	.string "region, from JOHTO to PALDEA!$"

ExoticSafari_Hall_Text_GoOnThrough::
	.string "The UNOVA, KALOS, ALOLA, GALAR and\n"
	.string "PALDEA ZONES are open. The rest are\l"
	.string "still being built!\p"
	.string "Just walk up to a door. It opens\n"
	.string "for pass holders.\p"
	.string "No SAFARI BALLS here: battle and\n"
	.string "catch them with your own BALLS.\p"
	.string "Go on through, LEADER!$"

ExoticSafari_Hall_Text_NoThanks::
	.string "Oh… The offer stands, if you change\n"
	.string "your mind!$"

ExoticSafari_Hall_Text_NoMoney::
	.string "Oh… You don't have enough money.\n"
	.string "The offer stands, though!$"

ExoticSafari_Hall_Text_Enjoy::
	.string "Enjoy the EXOTIC POKéMON SAFARI,\n"
	.string "LEADER {PLAYER}!\p"
	.string "Walk up to an open zone's door\n"
	.string "and it'll open for you.$"

ExoticSafari_Hall_Text_Kiosk::
	.string "Welcome to the SAFARI KIOSK!\n"
	.string "Stocking up for a trip?$"

ExoticSafari_Hall_Text_EvoKiosk::
	.string "EVOLUTION KIOSK! Anything found\n"
	.string "out in the zones, we stock here.$"

ExoticSafari_Hall_Text_EvoKioskEmpty::
	.string "Nothing's come in yet, though.\p"
	.string "Find an evolution item in a zone\n"
	.string "and we'll keep more of them here.$"
