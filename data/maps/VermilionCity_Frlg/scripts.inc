VermilionCity_Frlg_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, VermilionCity_OnLoad
	map_script MAP_SCRIPT_ON_TRANSITION, VermilionCity_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, VermilionCity_OnFrame
	.byte 0

VermilionCity_OnFrame::
	map_script_2 VAR_MAP_SCENE_VERMILION_CITY, 2, VermilionCity_EventScript_ExitSSAnne
	.2byte 0

VermilionCity_EventScript_ExitSSAnne::
	lockall
	applymovement LOCALID_PLAYER, VermilionCity_Movement_ExitSSAnne
	waitmovement 0
	setvar VAR_MAP_SCENE_VERMILION_CITY, 3
	releaseall
	end

VermilionCity_Movement_ExitSSAnne::
	walk_up
	walk_up
	step_end

@ Pokémon Canvas: the Universal Pokémon Medical Center on the lot, once it's built (data/scripts/medical_center_lot.inc)
VermilionCity_OnLoad::
	call_if_set FLAG_MEDCENTER_OPEN, VermilionCity_EventScript_MedicalCenterBuilt
	end

VermilionCity_OnTransition::
	setworldmapflag FLAG_WORLD_MAP_VERMILION_CITY
	call_if_set FLAG_TALKED_TO_OAKS_AIDE_IN_VERMILION, VermilionCity_EventScript_HideOaksAide
	end

VermilionCity_EventScript_HideOaksAide::
	setflag FLAG_HIDE_VERMILION_CITY_OAKS_AIDE
	return

VermilionCity_EventScript_Woman::
	msgbox VermilionCity_Text_GrimerMultipliesInSludge, MSGBOX_NPC
	end

VermilionCity_EventScript_OldMan1::
	lock
	faceplayer
	goto_if_eq VAR_MAP_SCENE_VERMILION_CITY, 3, VermilionCity_EventScript_OldMan1SSAnneLeft
	msgbox VermilionCity_Text_DidYouSeeSSAnneInHarbor
	release
	end

VermilionCity_EventScript_OldMan1SSAnneLeft::
	msgbox VermilionCity_Text_SSAnneHasDepartedForYear
	release
	end

VermilionCity_EventScript_FerrySailor::
	lock
	faceplayer
	goto_if_ge VAR_JOHTO_STATE, JOHTO_INVITED, Johto_EventScript_VermilionFerry
	goto_if_eq VAR_MAP_SCENE_VERMILION_CITY, 3, VermilionCity_EventScript_CheckSeagallopPresent
	msgbox VermilionCity_Text_WelcomeToTheSSAnne
	release
	end

VermilionCity_EventScript_CheckHasMysticTicket::
	goto_if_unset FLAG_ENABLE_SHIP_NAVEL_ROCK, EventScript_SetResultFalse
	checkitem ITEM_MYSTIC_TICKET
	goto_if_eq VAR_RESULT, FALSE, EventScript_SetResultFalse
	goto EventScript_SetResultTrue
	end

VermilionCity_EventScript_CheckHasAuroraTicket::
	goto_if_unset FLAG_ENABLE_SHIP_BIRTH_ISLAND, EventScript_SetResultFalse
	checkitem ITEM_AURORA_TICKET
	goto_if_eq VAR_RESULT, FALSE, EventScript_SetResultFalse
	goto EventScript_SetResultTrue
	end

VermilionCity_EventScript_CheckSeagallopPresent::
	setvar VAR_0x8004, SEAGALLOP_VERMILION_CITY
	goto_if_ge VAR_MAP_SCENE_ONE_ISLAND_POKEMON_CENTER_1F, 5, VermilionCity_EventScript_ChooseSeagallopDestRainbowPass
	goto_if_ge VAR_MAP_SCENE_ONE_ISLAND_POKEMON_CENTER_1F, 1, VermilionCity_EventScript_ChooseSeagallopDestTriPass
	msgbox VermilionCity_Text_TheShipSetSail
	release
	end

VermilionCity_EventScript_ChooseSeagallopDestRainbowPass::
	call VermilionCity_EventScript_CheckHasMysticTicket
	goto_if_eq VAR_RESULT, TRUE, VermilionCity_EventScript_HasMysticTicket
	call VermilionCity_EventScript_CheckHasAuroraTicket
	goto_if_eq VAR_RESULT, TRUE, VermilionCity_EventScript_HasAuroraTicket
	setvar VAR_0x8004, SEAGALLOP_VERMILION_CITY
	message VermilionCity_Text_BoardSeagallopRainbowPass
	waitmessage
	goto EventScript_SeviiDestinationsPage1
	end

VermilionCity_EventScript_HasMysticTicket::
	call VermilionCity_EventScript_CheckHasAuroraTicket
	goto_if_eq VAR_RESULT, TRUE, VermilionCity_EventScript_HasMysticAndAuroraTickets
	call_if_unset FLAG_SHOWN_MYSTIC_TICKET, VermilionCity_EventScript_ShowMysticTicket
	message VermilionCity_Text_BoardSeagallopFerry
	waitmessage
	multichoice 17, 6, MULTI_SEVII_NAVEL, FALSE
	switch VAR_RESULT
	case 0, EventScript_SeviiDestinationsPage1
	case 1, EventScript_SailToNavelRock
	case 2, EventScript_CancelSail
	case 127, EventScript_CancelSail
	end

VermilionCity_EventScript_ShowMysticTicket::
	setflag FLAG_SHOWN_MYSTIC_TICKET
	msgbox VermilionCity_Text_OhMysticTicketTakeYouToNavelRock
	return

VermilionCity_EventScript_HasAuroraTicket::
	call_if_unset FLAG_SHOWN_AURORA_TICKET, VermilionCity_EventScript_ShowAuroraTicket
	message VermilionCity_Text_BoardSeagallopFerry
	waitmessage
	multichoice 13, 6, MULTI_SEVII_BIRTH, FALSE
	switch VAR_RESULT
	case 0, EventScript_SeviiDestinationsPage1
	case 1, EventScript_SailToBirthIsland
	case 2, EventScript_CancelSail
	case 127, EventScript_CancelSail
	end

VermilionCity_EventScript_ShowAuroraTicket::
	setflag FLAG_SHOWN_AURORA_TICKET
	msgbox VermilionCity_Text_OhAuroraTicketTakeYouToBirthIsland
	return

VermilionCity_EventScript_HasMysticAndAuroraTickets::
	call_if_unset FLAG_SHOWN_MYSTIC_TICKET, VermilionCity_EventScript_ShowMysticTicket
	call_if_unset FLAG_SHOWN_AURORA_TICKET, VermilionCity_EventScript_ShowAuroraTicket
	message VermilionCity_Text_BoardSeagallopFerry
	waitmessage
	multichoice 13, 5, MULTI_SEVII_NAVEL_BIRTH, FALSE
	switch VAR_RESULT
	case 0, EventScript_SeviiDestinationsPage1
	case 1, EventScript_SailToNavelRock
	case 2, EventScript_SailToBirthIsland
	case 3, EventScript_CancelSail
	case 127, EventScript_CancelSail
	end

EventScript_SailToNavelRock::
	msgbox VermilionCity_Text_Seagallop10Departing
	setvar VAR_0x8004, SEAGALLOP_VERMILION_CITY
	setvar VAR_0x8006, SEAGALLOP_NAVEL_ROCK
	goto EventScript_SetSail
	end

EventScript_SailToBirthIsland::
	msgbox VermilionCity_Text_Seagallop12Departing
	setvar VAR_0x8004, SEAGALLOP_VERMILION_CITY
	setvar VAR_0x8006, SEAGALLOP_BIRTH_ISLAND
	goto EventScript_SetSail
	end

VermilionCity_EventScript_ChooseSeagallopDestTriPass::
	message VermilionCity_Text_BoardSeagallopTriPass
	waitmessage
	setvar VAR_0x8004, SEAGALLOP_VERMILION_CITY
	multichoice 19, 5, MULTI_SEAGALLOP_123, FALSE
	switch VAR_RESULT
	case 0, EventScript_SailToOneIsland2
	case 1, EventScript_SailToTwoIsland2
	case 2, EventScript_SailToThreeIsland2
	case 3, EventScript_CancelSail
	case 127, EventScript_CancelSail
	end

@ Unreachable
Vermilion_EventScript_Unused::
	release
	end

VermilionCity_EventScript_CheckTicketLeft::
	lockall
	goto VermilionCity_EventScript_CheckTicket
	end

VermilionCity_EventScript_CheckTicketRight::
	lockall
	goto VermilionCity_EventScript_CheckTicket
	end

VermilionCity_EventScript_ExitedTicketCheck::
	lockall
	setvar VAR_VERMILION_CITY_TICKET_CHECK_TRIGGER, 0
	releaseall
	end

VermilionCity_EventScript_CheckTicket::
	textcolor NPC_TEXT_COLOR_MALE
	applymovement LOCALID_VERMILION_FERRY_SAILOR, Common_Movement_WalkInPlaceFasterLeft
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterRight
	waitmovement 0
	goto_if_ge VAR_JOHTO_STATE, JOHTO_INVITED, Johto_EventScript_VermilionFerryTrigger
	goto_if_eq VAR_MAP_SCENE_VERMILION_CITY, 3, VermilionCity_EventScript_CheckSeagallopPresentTrigger
	msgbox VermilionCity_Text_DoYouHaveATicket
	goto_if_unset FLAG_GOT_SS_TICKET, VermilionCity_EventScript_DontHaveSSTicket
	msgbox VermilionCity_Text_FlashedSSTicket
	setvar VAR_VERMILION_CITY_TICKET_CHECK_TRIGGER, 1
	releaseall
	end

VermilionCity_EventScript_DontHaveSSTicket::
	msgbox VermilionCity_Text_DontHaveNeededSSTicket
	closemessage
	goto VermilionCity_EventScript_WalkUpPier
	end

VermilionCity_EventScript_CheckSeagallopPresentTrigger::
	setvar VAR_0x8004, SEAGALLOP_VERMILION_CITY
	goto_if_ge VAR_MAP_SCENE_ONE_ISLAND_POKEMON_CENTER_1F, 5, VermilionCity_EventScript_ChooseSeagallopDestRainbowPass
	goto_if_ge VAR_MAP_SCENE_ONE_ISLAND_POKEMON_CENTER_1F, 1, VermilionCity_EventScript_ChooseSeagallopDestTriPass
	msgbox VermilionCity_Text_TheShipSetSail
	closemessage
	goto VermilionCity_EventScript_WalkUpPier
	end

VermilionCity_EventScript_Sailor::
	msgbox VermilionCity_Text_SSAnneVisitsOnceAYear, MSGBOX_NPC
	end

VermilionCity_EventScript_OaksAide::
	lock
	faceplayer
	setflag FLAG_TALKED_TO_OAKS_AIDE_IN_VERMILION
	msgbox VermilionCity_Text_Route2AideHasPackageForYou
	release
	end

VermilionCity_EventScript_OldMan2::
	msgbox VermilionCity_Text_BuildingOnThisLand, MSGBOX_NPC
	end

@ Pokémon Canvas: the foreman (src/medical_center.c). His crew quit: he needs a MACHOP, a MACHOKE, a
@ MACHAMP and a shiny MACHAMP. Each one goes to work for him for good, and each
@ turns something up in the dig. With the fourth, the Universal Pokémon Medical Center goes up.
VermilionCity_EventScript_Foreman::
	lock
	faceplayer
	special MedCenter_BufferHelper
	copyvar VAR_0x8006, VAR_RESULT
	call_if_eq VAR_0x8006, 0, VermilionCity_EventScript_ForemanIntro
	call_if_ne VAR_0x8006, 0, VermilionCity_EventScript_ForemanNext
	msgbox VermilionCity_Text_ForemanAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, VermilionCity_EventScript_ForemanNo
	msgbox VermilionCity_Text_ForemanWhich
	special ChoosePartyMon
	goto_if_ge VAR_0x8004, PARTY_SIZE, VermilionCity_EventScript_ForemanNo
	special MedCenter_CheckHelper
	switch VAR_RESULT
	case HELPER_OK, VermilionCity_EventScript_ForemanTake
	case HELPER_NOT_SHINY, VermilionCity_EventScript_ForemanNotShiny
	case HELPER_EGG, VermilionCity_EventScript_ForemanEgg
	case HELPER_LAST_MON, VermilionCity_EventScript_ForemanLastMon
	msgbox VermilionCity_Text_ForemanWrong
	release
	end

VermilionCity_EventScript_ForemanIntro::
	msgbox VermilionCity_Text_ForemanIntro
	return

VermilionCity_EventScript_ForemanNext::
	msgbox VermilionCity_Text_ForemanNext
	return

VermilionCity_EventScript_ForemanTake::
	msgbox VermilionCity_Text_ForemanThanks
	special MedCenter_TakeHelper
	copyvar VAR_0x8006, VAR_RESULT
	playse SE_M_MEGA_KICK
	waitse
	switch VAR_0x8006
	case 1, VermilionCity_EventScript_ForemanFind1
	case 2, VermilionCity_EventScript_ForemanFind2
	case 3, VermilionCity_EventScript_ForemanFind3
	goto VermilionCity_EventScript_ForemanBuilt
	end

@ the first dig: the Mt. Moon fossil the player didn't take
VermilionCity_EventScript_ForemanFind1::
	msgbox VermilionCity_Text_ForemanFound
	setvar VAR_0x8009, ITEM_HELIX_FOSSIL
	call_if_set FLAG_GOT_HELIX_FOSSIL, VermilionCity_EventScript_ForemanDome
	call GymHack_EventScript_GiveStatItem
	msgbox VermilionCity_Text_ForemanFossil
	release
	end

VermilionCity_EventScript_ForemanDome::
	setvar VAR_0x8009, ITEM_DOME_FOSSIL
	return

@ the second: a ROOT or a CLAW FOSSIL; the third: a SKULL or an ARMOR FOSSIL
VermilionCity_EventScript_ForemanFind2::
	msgbox VermilionCity_Text_ForemanFoundTwo
	dynmultichoice 0, 0, TRUE, 2, 0, DYN_MULTICHOICE_CB_NONE, VermilionCity_Text_RootFossil, VermilionCity_Text_ClawFossil
	setvar VAR_0x8009, ITEM_ROOT_FOSSIL
	call_if_eq VAR_RESULT, 1, VermilionCity_EventScript_ForemanClaw
	call GymHack_EventScript_GiveStatItem
	msgbox VermilionCity_Text_ForemanFossil
	release
	end

VermilionCity_EventScript_ForemanClaw::
	setvar VAR_0x8009, ITEM_CLAW_FOSSIL
	return

VermilionCity_EventScript_ForemanFind3::
	msgbox VermilionCity_Text_ForemanFoundTwo
	dynmultichoice 0, 0, TRUE, 2, 0, DYN_MULTICHOICE_CB_NONE, VermilionCity_Text_SkullFossil, VermilionCity_Text_ArmorFossil
	setvar VAR_0x8009, ITEM_SKULL_FOSSIL
	call_if_eq VAR_RESULT, 1, VermilionCity_EventScript_ForemanArmor
	call GymHack_EventScript_GiveStatItem
	msgbox VermilionCity_Text_ForemanFossil
	release
	end

VermilionCity_EventScript_ForemanArmor::
	setvar VAR_0x8009, ITEM_ARMOR_FOSSIL
	return

@ the fourth: it goes up (the player out in front of its door)
VermilionCity_EventScript_ForemanBuilt::
	msgbox VermilionCity_Text_ForemanBuilding
	closemessage
	fadescreen FADE_TO_BLACK
	playse SE_M_MEGA_KICK
	waitse
	delay 20
	playse SE_M_STRENGTH
	waitse
	delay 20
	playse SE_M_MEGA_KICK
	waitse
	delay 30
	warpsilent MAP_VERMILION_CITY, 35, 14
	waitstate
	release
	end

VermilionCity_EventScript_ForemanNo::
	msgbox VermilionCity_Text_ForemanNo
	release
	end

VermilionCity_EventScript_ForemanNotShiny::
	msgbox VermilionCity_Text_ForemanNotShiny
	release
	end

VermilionCity_EventScript_ForemanEgg::
	msgbox VermilionCity_Text_ForemanEgg
	release
	end

VermilionCity_EventScript_ForemanLastMon::
	msgbox VermilionCity_Text_ForemanLastMon
	release
	end

VermilionCity_EventScript_Machop::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_MACHOP, CRY_MODE_NORMAL
	msgbox VermilionCity_Text_Machop
	waitmoncry
	msgbox VermilionCity_Text_MachopStompingLandFlat
	release
	end

VermilionCity_EventScript_CitySign::
	msgbox VermilionCity_Text_CitySign, MSGBOX_SIGN
	end

VermilionCity_EventScript_SnorlaxNotice::
	msgbox VermilionCity_Text_SnorlaxBlockingRoute12, MSGBOX_SIGN
	end

VermilionCity_EventScript_PokemonFanClubSign::
	msgbox VermilionCity_Text_PokemonFanClubSign, MSGBOX_SIGN
	end

VermilionCity_EventScript_GymSign::
	lockall
	famechecker FAMECHECKER_LTSURGE, 0
	msgbox VermilionCity_Text_GymSign
	releaseall
	end

VermilionCity_EventScript_HarborSign::
	msgbox VermilionCity_Text_VermilionHarbor, MSGBOX_SIGN
	end

VermilionCity_Text_GrimerMultipliesInSludge::
	.string "We're careful about pollution here.\p"
	.string "We've heard GRIMER multiplies in\n"
	.string "toxic sludge.$"

VermilionCity_Text_DidYouSeeSSAnneInHarbor::
	.string "Did you see the S.S. ANNE moored\n"
	.string "in the harbor?$"

VermilionCity_Text_SSAnneHasDepartedForYear::
	.string "So, the S.S. ANNE has departed?\p"
	.string "She'll be back in VERMILION about\n"
	.string "this time next year.$"

VermilionCity_Text_BuildingOnThisLand::
	.string "I'm putting up a building on this\n"
	.string "plot of land. I own it all.\p"
	.string "My POKéMON is tamping the land\n"
	.string "to lay the foundation.$"

VermilionCity_Text_ForemanIntro::
	.string "I'm putting up a building on this\n"
	.string "plot of land. A big one!\p"
	.string "But my whole crew quit on me.\n"
	.string "All I've got left is this MACHOP.\p"
	.string "Say, could you lend me a hand?\n"
	.string "Another MACHOP would do it.$"

VermilionCity_Text_ForemanNext::
	.string "The building's coming along!\n"
	.string "But I could use a {STR_VAR_1} now.$"

VermilionCity_Text_ForemanAsk::
	.string "It'd stay on and work for me.\n"
	.string "Will you send me a {STR_VAR_1}?$"

VermilionCity_Text_ForemanWhich::
	.string "Which one?$"

VermilionCity_Text_ForemanThanks::
	.string "Ha! Now that's a worker!\n"
	.string "Welcome to the crew!$"

VermilionCity_Text_ForemanFound::
	.string "Hey, look what we dug up! An old\n"
	.string "fossil. Here, you have it!$"

VermilionCity_Text_ForemanFoundTwo::
	.string "Would you look at that! Two\n"
	.string "fossils in the dig. Pick one!$"

VermilionCity_Text_ForemanFossil::
	.string "They say the lab on CINNABAR can\n"
	.string "bring those back to life!$"

VermilionCity_Text_RootFossil::
	.string "ROOT FOSSIL$"

VermilionCity_Text_ClawFossil::
	.string "CLAW FOSSIL$"

VermilionCity_Text_SkullFossil::
	.string "SKULL FOSSIL$"

VermilionCity_Text_ArmorFossil::
	.string "ARMOR FOSSIL$"

VermilionCity_Text_ForemanBuilding::
	.string "A shiny MACHAMP! With a foreman\n"
	.string "like that, we'll be done tonight!\p"
	.string "All right, boys!\n"
	.string "Let's raise the roof!$"

VermilionCity_Text_ForemanNo::
	.string "Well, if you change your mind…$"

VermilionCity_Text_ForemanWrong::
	.string "That's not a {STR_VAR_1}!$"

VermilionCity_Text_ForemanNotShiny::
	.string "A MACHAMP! But I need a foreman\n"
	.string "that stands out on the site.\p"
	.string "One in a different color, you know?\n"
	.string "A shiny one!$"

VermilionCity_Text_ForemanEgg::
	.string "An EGG can't lift beams!$"

VermilionCity_Text_ForemanLastMon::
	.string "I can't take your only POKéMON!$"

VermilionCity_Text_Machop::
	.string "MACHOP: Guoh! Gogogoh!$"

VermilionCity_Text_MachopStompingLandFlat::
	.string "A MACHOP is stomping the land flat.$"

VermilionCity_Text_SSAnneVisitsOnceAYear::
	.string "The S.S. ANNE is a famous luxury\n"
	.string "cruise ship.\p"
	.string "It visits VERMILION once a year.$"

VermilionCity_Text_CitySign::
	.string "VERMILION CITY\n"
	.string "The Port of Exquisite Sunsets$"

VermilionCity_Text_SnorlaxBlockingRoute12::
	.string "NOTICE!\p"
	.string "ROUTE 12 may be blocked off by a\n"
	.string "sleeping POKéMON.\p"
	.string "Detour through ROCK TUNNEL to\n"
	.string "LAVENDER TOWN.\p"
	.string "VERMILION POLICE$"

VermilionCity_Text_PokemonFanClubSign::
	.string "POKéMON FAN CLUB\n"
	.string "All POKéMON fans welcome!$"

VermilionCity_Text_GymSign::
	.string "VERMILION CITY POKéMON GYM\n"
	.string "LEADER: LT. SURGE\l"
	.string "The Lightning American!$"

VermilionCity_Text_VermilionHarbor::
	.string "VERMILION HARBOR$"

VermilionCity_Text_WelcomeToTheSSAnne::
	.string "Welcome to the S.S. ANNE!$"

VermilionCity_Text_DoYouHaveATicket::
	.string "Welcome to the S.S. ANNE!\p"
	.string "Excuse me, do you have a ticket?$"

VermilionCity_Text_FlashedSSTicket::
	.string "{FONT_NORMAL}{PLAYER} flashed the S.S. TICKET!\p"
	.string "{FONT_MALE}Great!\n"
	.string "Welcome to the S.S. ANNE!$"

VermilionCity_Text_DontHaveNeededSSTicket::
	.string "{FONT_NORMAL}{PLAYER} doesn't have the needed\n"
	.string "S.S. TICKET.\p"
	.string "{FONT_MALE}Sorry!\p"
	.string "You need a ticket to get aboard.$"

VermilionCity_Text_TheShipSetSail::
	.string "The ship set sail.$"

VermilionCity_Text_BoardSeagallopTriPass::
	.string "Ah, you have a TRI-PASS.\p"
	.string "Would you like to board\n"
	.string "a SEAGALLOP ferry?$"

VermilionCity_Text_Seagallop7Departing::
	.string "Okay, everything's in order.\p"
	.string "SEAGALLOP HI-SPEED 7 will be\n"
	.string "departing immediately.$"

VermilionCity_Text_BoardSeagallopRainbowPass::
	.string "Ah, you have a RAINBOW PASS.\p"
	.string "Would you like to board\n"
	.string "a SEAGALLOP ferry?$"

VermilionCity_Text_OhMysticTicketTakeYouToNavelRock::
	.string "Oh! That's a MYSTICTICKET!\n"
	.string "Now that is rare.\p"
	.string "We'll be happy to take you to\n"
	.string "NAVEL ROCK anytime.$"

VermilionCity_Text_OhAuroraTicketTakeYouToBirthIsland::
	.string "Oh! That's an AURORATICKET!\n"
	.string "Now that is rare.\p"
	.string "We'll be happy to take you to\n"
	.string "BIRTH ISLAND anytime.$"

VermilionCity_Text_BoardSeagallopFerry::
	.string "Would you like to board\n"
	.string "a SEAGALLOP ferry?$"

VermilionCity_Text_Seagallop10Departing::
	.string "Okay, everything's in order for you\n"
	.string "to board a special ferry.\p"
	.string "SEAGALLOP HI-SPEED 10 will be\n"
	.string "departing immediately.$"

VermilionCity_Text_Seagallop12Departing::
	.string "Okay, everything's in order for you\n"
	.string "to board a special ferry.\p"
	.string "SEAGALLOP HI-SPEED 12 will be\n"
	.string "departing immediately.$"

VermilionCity_Text_Route2AideHasPackageForYou::
	.string "Oh, hello, {PLAYER}!\n"
	.string "How are you doing?\p"
	.string "It's me, one of PROF. OAK's AIDES.\p"
	.string "Did you meet the other AIDE?\p"
	.string "He had a package from PROF. OAK\n"
	.string "for you, {PLAYER}.\p"
	.string "He said he'd look for you around\n"
	.string "ROUTE 2, {PLAYER}.\p"
	.string "If you're in the ROUTE 2 area,\n"
	.string "please look for him.$"

