@ The Exotic Pokémon Safari's areas: what their legendaries share (each area's scripts are made by
@ tools/gym_hack/build_exotic_safari.py; the roamers' placement is src/exotic_safari.c).

@ A legendary (VAR_0x8004: its species) cries, then the battle. VAR_RESULT: the outcome
ExoticSafari_EventScript_LegendBattle::
	copyvar VAR_0x8008, VAR_0x8004
	playmoncry VAR_0x8008, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	special ExoticSafari_LegendForm
	setvar VAR_0x8005, EXOTIC_SAFARI_LEGEND_LEVEL
	special ExoticSafari_LegendLevel
	setvar VAR_0x8006, ITEM_NONE
	special CreateEnemyEventMon
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	waitstate
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	return

@ A legendary that stands in one place, after a battle that didn't catch it: gone for now (its
@ temp flag), back the next time the player comes into the area
ExoticSafari_EventScript_LegendGone::
	special ExoticSafari_MarkGone
	fadescreen FADE_TO_BLACK
	removeobject VAR_LAST_TALKED
	fadescreen FADE_FROM_BLACK
	msgbox ExoticSafari_Text_LegendGone
	release
	end

@ The spot where a legendary appears, while it isn't there (VAR_0x8004: which)
ExoticSafari_EventScript_Seal::
	special ExoticSafari_BufferSeal
	goto_if_eq VAR_RESULT, SEAL_RESULT_CAUGHT, ExoticSafari_EventScript_ReleaseAll
	goto_if_eq VAR_RESULT, SEAL_RESULT_AWAY, ExoticSafari_EventScript_SealBroken
	switch VAR_RESULT
	case SEAL_STAKES, ExoticSafari_EventScript_SealStakes
	case SEAL_MASKS, ExoticSafari_EventScript_SealMasks
	case SEAL_ROAMERS, ExoticSafari_EventScript_SealRoamers
	case SEAL_ITEM, ExoticSafari_EventScript_SealItem
	case SEAL_LAMPS, ExoticSafari_EventScript_SealLamps
	case SEAL_FRIENDSHIP, ExoticSafari_EventScript_SealFriendship
	case SEAL_DOJO, ExoticSafari_EventScript_SealDojo
	case SEAL_STONES, ExoticSafari_EventScript_SealStones
	case SEAL_DRAGON, ExoticSafari_EventScript_SealDragon
	case SEAL_DRAGON_LEAD, ExoticSafari_EventScript_SealDragonLead
	case SEAL_GUARDIANS, ExoticSafari_EventScript_SealGuardians
	case SEAL_FLUTE, ExoticSafari_EventScript_SealFlute
	case SEAL_LIGHT, ExoticSafari_EventScript_SealLight
	case SEAL_PARTS, ExoticSafari_EventScript_SealParts
	case SEAL_STEEL, ExoticSafari_EventScript_SealSteel
	case SEAL_SHADOW, ExoticSafari_EventScript_SealShadow
	case SEAL_STORM, ExoticSafari_EventScript_SealStorm
	case SEAL_CELLS, ExoticSafari_EventScript_SealCells
	case SEAL_BOTTLE, ExoticSafari_EventScript_SealBottle
	case SEAL_STEAM, ExoticSafari_EventScript_SealSteam
	case SEAL_DRAIN, ExoticSafari_EventScript_SealDrain
	case SEAL_CAUGHT, ExoticSafari_EventScript_SealCaught
	case SEAL_KEY, ExoticSafari_EventScript_SealKey
	case SEAL_LEVEL, ExoticSafari_EventScript_SealLevel
	case SEAL_BERRY, ExoticSafari_EventScript_SealBerry
	case SEAL_HP_EVS, ExoticSafari_EventScript_SealHpEvs
	case SEAL_REGIS, ExoticSafari_EventScript_SealRegis
	case SEAL_AZURE_FLUTE, ExoticSafari_EventScript_SealAzureFlute
	case SEAL_METEORITE, ExoticSafari_EventScript_SealMeteorite
	case SEAL_BLUE_ORB, ExoticSafari_EventScript_SealBlueOrb
	case SEAL_WISH, ExoticSafari_EventScript_SealWish
	case SEAL_GS_BALL, ExoticSafari_EventScript_SealGsBall
	case SEAL_REVISIT, ExoticSafari_EventScript_SealRevisit
	case SEAL_PARTY, EventMons_EventScript_SealParty
	msgbox ExoticSafari_Text_SealMastery
	releaseall
	end

@ Its seal is broken but it isn't here: away for this visit (it ran from a battle), or it comes now
@ (what it waited for was done while the player was in the area: a lead POKéMON swapped in...)
ExoticSafari_EventScript_SealBroken::
	special ExoticSafari_TryAppear
	goto_if_eq VAR_RESULT, TRUE, ExoticSafari_EventScript_Appear
	goto ExoticSafari_EventScript_SealAway

@ A static legendary comes (ExoticSafari_TryAppear: VAR_0x8005 its local id, VAR_0x8006 its species)
ExoticSafari_EventScript_Appear::
	addobject VAR_0x8005
	playmoncry VAR_0x8006, CRY_MODE_ENCOUNTER
	waitmoncry
	msgbox ExoticSafari_Text_Appeared
	releaseall
	end

ExoticSafari_EventScript_SealItem::
	msgbox ExoticSafari_Text_SealItem
	releaseall
	end

ExoticSafari_EventScript_SealLamps::
	msgbox ExoticSafari_Text_SealLamps
	releaseall
	end

ExoticSafari_EventScript_SealFriendship::
	msgbox ExoticSafari_Text_SealFriendship
	releaseall
	end

ExoticSafari_EventScript_SealDojo::
	msgbox ExoticSafari_Text_SealDojo
	releaseall
	end

ExoticSafari_EventScript_SealRevisit::
	msgbox ExoticSafari_Text_SealRevisit
	releaseall
	end

@ Kubfu caught (before its caught flag is set): the second one? (the first, set already)
ExoticSafari_EventScript_KubfuCaught::
	goto_if_unset FLAG_EXOTIC_LEGEND_KUBFU, ExoticSafari_EventScript_Return
	setflag FLAG_EXOTIC_KUBFU_TWICE
ExoticSafari_EventScript_Return::
	return

ExoticSafari_EventScript_SealStones::
	msgbox ExoticSafari_Text_SealStones
	releaseall
	end

ExoticSafari_EventScript_SealDragon::
	msgbox ExoticSafari_Text_SealDragon
	releaseall
	end

ExoticSafari_EventScript_SealDragonLead::
	msgbox ExoticSafari_Text_SealDragonLead
	releaseall
	end

ExoticSafari_EventScript_SealGuardians::
	msgbox ExoticSafari_Text_SealGuardians
	releaseall
	end

ExoticSafari_EventScript_SealFlute::
	msgbox ExoticSafari_Text_SealFlute
	releaseall
	end

ExoticSafari_EventScript_SealLight::
	msgbox ExoticSafari_Text_SealLight
	releaseall
	end

ExoticSafari_EventScript_SealParts::
	msgbox ExoticSafari_Text_SealParts
	releaseall
	end

ExoticSafari_EventScript_SealSteel::
	msgbox ExoticSafari_Text_SealSteel
	releaseall
	end

ExoticSafari_EventScript_SealShadow::
	msgbox ExoticSafari_Text_SealShadow
	releaseall
	end

ExoticSafari_EventScript_SealStorm::
	msgbox ExoticSafari_Text_SealStorm
	releaseall
	end

ExoticSafari_EventScript_SealCells::
	msgbox ExoticSafari_Text_SealCells
	releaseall
	end

ExoticSafari_EventScript_SealBottle::
	msgbox ExoticSafari_Text_SealBottle
	releaseall
	end

ExoticSafari_EventScript_SealSteam::
	msgbox ExoticSafari_Text_SealSteam
	releaseall
	end

ExoticSafari_EventScript_SealDrain::
	msgbox ExoticSafari_Text_SealDrain
	releaseall
	end

ExoticSafari_EventScript_SealCaught::
	msgbox ExoticSafari_Text_SealCaught
	releaseall
	end

ExoticSafari_EventScript_SealKey::
	msgbox ExoticSafari_Text_SealKey
	releaseall
	end

ExoticSafari_EventScript_SealLevel::
	msgbox ExoticSafari_Text_SealLevel
	releaseall
	end

ExoticSafari_EventScript_SealBerry::
	msgbox ExoticSafari_Text_SealBerry
	releaseall
	end

ExoticSafari_EventScript_SealHpEvs::
	msgbox ExoticSafari_Text_SealHpEvs
	releaseall
	end

ExoticSafari_EventScript_SealRegis::
	msgbox ExoticSafari_Text_SealRegis
	releaseall
	end

ExoticSafari_EventScript_SealAzureFlute::
	msgbox ExoticSafari_Text_SealAzureFlute
	releaseall
	end

ExoticSafari_EventScript_SealMeteorite::
	msgbox ExoticSafari_Text_SealMeteorite
	releaseall
	end

ExoticSafari_EventScript_SealBlueOrb::
	goto_if_eq VAR_0x8005, TRUE, ExoticSafari_EventScript_SealBlueOrbFound
	msgbox ExoticSafari_Text_SealBlueOrb
	releaseall
	end

ExoticSafari_EventScript_SealBlueOrbFound::
	msgbox ExoticSafari_Text_SealBlueOrbFound
	releaseall
	end

ExoticSafari_EventScript_SealWish::
	msgbox ExoticSafari_Text_SealWish
	releaseall
	end

@ Hoenn: the Sealed Chamber's tablet (the rock by the Island Cave): Wailord first, Relicanth last
@ in the party, and the three Regi chambers' inner walls can be opened
ExoticSafari_EventScript_SealedTablet::
	lockall
	goto_if_set FLAG_EXOTIC_REGIS_UNSEALED, ExoticSafari_EventScript_TabletDone
	msgbox ExoticSafari_Text_Tablet
	specialvar VAR_RESULT, CheckRelicanthWailord
	goto_if_eq VAR_RESULT, FALSE, ExoticSafari_EventScript_ReleaseAll
	msgbox ExoticSafari_Text_TabletAnswers
	closemessage
	playse SE_M_STRENGTH
	special DoSealedChamberShakingEffect_Long
	waitstate
	setflag FLAG_EXOTIC_REGIS_UNSEALED
	msgbox ExoticSafari_Text_TabletOpened
	releaseall
	end

ExoticSafari_EventScript_TabletDone::
	msgbox ExoticSafari_Text_TabletDone
	releaseall
	end

@ Hoenn: a Regi chamber's inner wall (VAR_0x8004: 0 Desert Ruins, 1 Island Cave, 2 Ancient Tomb)
ExoticSafari_EventScript_RegiWall::
	special ExoticSafari_RegiWall
	goto_if_eq VAR_RESULT, 2, ExoticSafari_EventScript_RegiWallOpen
	goto_if_eq VAR_RESULT, 0, ExoticSafari_EventScript_RegiWallSealed
	switch VAR_0x8004
	case 0, ExoticSafari_EventScript_RegiWallRock
	case 1, ExoticSafari_EventScript_RegiWallIce
	msgbox ExoticSafari_Text_RegiWallSteel
	specialvar VAR_RESULT, ExoticSafari_RegiLeadKnows
	goto_if_eq VAR_RESULT, FALSE, ExoticSafari_EventScript_ReleaseAll
	msgbox ExoticSafari_Text_RegiWallFlash
	goto ExoticSafari_EventScript_RegiWallOpens

ExoticSafari_EventScript_RegiWallRock::
	msgbox ExoticSafari_Text_RegiWallRock
	specialvar VAR_RESULT, ExoticSafari_RegiLeadKnows
	goto_if_eq VAR_RESULT, FALSE, ExoticSafari_EventScript_ReleaseAll
	msgbox ExoticSafari_Text_RegiWallRockSmash
	goto ExoticSafari_EventScript_RegiWallOpens

ExoticSafari_EventScript_RegiWallIce::
	msgbox ExoticSafari_Text_RegiWallIce
	special ExoticSafari_StartStillness
	releaseall
	end

ExoticSafari_EventScript_RegiWallOpens::
	closemessage
	special ExoticSafari_OpenRegiWall
	waitse
	releaseall
	end

ExoticSafari_EventScript_RegiWallOpen::
	msgbox ExoticSafari_Text_RegiWallOpen
	releaseall
	end

ExoticSafari_EventScript_RegiWallSealed::
	msgbox ExoticSafari_Text_RegiWallSealed
	releaseall
	end

ExoticSafari_EventScript_SealGsBall::
	msgbox ExoticSafari_Text_SealGsBall
	releaseall
	end

@ Johto: a keepsake picked up (its flag set; STR_VAR_1 its name)
ExoticSafari_EventScript_JohtoKeepsake::
	removeobject VAR_LAST_TALKED
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	message ExoticSafari_Text_FoundKey
	waitmessage
	waitfanfare
	msgbox ExoticSafari_Text_KeyKept
	release
	end

@ Johto: the old man in New Bark Town (the roaming beasts: every area visited)
ExoticSafari_EventScript_BeastSage::
	lock
	faceplayer
	special ExoticSafari_BeastSage
	goto_if_eq VAR_RESULT, 6, ExoticSafari_EventScript_BeastSageDone
	msgbox ExoticSafari_Text_BeastSage
	release
	end

ExoticSafari_EventScript_BeastSageDone::
	msgbox ExoticSafari_Text_BeastSageDone
	release
	end

@ Johto: the Dragon's Den elder, at the shrine's door until the Silver Wing is found
ExoticSafari_EventScript_DenElder::
	lock
	faceplayer
	msgbox ExoticSafari_Text_DenElder
	release
	end

@ Johto: a stone panel in the Ruins of Alph (VAR_0x8004: which)
ExoticSafari_EventScript_UnownPanel::
	special ExoticSafari_UnownPanel
	goto_if_eq VAR_RESULT, 0, ExoticSafari_EventScript_UnownPanelOpen
	switch VAR_0x8004
	case 0, ExoticSafari_EventScript_UnownPanelEscape
	case 1, ExoticSafari_EventScript_UnownPanelWater
	case 2, ExoticSafari_EventScript_UnownPanelLight
	msgbox ExoticSafari_Text_UnownPanelHoOh
	goto ExoticSafari_EventScript_UnownPanelAnswer

ExoticSafari_EventScript_UnownPanelEscape::
	msgbox ExoticSafari_Text_UnownPanelEscape
	goto ExoticSafari_EventScript_UnownPanelAnswer

ExoticSafari_EventScript_UnownPanelWater::
	msgbox ExoticSafari_Text_UnownPanelWater
	goto ExoticSafari_EventScript_UnownPanelAnswer

ExoticSafari_EventScript_UnownPanelLight::
	msgbox ExoticSafari_Text_UnownPanelLight
	goto ExoticSafari_EventScript_UnownPanelAnswer

ExoticSafari_EventScript_UnownPanelAnswer::
	goto_if_eq VAR_RESULT, 1, ExoticSafari_EventScript_ReleaseAll
	closemessage
	playse SE_M_STRENGTH
	special DoSealedChamberShakingEffect_Short
	waitstate
	goto_if_eq VAR_RESULT, 3, ExoticSafari_EventScript_UnownPanelsAll
	msgbox ExoticSafari_Text_UnownPanelOpens
	releaseall
	end

ExoticSafari_EventScript_UnownPanelsAll::
	msgbox ExoticSafari_Text_UnownPanelsAll
	releaseall
	end

ExoticSafari_EventScript_UnownPanelOpen::
	msgbox ExoticSafari_Text_UnownPanelOpen
	releaseall
	end

@ Johto: the Red Gyarados in the middle of the Lake of Rage (shiny; not a legendary)
ExoticSafari_EventScript_RedGyarados::
	lock
	faceplayer
	playmoncry SPECIES_GYARADOS, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setvar VAR_0x8004, SPECIES_GYARADOS
	setvar VAR_0x8005, 55
	setvar VAR_0x8006, ITEM_NONE
	special CreateEnemyEventMon
	createmon 1, 0, SPECIES_GYARADOS, 55, shinyMode=SHINY_MODE_ALWAYS
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLegendaryBattle
	waitstate
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_RedGyaradosGone
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_RED_GYARADOS
	release
	end

ExoticSafari_EventScript_RedGyaradosGone::
	fadescreen FADE_TO_BLACK
	removeobject VAR_LAST_TALKED
	fadescreen FADE_FROM_BLACK
	msgbox ExoticSafari_Text_LegendGone
	release
	end

@ Sinnoh: the Snowpoint elder. Once every gym type is mastered he gives the Azure Flute (Arceus,
@ at the top of the Hall of Origin, answers it); before that, a hint
ExoticSafari_EventScript_SnowpointElder::
	lock
	faceplayer
	goto_if_set FLAG_EXOTIC_AZURE_FLUTE, ExoticSafari_EventScript_ElderAfter
	special ExoticSafari_AllTypesMastered
	goto_if_eq VAR_RESULT, FALSE, ExoticSafari_EventScript_ElderHint
	msgbox ExoticSafari_Text_ElderFlute
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	message ExoticSafari_Text_ReceivedAzureFlute
	waitmessage
	waitfanfare
	setflag FLAG_EXOTIC_AZURE_FLUTE
	msgbox ExoticSafari_Text_ElderAfter
	release
	end

ExoticSafari_EventScript_ElderHint::
	msgbox ExoticSafari_Text_ElderHint
	release
	end

ExoticSafari_EventScript_ElderAfter::
	msgbox ExoticSafari_Text_ElderAfter
	release
	end

@ Sinnoh: Rotom's room in Snowpoint. Each appliance (VAR_0x8006: which) takes a Rotom in, or lets
@ it back out
ExoticSafari_EventScript_Appliance::
	lock
	msgbox ExoticSafari_Text_ApplianceAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, ExoticSafari_EventScript_ReleaseOnly
	special ChooseRotomForAppliance
	setvar VAR_ITEM_ID, ITEM_NONE
	release
	end

ExoticSafari_EventScript_ReleaseOnly::
	release
	end

ExoticSafari_EventScript_RotomRoomGuide::
	msgbox ExoticSafari_Text_RotomRoomGuide, MSGBOX_NPC
	end

@ One of Unova's keepsakes picked up (VAR_0x8004: which)
ExoticSafari_EventScript_Key::
	special ExoticSafari_TakeKey
	removeobject VAR_LAST_TALKED
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	message ExoticSafari_Text_FoundKey
	waitmessage
	waitfanfare
	msgbox ExoticSafari_Text_KeyKept
	release
	end

@ The Giant Chasm's old man: the DNA Splicers, once Kyurem is caught
ExoticSafari_EventScript_ChasmElder::
	lock
	faceplayer
	goto_if_set FLAG_EXOTIC_GOT_DNA_SPLICERS, ExoticSafari_EventScript_ChasmElderAfter
	setvar VAR_0x8004, SPECIES_KYUREM
	special ExoticSafari_LegendCaught
	goto_if_eq VAR_RESULT, FALSE, ExoticSafari_EventScript_ChasmElderNotYet
	msgbox ExoticSafari_Text_ChasmElderGive
	giveitem ITEM_DNA_SPLICERS
	goto_if_eq VAR_RESULT, FALSE, ExoticSafari_EventScript_ChasmElderEnd
	setflag FLAG_EXOTIC_GOT_DNA_SPLICERS
ExoticSafari_EventScript_ChasmElderAfter::
	msgbox ExoticSafari_Text_ChasmElderAfter
ExoticSafari_EventScript_ChasmElderEnd::
	release
	end

ExoticSafari_EventScript_ChasmElderNotYet::
	msgbox ExoticSafari_Text_ChasmElderNotYet
	release
	end

@ Skyarrow Bridge's shrine keeper: the Reveal Glass, once Thundurus and Landorus are caught
ExoticSafari_EventScript_ShrineKeeper::
	lock
	faceplayer
	goto_if_set FLAG_EXOTIC_GOT_REVEAL_GLASS, ExoticSafari_EventScript_ShrineKeeperAfter
	setvar VAR_0x8004, SPECIES_THUNDURUS
	special ExoticSafari_LegendCaught
	goto_if_eq VAR_RESULT, FALSE, ExoticSafari_EventScript_ShrineKeeperNotYet
	setvar VAR_0x8004, SPECIES_LANDORUS
	special ExoticSafari_LegendCaught
	goto_if_eq VAR_RESULT, FALSE, ExoticSafari_EventScript_ShrineKeeperNotYet
	msgbox ExoticSafari_Text_ShrineKeeperGive
	giveitem ITEM_REVEAL_GLASS
	goto_if_eq VAR_RESULT, FALSE, ExoticSafari_EventScript_ChasmElderEnd
	setflag FLAG_EXOTIC_GOT_REVEAL_GLASS
ExoticSafari_EventScript_ShrineKeeperAfter::
	msgbox ExoticSafari_Text_ShrineKeeperAfter
	release
	end

ExoticSafari_EventScript_ShrineKeeperNotYet::
	msgbox ExoticSafari_Text_ShrineKeeperNotYet
	release
	end

@ A Zygarde Cell picked up (VAR_0x8004: which)
ExoticSafari_EventScript_Cell::
	special ExoticSafari_TakeCell
	removeobject VAR_LAST_TALKED
	playfanfare MUS_OBTAIN_ITEM
	message ExoticSafari_Text_FoundCell
	waitmessage
	waitfanfare
	goto_if_eq VAR_RESULT, 0, ExoticSafari_EventScript_LastCell
	msgbox ExoticSafari_Text_CellsLeft
	release
	end

ExoticSafari_EventScript_LastCell::
	msgbox ExoticSafari_Text_LastCell
	giveitem ITEM_ZYGARDE_CUBE
	msgbox ExoticSafari_Text_ZygardeCube
	release
	end

@ Zygarde, before its battle: whole once all its Cells are found; before that, its 10% Forme (the
@ player can wait)
ExoticSafari_EventScript_ZygardePre::
	special ExoticSafari_CountCells
	goto_if_eq VAR_RESULT, 0, ExoticSafari_EventScript_ZygardeWhole
	msgbox ExoticSafari_Text_ZygardePartial, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, ExoticSafari_EventScript_ZygardeWait
	return

ExoticSafari_EventScript_ZygardeWhole::
	msgbox ExoticSafari_Text_ZygardeWhole
	return

ExoticSafari_EventScript_ZygardeWait::
	msgbox ExoticSafari_Text_ZygardeWait
	release
	end

@ Glittering Cave: the Prison Bottle on its pedestal. Taken, it calls Hoopa through a ring
ExoticSafari_EventScript_PrisonBottle::
	lock
	msgbox ExoticSafari_Text_PrisonBottle, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, ExoticSafari_EventScript_PrisonBottleLeft
	removeobject VAR_LAST_TALKED
	setflag FLAG_EXOTIC_PRISON_BOTTLE
	giveitem ITEM_PRISON_BOTTLE
	msgbox ExoticSafari_Text_PrisonBottleShakes
	setvar VAR_0x8004, EXOTIC_SAFARI_STATIC_HOOPA
	special ExoticSafari_TryAppear
	goto_if_eq VAR_RESULT, TRUE, ExoticSafari_EventScript_Appear
	release
	end

ExoticSafari_EventScript_PrisonBottleLeft::
	release
	end

@ Lumiose's Furfrou groomer: a trim for ¥500
ExoticSafari_EventScript_Groomer::
	lock
	faceplayer
	msgbox ExoticSafari_Text_GroomerIntro, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, ExoticSafari_EventScript_GroomerBye
	checkmoney 500
	goto_if_eq VAR_RESULT, FALSE, ExoticSafari_EventScript_GroomerNoMoney
	msgbox ExoticSafari_Text_GroomerWhich
	special ChoosePartyMon
	waitstate
	goto_if_ge VAR_0x8004, PARTY_SIZE, ExoticSafari_EventScript_GroomerBye
	special ExoticSafari_IsFurfrou
	goto_if_eq VAR_RESULT, FALSE, ExoticSafari_EventScript_GroomerNotFurfrou
	message ExoticSafari_Text_GroomerStyle
	waitmessage
	dynmultichoice 0, 0, FALSE, 5, 0, DYN_MULTICHOICE_CB_NONE, ExoticSafari_Text_TrimHeart, ExoticSafari_Text_TrimStar, ExoticSafari_Text_TrimDiamond, ExoticSafari_Text_TrimDebutante, ExoticSafari_Text_TrimMatron, ExoticSafari_Text_TrimDandy, ExoticSafari_Text_TrimLaReine, ExoticSafari_Text_TrimKabuki, ExoticSafari_Text_TrimPharaoh, ExoticSafari_Text_TrimNatural
	closemessage
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, ExoticSafari_EventScript_GroomerBye
	copyvar VAR_0x8005, VAR_RESULT
	special ExoticSafari_TrimFurfrou
	goto_if_eq VAR_RESULT, FALSE, ExoticSafari_EventScript_GroomerSame
	removemoney 500
	fadescreen FADE_TO_BLACK
	playse SE_USE_ITEM
	delay 60
	fadescreen FADE_FROM_BLACK
	msgbox ExoticSafari_Text_GroomerDone
	release
	end

ExoticSafari_EventScript_GroomerSame::
	msgbox ExoticSafari_Text_GroomerSame
	release
	end

ExoticSafari_EventScript_GroomerNotFurfrou::
	msgbox ExoticSafari_Text_GroomerNotFurfrou
	release
	end

ExoticSafari_EventScript_GroomerNoMoney::
	msgbox ExoticSafari_Text_GroomerNoMoney
	release
	end

ExoticSafari_EventScript_GroomerBye::
	msgbox ExoticSafari_Text_GroomerBye
	release
	end

@ A relic of the Alola legendaries picked up (VAR_0x8004: which)
ExoticSafari_EventScript_Relic::
	special ExoticSafari_TakeRelic
	removeobject VAR_LAST_TALKED
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	goto_if_ne VAR_RESULT, 0xFF, ExoticSafari_EventScript_Part
	message ExoticSafari_Text_FoundRelic
	waitmessage
	waitfanfare
	msgbox ExoticSafari_Text_RelicKept
	release
	end

ExoticSafari_EventScript_Part::
	message ExoticSafari_Text_FoundPart
	waitmessage
	waitfanfare
	msgbox ExoticSafari_Text_PartsLeft
	release
	end

@ Hano Beach's caretaker: Type: Null, for a trainer who has caught Alola's three first partners
ExoticSafari_EventScript_Caretaker::
	lock
	faceplayer
	goto_if_set FLAG_EXOTIC_LEGEND_TYPE_NULL, ExoticSafari_EventScript_CaretakerAfter
	special ExoticSafari_CountStarters
	goto_if_ne VAR_RESULT, 0, ExoticSafari_EventScript_CaretakerNotYet
	msgbox ExoticSafari_Text_CaretakerGive
	givemon SPECIES_TYPE_NULL, EXOTIC_SAFARI_LEGEND_LEVEL
	goto_if_eq VAR_RESULT, MON_CANT_GIVE, ExoticSafari_EventScript_CaretakerFull
	setflag FLAG_EXOTIC_LEGEND_TYPE_NULL
	playfanfare MUS_LEVEL_UP
	message ExoticSafari_Text_CaretakerGot
	waitmessage
	waitfanfare
	msgbox ExoticSafari_Text_CaretakerAfter
	release
	end

ExoticSafari_EventScript_CaretakerNotYet::
	buffernumberstring STR_VAR_2, VAR_RESULT
	msgbox ExoticSafari_Text_CaretakerNotYet
	release
	end

ExoticSafari_EventScript_CaretakerFull::
	msgbox ExoticSafari_Text_CaretakerFull
	release
	end

ExoticSafari_EventScript_CaretakerAfter::
	msgbox ExoticSafari_Text_CaretakerAfter
	release
	end

ExoticSafari_EventScript_SealStakes::
	msgbox ExoticSafari_Text_SealStakes
	releaseall
	end

ExoticSafari_EventScript_SealMasks::
	msgbox ExoticSafari_Text_SealMasks
	releaseall
	end

ExoticSafari_EventScript_SealRoamers::
	msgbox ExoticSafari_Text_SealRoamers
	releaseall
	end

ExoticSafari_EventScript_SealAway::
	msgbox ExoticSafari_Text_SealAway
ExoticSafari_EventScript_ReleaseAll::
	releaseall
	end

@ A Treasure of Ruin's stake (VAR_0x8004: its color)
ExoticSafari_EventScript_Stake::
	lock
	copyvar VAR_0x8009, VAR_0x8004
	special ExoticSafari_CountStakes
	msgbox ExoticSafari_Text_Stake, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, ExoticSafari_EventScript_Release
	playse SE_M_STRENGTH
	removeobject VAR_LAST_TALKED
	waitse
	copyvar VAR_0x8004, VAR_0x8009
	special ExoticSafari_CountStakes
	goto_if_eq VAR_RESULT, 0, ExoticSafari_EventScript_LastStake
	msgbox ExoticSafari_Text_StakePulled
	release
	end

ExoticSafari_EventScript_LastStake::
	playse SE_M_ROCK_THROW
	msgbox ExoticSafari_Text_LastStake
	release
	end

ExoticSafari_EventScript_Rope::
	msgbox ExoticSafari_Text_Rope, MSGBOX_NPC
	end

@ A roamer: whatever happens, it's gone from this area (caught, or off to roam somewhere else)
ExoticSafari_EventScript_RoamerGone::
	fadescreen FADE_TO_BLACK
	removeobject VAR_LAST_TALKED
	fadescreen FADE_FROM_BLACK
	goto_if_eq VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_RoamerEnd
	msgbox ExoticSafari_Text_RoamerRanOff
ExoticSafari_EventScript_RoamerEnd::
	release
	end

ExoticSafari_EventScript_Release::
	release
	end

@ Zarude's lamps: a big mushroom in Glimwood touched (VAR_0x8004: which)
ExoticSafari_EventScript_Lamp::
	lockall
	special ExoticSafari_Lamp
	goto_if_eq VAR_RESULT, 0, ExoticSafari_EventScript_LampLit
	playse SE_M_REFLECT
	waitse
	goto_if_eq VAR_RESULT, 2, ExoticSafari_EventScript_LastLamp
	msgbox ExoticSafari_Text_LampLights
	releaseall
	end

ExoticSafari_EventScript_LampLit::
	msgbox ExoticSafari_Text_LampLit
	releaseall
	end

ExoticSafari_EventScript_LastLamp::
	msgbox ExoticSafari_Text_LastLamp
	setvar VAR_0x8004, EXOTIC_SAFARI_STATIC_ZARUDE
	special ExoticSafari_TryAppear
	goto_if_eq VAR_RESULT, TRUE, ExoticSafari_EventScript_Appear
	releaseall
	end

@ Regieleki's stepping stones: one stepped on (VAR_0x8004: which)
@ (a step trigger: locked and released, or it would fire again every frame)
ExoticSafari_EventScript_Stone::
	lockall
	special ExoticSafari_Stone
	goto_if_eq VAR_RESULT, 1, ExoticSafari_EventScript_StoneLit
	goto_if_eq VAR_RESULT, 2, ExoticSafari_EventScript_LastStone
	goto_if_eq VAR_RESULT, 3, ExoticSafari_EventScript_StonesOut
	releaseall
	end

ExoticSafari_EventScript_StoneLit::
	playse SE_M_THUNDERBOLT2
	releaseall
	end

ExoticSafari_EventScript_StonesOut::
	playse SE_FAILURE
	releaseall
	end

ExoticSafari_EventScript_LastStone::
	playse SE_M_THUNDER_WAVE
	waitse
	msgbox ExoticSafari_Text_LastStone
	setvar VAR_0x8004, EXOTIC_SAFARI_STATIC_REGIELEKI
	special ExoticSafari_TryAppear
	goto_if_eq VAR_RESULT, TRUE, ExoticSafari_EventScript_Appear
	releaseall
	end

@ Kubfu's dojo master, in Wyndon Stadium's lobby: beat her and Kubfu comes to the pitch
ExoticSafari_EventScript_DojoMaster::
	lock
	faceplayer
	goto_if_set FLAG_EXOTIC_DOJO_WON, ExoticSafari_EventScript_DojoAfter
	msgbox ExoticSafari_Text_DojoIntro, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, ExoticSafari_EventScript_DojoDecline
	special ExoticSafari_SetupDojo
	goto_if_eq VAR_RESULT, FALSE, ExoticSafari_EventScript_Release
	msgbox ExoticSafari_Text_DojoGo
	closemessage
	cleartrainerflag TRAINER_GYM_CHALLENGER
	setflag FLAG_GYM_CHALLENGE_BATTLE
	trainerbattle_no_intro TRAINER_GYM_CHALLENGER, ExoticSafari_Text_DojoDefeat
	clearflag FLAG_GYM_CHALLENGE_BATTLE
	specialvar VAR_RESULT, GetBattleOutcome
	special HealPlayerParty
	goto_if_ne VAR_RESULT, B_OUTCOME_WON, ExoticSafari_EventScript_DojoLost
	setflag FLAG_EXOTIC_DOJO_WON
	msgbox ExoticSafari_Text_DojoWon
	release
	end

ExoticSafari_EventScript_DojoAfter::
	goto_if_set FLAG_EXOTIC_KUBFU_TWICE, ExoticSafari_EventScript_DojoBothCaught
	goto_if_set FLAG_EXOTIC_LEGEND_KUBFU, ExoticSafari_EventScript_DojoSecond
	msgbox ExoticSafari_Text_DojoAfter
	release
	end

ExoticSafari_EventScript_DojoSecond::
	msgbox ExoticSafari_Text_DojoSecond
	release
	end

ExoticSafari_EventScript_DojoBothCaught::
	msgbox ExoticSafari_Text_DojoBothCaught
	release
	end

ExoticSafari_EventScript_DojoDecline::
	msgbox ExoticSafari_Text_DojoDecline
	release
	end

ExoticSafari_EventScript_DojoLost::
	msgbox ExoticSafari_Text_DojoLost
	release
	end

ExoticSafari_Text_LegendGone:
	.string "It disappeared…\n"
	.string "Maybe it'll be back later.$"

ExoticSafari_Text_SealStakes:
	.string "An eerie aura hangs over this spot.\p"
	.string "Something is sealed here, held down\n"
	.string "by {STR_VAR_1} stakes. {STR_VAR_2} of them still\l"
	.string "stand somewhere in the zone.$"

ExoticSafari_Text_SealMasks:
	.string "Little footprints circle this spot,\n"
	.string "as if someone keeps coming back to\l"
	.string "look for something.\p"
	.string "There's a broken mask in the grass.\n"
	.string "Some masks must have gone missing…\l"
	.string "{STR_VAR_2} of them, by the look of it.$"

ExoticSafari_Text_SealRoamers:
	.string "A sweet, sickly smell hangs here…\p"
	.string "Whatever lives here seems to be\n"
	.string "waiting for its loyal followers.\l"
	.string "{STR_VAR_2} of them still roam the zone.$"

ExoticSafari_Text_SealMastery:
	.string "The ground glows with every color\n"
	.string "at once…\p"
	.string "Something here is waiting for a\n"
	.string "trainer who has mastered every\l"
	.string "type of POKéMON.$"

ExoticSafari_Text_SealAway:
	.string "Something was here just a moment\n"
	.string "ago…$"

ExoticSafari_Text_Stake:
	.string "A {STR_VAR_1} stake is planted firmly in\n"
	.string "the ground. It glows eerily…\p"
	.string "Pull it out?$"

ExoticSafari_Text_StakePulled:
	.string "{PLAYER} pulled out the {STR_VAR_1} stake!\p"
	.string "{STR_VAR_1} stakes left to find: {STR_VAR_2}$"

ExoticSafari_Text_LastStake:
	.string "{PLAYER} pulled out the last\n"
	.string "{STR_VAR_1} stake!\p"
	.string "Somewhere in the zone, something\n"
	.string "rumbles…$"

ExoticSafari_Text_Rope:
	.string "A velvet rope. The stairs are\n"
	.string "off-limits to visitors.$"

ExoticSafari_Text_RoamerRanOff:
	.string "It ran off to roam somewhere else\n"
	.string "in the zone!$"

ExoticSafari_Text_Appeared:
	.string "A POKéMON appeared!$"

ExoticSafari_Text_SealItem:
	.string "An old stone plinth stands here,\n"
	.string "worn by the snow.\p"
	.string "There's a hollow in it, the shape\n"
	.string "of a {STR_VAR_1}…\p"
	.string "Whatever it held must have been\n"
	.string "lost somewhere in the zone.$"

ExoticSafari_Text_SealLamps:
	.string "Something rustles in the vines\n"
	.string "above, then goes still…\p"
	.string "It seems to be waiting for the\n"
	.string "forest to light up.\p"
	.string "{STR_VAR_2} of Glimwood's big mushrooms\n"
	.string "are still dark.$"

ExoticSafari_Text_SealFriendship:
	.string "Pink flowers bloom all over this\n"
	.string "little glade.\p"
	.string "The air feels warm, full of love…\p"
	.string "Whatever lives here might show\n"
	.string "itself to a POKéMON that loves its\l"
	.string "trainer with all its heart.$"

ExoticSafari_Text_SealDojo:
	.string "Little paw prints are all over\n"
	.string "the pitch, as if something has\l"
	.string "been training here.\p"
	.string "There's a note: “Train with us?\n"
	.string "Beat BEA of the Master Dojo\l"
	.string "first! She waits in the lobby.”$"

ExoticSafari_Text_SealStones:
	.string "The air here crackles with static…\p"
	.string "Something seems to be waiting for\n"
	.string "the lightning to fall.$"

ExoticSafari_Text_SealDragon:
	.string "Ancient carvings of a dragon cover\n"
	.string "the rock.\p"
	.string "There's a hollow in the carving,\n"
	.string "the shape of a {STR_VAR_1}…$"

ExoticSafari_Text_SealDragonLead:
	.string "Ancient carvings of a dragon cover\n"
	.string "the rock.\p"
	.string "The {STR_VAR_1} hums… The carvings\n"
	.string "seem to be waiting for a\l"
	.string "DRAGON-type POKéMON in the lead.$"

ExoticSafari_Text_LampLights:
	.string "{PLAYER} touched the big mushroom.\n"
	.string "It lit up with a soft glow!\p"
	.string "Big mushrooms still dark: {STR_VAR_2}$"

ExoticSafari_Text_LampLit:
	.string "The big mushroom glows softly.$"

ExoticSafari_Text_LastLamp:
	.string "{PLAYER} touched the big mushroom.\n"
	.string "It lit up with a soft glow!\p"
	.string "Every big mushroom in Glimwood is\n"
	.string "glowing now…\p"
	.string "Deep in the forest, something\n"
	.string "stirs in the vines!$"

ExoticSafari_Text_StonesSign:
	.string "The lightning falls from the sky\n"
	.string "to the ground, zigzagging as it\l"
	.string "goes.$"

ExoticSafari_Text_LastStone:
	.string "The stepping stones blaze with\n"
	.string "light!\p"
	.string "Something crackles to life at the\n"
	.string "top of the bolt!$"

ExoticSafari_Text_DojoIntro:
	.string "I'm BEA. I train at the Master\n"
	.string "Dojo, and a little KUBFU trains\l"
	.string "with us.\p"
	.string "It only trusts trainers who've\n"
	.string "proven their strength. Beat me,\l"
	.string "and it'll train with you.\p"
	.string "Will you battle?$"

ExoticSafari_Text_DojoGo:
	.string "…Then show me your training!$"

ExoticSafari_Text_DojoDefeat:
	.string "…You've trained hard. Well done.$"

ExoticSafari_Text_DojoWon:
	.string "KUBFU will be waiting for you on\n"
	.string "the pitch. Train it well.$"

ExoticSafari_Text_DojoAfter:
	.string "KUBFU is waiting for you on the\n"
	.string "pitch.$"

ExoticSafari_Text_DojoSecond:
	.string "That KUBFU of yours has a sibling,\n"
	.string "you know. It trains all over GALAR.\p"
	.string "Walk every part of GALAR again, and\n"
	.string "it'll come to the pitch to find you.$"

ExoticSafari_Text_DojoBothCaught:
	.string "Two KUBFU, two paths. Train them\n"
	.string "well!$"

ExoticSafari_Text_SealRevisit:
	.string "Small footprints lead off the pitch…\p"
	.string "Another KUBFU is training out in\n"
	.string "GALAR. It may come here once you've\l"
	.string "been back to every area: {STR_VAR_2}\l"
	.string "more to go.$"

ExoticSafari_Text_DojoDecline:
	.string "…Come back when you're ready.$"

ExoticSafari_Text_DojoLost:
	.string "Training is everything. Come back\n"
	.string "when you're stronger.$"

ExoticSafari_Text_SealGuardians:
	.string "Stars glitter in the sand here, as if\n"
	.string "something fell from the sky…\p"
	.string "It seems to hide from the island\n"
	.string "guardians. {STR_VAR_2} of the TAPUs still\l"
	.string "roam the zone.$"

ExoticSafari_Text_SealFlute:
	.string "An old altar. A carving shows a\n"
	.string "flute being played to the sky…\p"
	.string "Perhaps the {STR_VAR_1} would call\n"
	.string "whatever this altar is for.$"

ExoticSafari_Text_SealLight:
	.string "The light here seems to drain away\n"
	.string "into the dark…\p"
	.string "Something here hungers for the light\n"
	.string "of the sun and the moon.$"

ExoticSafari_Text_SealParts:
	.string "Little cogs lie in the sand, as if a\n"
	.string "machine had come apart here…\p"
	.string "{STR_VAR_2} of its MACHINE PARTS are still\n"
	.string "lost somewhere in the zone.$"

ExoticSafari_Text_SealSteel:
	.string "Nuts and bolts are scattered about,\n"
	.string "chewed on…\p"
	.string "Whatever left them might come out\n"
	.string "for a STEEL-type POKéMON in the\l"
	.string "lead.$"

ExoticSafari_Text_SealShadow:
	.string "A shadow flickers at the edge of the\n"
	.string "shrine, then it's gone…\p"
	.string "It might show itself to a lead\n"
	.string "POKéMON of the FIGHTING or GHOST\l"
	.string "type.$"

ExoticSafari_Text_SealStorm:
	.string "Scorch marks streak the planks, as\n"
	.string "if lightning ran across them…\p"
	.string "Whatever made them must come with\n"
	.string "the thunderstorms.$"

ExoticSafari_Text_FoundRelic:
	.string "{PLAYER} found the {STR_VAR_1}!$"

ExoticSafari_Text_FoundPart:
	.string "{PLAYER} found a MACHINE PART!$"

ExoticSafari_Text_PartsLeft:
	.string "It's an odd little cog, finely made.\n"
	.string "MACHINE PARTS still to find: {STR_VAR_2}$"

ExoticSafari_Text_RelicKept:
	.string "{PLAYER} put the {STR_VAR_1} away\n"
	.string "carefully.$"

ExoticSafari_Text_CaretakerNotYet:
	.string "I look after this resort… and a\n"
	.string "POKéMON in its care, TYPE: NULL.\p"
	.string "I'd only trust it to a trainer who\n"
	.string "has caught Alola's three first\l"
	.string "partners: ROWLET, LITTEN, POPPLIO.\p"
	.string "You haven't caught {STR_VAR_1} yet.\n"
	.string "({STR_VAR_2} to go.)$"

ExoticSafari_Text_CaretakerGive:
	.string "ROWLET, LITTEN and POPPLIO… you've\n"
	.string "caught them all!\p"
	.string "Then please, take TYPE: NULL. It\n"
	.string "needs a trainer like you.$"

ExoticSafari_Text_CaretakerGot:
	.string "{PLAYER} received TYPE: NULL!$"

ExoticSafari_Text_CaretakerFull:
	.string "Oh dear, you have no room for it.\n"
	.string "Come back when you do.$"

ExoticSafari_Text_CaretakerAfter:
	.string "Bond with TYPE: NULL, and it might\n"
	.string "become something more.$"

ExoticSafari_Text_SealCells:
	.string "Little green specks drift about the\n"
	.string "tower's foot, gathering, scattering…\p"
	.string "ZYGARDE CELLS are hidden about the\n"
	.string "zone. Find {STR_VAR_2} more, and they\l"
	.string "may come together here.$"

ExoticSafari_Text_SealBottle:
	.string "A faint ring of light flickers in the\n"
	.string "air… and fades.\p"
	.string "Something here is bound to the\n"
	.string "bottle on the pedestal nearby.$"

ExoticSafari_Text_SealSteam:
	.string "Steam hisses from cracks in the\n"
	.string "rock: fire and water, together…\p"
	.string "Whatever lives here might come out\n"
	.string "for a FIRE- or WATER-type POKéMON\l"
	.string "in the lead, with the other type\l"
	.string "behind it.$"

ExoticSafari_Text_SealDrain:
	.string "The scrub here is withered and gray,\n"
	.string "as if its life had been drained\l"
	.string "away…\p"
	.string "Something waits for the life of\n"
	.string "KALOS: catch {STR_VAR_2} more KALOS\l"
	.string "POKéMON.$"

ExoticSafari_Text_FoundCell:
	.string "{PLAYER} found a ZYGARDE CELL!\p"
	.string "It wriggled free… and drifted off\n"
	.string "toward LUMIOSE CITY.$"

ExoticSafari_Text_CellsLeft:
	.string "ZYGARDE CELLS found: {STR_VAR_1}\n"
	.string "Still out there: {STR_VAR_2}$"

ExoticSafari_Text_LastCell:
	.string "That was the last of them!\n"
	.string "Every ZYGARDE CELL has been found.\p"
	.string "One last speck stays behind, glowing…\n"
	.string "it hardens into a little cube!$"

ExoticSafari_Text_ZygardeCube:
	.string "The ZYGARDE CUBE changes ZYGARDE's\n"
	.string "FORME. Try it from the bag!$"

ExoticSafari_Text_ZygardePartial:
	.string "ZYGARDE is only partly here: its\n"
	.string "{STR_VAR_1} FORME, still missing {STR_VAR_2} CELLS.\p"
	.string "Find them all and it will be whole.\n"
	.string "Battle it as it is?$"

ExoticSafari_Text_ZygardeWhole:
	.string "Every CELL has come together.\n"
	.string "ZYGARDE stands whole, its full power\l"
	.string "ready to come out!$"

ExoticSafari_Text_ZygardeWait:
	.string "ZYGARDE watches, waiting for the rest\n"
	.string "of itself…$"

ExoticSafari_Text_PrisonBottle:
	.string "An ornate bottle sits on the\n"
	.string "pedestal. It's humming faintly…\p"
	.string "Take the bottle?$"

ExoticSafari_Text_PrisonBottleShakes:
	.string "The PRISON BOTTLE shakes!\n"
	.string "A ring of light opens nearby…$"

ExoticSafari_Text_GroomerIntro:
	.string "Bonjour! I'm LUMIOSE's FURFROU\n"
	.string "groomer.\p"
	.string "A trim is ¥500. Shall I give your\n"
	.string "FURFROU a new look?$"

ExoticSafari_Text_GroomerWhich:
	.string "Which POKéMON shall I groom?$"

ExoticSafari_Text_GroomerStyle:
	.string "Magnifique! Which trim would\n"
	.string "{STR_VAR_1} like?$"

ExoticSafari_Text_GroomerDone:
	.string "Voilà! {STR_VAR_1} looks wonderful!\n"
	.string "Come back for a new trim any time.$"

ExoticSafari_Text_GroomerSame:
	.string "But {STR_VAR_1} already has that\n"
	.string "trim! No charge, of course.$"

ExoticSafari_Text_GroomerNotFurfrou:
	.string "Oh, I only groom FURFROU, I'm\n"
	.string "afraid.$"

ExoticSafari_Text_GroomerNoMoney:
	.string "Oh dear, a trim is ¥500, and you\n"
	.string "don't have enough.$"

ExoticSafari_Text_GroomerBye:
	.string "Au revoir! Bring your FURFROU by\n"
	.string "any time.$"

ExoticSafari_Text_TrimHeart:
	.string "HEART$"
ExoticSafari_Text_TrimStar:
	.string "STAR$"
ExoticSafari_Text_TrimDiamond:
	.string "DIAMOND$"
ExoticSafari_Text_TrimDebutante:
	.string "DEBUTANTE$"
ExoticSafari_Text_TrimMatron:
	.string "MATRON$"
ExoticSafari_Text_TrimDandy:
	.string "DANDY$"
ExoticSafari_Text_TrimLaReine:
	.string "LA REINE$"
ExoticSafari_Text_TrimKabuki:
	.string "KABUKI$"
ExoticSafari_Text_TrimPharaoh:
	.string "PHARAOH$"
ExoticSafari_Text_TrimNatural:
	.string "NATURAL$"

ExoticSafari_Text_SealCaught:
	.string "Something stirs here, then stills…\p"
	.string "It's waiting for\n"
	.string "{STR_VAR_1}\l"
	.string "to be caught first.$"

ExoticSafari_Text_SealLevel:
	.string "The island is quiet, and the lake\n"
	.string "still as glass…\p"
	.string "Whatever lives here might come out\n"
	.string "for a POKéMON of great knowledge:\l"
	.string "level {STR_VAR_2} or higher, in the lead.$"

ExoticSafari_Text_SealBerry:
	.string "Ripples spread across the lake,\n"
	.string "though there's no wind…\p"
	.string "Whatever lives here might come out\n"
	.string "for a POKéMON in the lead that has\l"
	.string "a BERRY to share.$"

ExoticSafari_Text_SealHpEvs:
	.string "The water churns around the island,\n"
	.string "restless…\p"
	.string "Whatever lives here might come out\n"
	.string "for a POKéMON of great willpower:\l"
	.string "{STR_VAR_2} HP EVs, in the lead.$"

ExoticSafari_Text_SealRegis:
	.string "An enormous shape is carved into the\n"
	.string "ice: a giant, asleep…\p"
	.string "It might wake for the ones it made.\n"
	.string "Bring {STR_VAR_1} in\l"
	.string "your party.$"

ExoticSafari_Text_SealAzureFlute:
	.string "The air hums with a power older than\n"
	.string "anything…\p"
	.string "Something here is waiting for a\n"
	.string "melody. The elder of Snowpoint City\l"
	.string "may know more.$"

ExoticSafari_Text_ElderHint:
	.string "Up above the city stands the HALL OF\n"
	.string "ORIGIN, where the first POKéMON is\l"
	.string "said to rest.\p"
	.string "It answers only the AZURE FLUTE, and\n"
	.string "I keep the flute for a trainer who\l"
	.string "has mastered every type there is.\p"
	.string "Master every type in your gym, then\n"
	.string "come back and see me.\p"
	.string "They say the first POKéMON's plates\n"
	.string "come to rest wherever a type is truly\l"
	.string "mastered… Keep an eye on the floor.$"

ExoticSafari_Text_ElderFlute:
	.string "You've mastered every type there is…\n"
	.string "I can see it in your eyes.\p"
	.string "Then this flute is yours.$"

ExoticSafari_Text_ReceivedAzureFlute:
	.string "{PLAYER} received the\n"
	.string "AZURE FLUTE!$"

ExoticSafari_Text_ElderAfter:
	.string "Climb to the HALL OF ORIGIN. The\n"
	.string "first POKéMON will hear the flute.\p"
	.string "They say the first POKéMON's plates\n"
	.string "come to rest wherever a type is truly\l"
	.string "mastered… Keep an eye on the floor.$"

ExoticSafari_Text_ApplianceAsk:
	.string "It's an old {STR_VAR_3}.\n"
	.string "Its motor looks like it would fit a\l"
	.string "ROTOM…\p"
	.string "Let a ROTOM try it?$"

ExoticSafari_Text_RotomRoomGuide:
	.string "My family's old appliances! Their\n"
	.string "motors still hum…\p"
	.string "A ROTOM loves to slip into a motor.\n"
	.string "Let yours try any of them!$"

ExoticSafari_Text_SealKey:
	.string "A strange presence lingers here,\n"
	.string "waiting…\p"
	.string "It might answer to the\n"
	.string "{STR_VAR_1}.$"

ExoticSafari_Text_FoundKey:
	.string "{PLAYER} found the {STR_VAR_1}!$"

ExoticSafari_Text_KeyKept:
	.string "{PLAYER} put the {STR_VAR_1} away\n"
	.string "carefully. Something somewhere\l"
	.string "seems to answer to it…$"

ExoticSafari_Text_ChasmElderGive:
	.string "You caught KYUREM? Then these are\n"
	.string "yours. Long ago, it was one with\l"
	.string "RESHIRAM and ZEKROM…$"

ExoticSafari_Text_ChasmElderAfter:
	.string "With the DNA SPLICERS, KYUREM can\n"
	.string "join with RESHIRAM or ZEKROM again.\p"
	.string "Both must be in your party.$"

ExoticSafari_Text_ChasmElderNotYet:
	.string "A dragon of ice sleeps in this\n"
	.string "chasm. It's waiting for RESHIRAM\l"
	.string "and ZEKROM, the halves of itself.\p"
	.string "Catch it, and come back to me.$"

ExoticSafari_Text_ShrineKeeperGive:
	.string "THUNDURUS and LANDORUS, both\n"
	.string "caught! Then take this.\p"
	.string "The REVEAL GLASS shows their true\n"
	.string "forms.$"

ExoticSafari_Text_ShrineKeeperAfter:
	.string "Use the REVEAL GLASS on THUNDURUS\n"
	.string "or LANDORUS to change their forme.$"

ExoticSafari_Text_ShrineKeeperNotYet:
	.string "Storms roam these lands: THUNDURUS.\n"
	.string "When it is caught, LANDORUS will\l"
	.string "come to bless the fields.\p"
	.string "Catch them both, and I'll show you\n"
	.string "something.$"

ExoticSafari_Text_SealMeteorite:
	.string "The ground here is scorched in a\n"
	.string "strange pattern, as if something\l"
	.string "fell from the sky…\p"
	.string "Whatever it was is still somewhere\n"
	.string "on the mountain.$"

ExoticSafari_Text_SealBlueOrb:
	.string "The sea here is deep and still.\n"
	.string "Something vast sleeps below…\p"
	.string "It might rise for the orb that\n"
	.string "holds its power. Sootopolis may\l"
	.string "keep it.$"

ExoticSafari_Text_SealBlueOrbFound:
	.string "The sea here is deep and still.\n"
	.string "Something vast sleeps below…\p"
	.string "It might rise for your lead POKéMON\n"
	.string "if it held the BLUE ORB.$"

ExoticSafari_Text_SealWish:
	.string "Starlight glints on the grass here.\n"
	.string "Something tiny sleeps, waiting to\l"
	.string "grant a wish…\p"
	.string "It may wake once the other\n"
	.string "{STR_VAR_2} legendary POKéMON of this\l"
	.string "zone are caught.$"

ExoticSafari_Text_Tablet:
	.string "Dots are carved deep into the\n"
	.string "rock. It's BRAILLE. It reads…\p"
	.string "“FIRST COMES WAILORD.\n"
	.string "LAST COMES RELICANTH.\p"
	.string "“THEN THE WALLS OF THE THREE\n"
	.string "CHAMBERS SHALL OPEN.”$"

ExoticSafari_Text_TabletAnswers:
	.string "Your WAILORD and RELICANTH\n"
	.string "seem to be reacting to the dots…$"

ExoticSafari_Text_TabletOpened:
	.string "There was a distant rumbling…\p"
	.string "Somewhere, the walls of three\n"
	.string "sealed chambers have loosened.$"

ExoticSafari_Text_TabletDone:
	.string "“FIRST COMES WAILORD.\n"
	.string "LAST COMES RELICANTH.”\p"
	.string "The tablet's work is done.$"

ExoticSafari_Text_RegiWallSealed:
	.string "Dots are carved into the wall.\n"
	.string "It's BRAILLE. It reads…\p"
	.string "“THIS CHAMBER IS SEALED. THE KEY IS\n"
	.string "THE TABLET BY THE ISLAND CAVE.”$"

ExoticSafari_Text_RegiWallRock:
	.string "Dots are carved into the wall.\n"
	.string "It's BRAILLE. It reads…\p"
	.string "“SHOW THE POWER OF ROCK SMASH\n"
	.string "BEFORE THIS WALL.”$"

ExoticSafari_Text_RegiWallRockSmash:
	.string "{STR_VAR_1} used ROCK SMASH!$"

ExoticSafari_Text_RegiWallSteel:
	.string "Dots are carved into the wall.\n"
	.string "It's BRAILLE. It reads…\p"
	.string "“LIGHT UP THE DARK WITH FLASH\n"
	.string "BEFORE THIS WALL.”$"

ExoticSafari_Text_RegiWallFlash:
	.string "{STR_VAR_1} used FLASH!$"

ExoticSafari_Text_RegiWallIce:
	.string "Dots are carved into the wall.\n"
	.string "It's BRAILLE. It reads…\p"
	.string "“STAY HERE, STILL AND QUIET,\n"
	.string "FOR TWO MINUTES.”$"

ExoticSafari_Text_RegiWallOpen:
	.string "There's a big hole in the wall.$"

ExoticSafari_Text_SealGsBall:
	.string "An old wooden shrine to the forest's\n"
	.string "guardian…\p"
	.string "There's a round hollow in it, the\n"
	.string "shape of a POKé BALL with a gold\l"
	.string "and silver shell. Somebody in the\l"
	.string "RUINS OF ALPH may have seen one.$"

ExoticSafari_Text_BeastSage:
	.string "Three legendary beasts roam Johto,\n"
	.string "fast as lightning, fire and wind.\p"
	.string "They only show themselves to a\n"
	.string "trainer who knows every corner of\l"
	.string "this land.\p"
	.string "You've been to {STR_VAR_1} of its {STR_VAR_2}\n"
	.string "places so far.$"

ExoticSafari_Text_BeastSageDone:
	.string "You've been everywhere in Johto!\p"
	.string "RAIKOU, ENTEI and SUICUNE are\n"
	.string "running free now. Keep your eyes\l"
	.string "open, wherever you go.$"

ExoticSafari_Text_DenElder:
	.string "This shrine guards the way to the\n"
	.string "silver guardian of the sea.\p"
	.string "Only one who carries its SILVER WING\n"
	.string "may pass. They say it was lost in\l"
	.string "the ICE PATH.$"

ExoticSafari_Text_UnownPanelEscape:
	.string "UNOWN letters are carved into the\n"
	.string "stone panel. They spell…\p"
	.string "ESCAPE.$"

ExoticSafari_Text_UnownPanelWater:
	.string "UNOWN letters are carved into the\n"
	.string "stone panel. They spell…\p"
	.string "WATER.$"

ExoticSafari_Text_UnownPanelLight:
	.string "UNOWN letters are carved into the\n"
	.string "stone panel. They spell…\p"
	.string "LIGHT.$"

ExoticSafari_Text_UnownPanelHoOh:
	.string "UNOWN letters are carved into the\n"
	.string "stone panel. They spell…\p"
	.string "HO-OH.$"

ExoticSafari_Text_UnownPanelOpens:
	.string "The panel slid open!\p"
	.string "{STR_VAR_1} of the 4 panels are open.$"

ExoticSafari_Text_UnownPanelsAll:
	.string "The panel slid open!\p"
	.string "The whole ruin rumbled… UNOWN of\n"
	.string "every shape are drifting through the\l"
	.string "RUINS OF ALPH now!$"

ExoticSafari_Text_UnownPanelOpen:
	.string "The stone panel stands open.$"
