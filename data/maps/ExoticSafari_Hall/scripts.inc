@ The Exotic Pokémon Safari's hall (tools/gym_hack/build_exotic_safari.py): one door per region,
@ its name above it. Walking through an open door goes to that region's zone. The zones open once
@ the player is a Gym Leader (the Pallet Town Gym is theirs); until then every door is shut.
ExoticSafari_Hall_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_Hall_OnLoad
	.byte 0

ExoticSafari_Hall_OnLoad::
	goto_if_set FLAG_PALLET_GYM_AWARDED, ExoticSafari_Hall_OnLoadEnd
	setvar VAR_0x8004, EXOTIC_SAFARI_DOOR_X_PALDEA
	call ExoticSafari_Hall_EventScript_ShutDoor
ExoticSafari_Hall_OnLoadEnd::
	end

@ VAR_0x8004 = the door's x
ExoticSafari_Hall_EventScript_ShutDoor::
	subvar VAR_0x8004, 1
	setmetatile VAR_0x8004, EXOTIC_SAFARI_DOOR_Y - 1, METATILE_EXOTIC_SAFARI_SHUT_TOP_L, TRUE
	setmetatile VAR_0x8004, EXOTIC_SAFARI_DOOR_Y, METATILE_EXOTIC_SAFARI_SHUT_DOOR_L, TRUE
	addvar VAR_0x8004, 1
	setmetatile VAR_0x8004, EXOTIC_SAFARI_DOOR_Y - 1, METATILE_EXOTIC_SAFARI_SHUT_TOP_M, TRUE
	setmetatile VAR_0x8004, EXOTIC_SAFARI_DOOR_Y, METATILE_EXOTIC_SAFARI_SHUT_DOOR_M, TRUE
	addvar VAR_0x8004, 1
	setmetatile VAR_0x8004, EXOTIC_SAFARI_DOOR_Y - 1, METATILE_EXOTIC_SAFARI_SHUT_TOP_R, TRUE
	setmetatile VAR_0x8004, EXOTIC_SAFARI_DOOR_Y, METATILE_EXOTIC_SAFARI_SHUT_DOOR_R, TRUE
	return

ExoticSafari_Hall_EventScript_Attendant::
	lock
	faceplayer
	goto_if_unset FLAG_PALLET_GYM_AWARDED, ExoticSafari_Hall_EventScript_AttendantNotYet
	msgbox ExoticSafari_Hall_Text_Welcome
	release
	end

ExoticSafari_Hall_EventScript_AttendantNotYet::
	msgbox ExoticSafari_Hall_Text_NotYet
	release
	end

@ The doors (signs on them: an open door is walked through, so only the shut ones say anything)
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
	bufferstring STR_VAR_1, ExoticSafari_Hall_Text_Unova
	goto ExoticSafari_Hall_EventScript_ShutSign
ExoticSafari_Hall_EventScript_DoorKalos::
	bufferstring STR_VAR_1, ExoticSafari_Hall_Text_Kalos
	goto ExoticSafari_Hall_EventScript_ShutSign
ExoticSafari_Hall_EventScript_DoorAlola::
	bufferstring STR_VAR_1, ExoticSafari_Hall_Text_Alola
	goto ExoticSafari_Hall_EventScript_ShutSign
ExoticSafari_Hall_EventScript_DoorGalar::
	bufferstring STR_VAR_1, ExoticSafari_Hall_Text_Galar
	goto ExoticSafari_Hall_EventScript_ShutSign
ExoticSafari_Hall_EventScript_DoorPaldea::
	goto_if_unset FLAG_PALLET_GYM_AWARDED, ExoticSafari_Hall_EventScript_LeadersOnly
	end

ExoticSafari_Hall_EventScript_ShutSign::
	msgbox ExoticSafari_Hall_Text_Shut, MSGBOX_SIGN
	end

ExoticSafari_Hall_EventScript_LeadersOnly::
	msgbox ExoticSafari_Hall_Text_LeadersOnly, MSGBOX_SIGN
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

ExoticSafari_Hall_Text_LeadersOnly::
	.string "The door is locked.\n"
	.string "“For GYM LEADERS only!”$"

ExoticSafari_Hall_Text_Welcome::
	.string "Welcome to the EXOTIC POKéMON\n"
	.string "SAFARI!\p"
	.string "Each door leads to a zone full of\n"
	.string "POKéMON from a faraway region.\p"
	.string "Only the PALDEA ZONE is open so far.\n"
	.string "The others are still being built!\p"
	.string "No SAFARI BALLS here: battle and\n"
	.string "catch them with your own BALLS.$"

ExoticSafari_Hall_Text_NotYet::
	.string "Welcome to the EXOTIC POKéMON\n"
	.string "SAFARI!\p"
	.string "Sorry, the zones are only open\n"
	.string "to GYM LEADERS.\p"
	.string "The POKéMON here come from very\n"
	.string "far away, and they're tough!$"
