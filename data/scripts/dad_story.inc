@ Pokémon Canvas: the Dad story (VAR_DAD_STORY, include/constants/dad_story.h; src/dad_story.c).
@ The words are in data/text/dad_story.inc (generated from tools/gym_hack/story/dad_story.txt).
@
@  1. OAK's lab, the Pokédex: MOM waits outside to send the player off (Pallet Town)
@  2. NUGGET BRIDGE: the player has to join TEAM ROCKET; the rival comes back to say MOM has something,
@     and the player is home: Dad's Type Ball (the player's GYM type)
@  3. SILPH CO. 11F: GIOVANNI alone, the MASTER BALL; MOM runs in and takes the player home, moves the
@     bookcase and shows them Dad's basement
@  4. VIRIDIAN GYM: after the badge, GIOVANNI's last battle (his team synthesized, in Type Balls); MOM's
@     CHANSEY ends him; DAD's CHANSEY and DAD's NOTEBOOK; the basement's switch appears

@ ---- 1. The send-off ----------------------------------------------------------------------------
@ (OAK's lab, after the mentor's visit)
DadStory_EventScript_ArmSendoff::
	goto_if_ne VAR_DAD_STORY, DAD_STORY_NONE, Common_EventScript_NopReturn
	setvar VAR_DAD_STORY, DAD_STORY_SENDOFF
	return

@ (Pallet Town's ON_TRANSITION) MOM is outside the lab only while she's waiting
DadStory_EventScript_PalletTransition::
	setflag FLAG_HIDE_SENDOFF_MOM
	goto_if_eq VAR_DAD_STORY, DAD_STORY_DONE, DadStory_EventScript_MemorialSetup
	goto_if_ne VAR_DAD_STORY, DAD_STORY_SENDOFF, Common_EventScript_NopReturn
	clearflag FLAG_HIDE_SENDOFF_MOM
	return

@ After VIRIDIAN: MOM and OAK wait at the CHANSEY statue, either side of where the player stands
DadStory_EventScript_MemorialSetup::
	clearflag FLAG_HIDE_SENDOFF_MOM
	setobjectxyperm LOCALID_PALLET_SENDOFF_MOM, DAD_STORY_MEMORIAL_X - 1, DAD_STORY_MEMORIAL_Y
	setobjectmovementtype LOCALID_PALLET_SENDOFF_MOM, MOVEMENT_TYPE_FACE_RIGHT
	clearflag FLAG_HIDE_OAK_IN_PALLET_TOWN
	setobjectxyperm LOCALID_PALLET_PROF_OAK, DAD_STORY_MEMORIAL_X + 1, DAD_STORY_MEMORIAL_Y
	setobjectmovementtype LOCALID_PALLET_PROF_OAK, MOVEMENT_TYPE_FACE_LEFT
	return

@ MOM and OAK remember the player's father; OAK explains the statue and the LEAGUE's approval
DadStory_EventScript_Memorial::
	lockall
	getplayerxy VAR_0x8004, VAR_0x8005
	goto_if_ne VAR_0x8004, DAD_STORY_MEMORIAL_X, DadStory_EventScript_MemorialGoThere
	goto_if_ne VAR_0x8005, DAD_STORY_MEMORIAL_Y, DadStory_EventScript_MemorialGoThere
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	delay 30
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterRight
	waitmovement 0
	textcolor NPC_TEXT_COLOR_MALE
	msgbox DadStory_Text_MemorialOak1
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	textcolor NPC_TEXT_COLOR_FEMALE
	msgbox DadStory_Text_MemorialMom1
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterRight
	waitmovement 0
	textcolor NPC_TEXT_COLOR_MALE
	msgbox DadStory_Text_MemorialOak2
	textcolor NPC_TEXT_COLOR_FEMALE
	msgbox DadStory_Text_MemorialMom2
	textcolor NPC_TEXT_COLOR_MALE
	msgbox DadStory_Text_MemorialOak3
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	textcolor NPC_TEXT_COLOR_FEMALE
	msgbox DadStory_Text_MemorialMom3
	closemessage
	setflag FLAG_GYM_APPROVAL_GIOVANNI
	setvar VAR_DAD_STORY, DAD_STORY_REMEMBERED
	fadescreen FADE_TO_BLACK
	removeobject LOCALID_PALLET_SENDOFF_MOM
	removeobject LOCALID_PALLET_PROF_OAK
	setflag FLAG_HIDE_SENDOFF_MOM
	setflag FLAG_HIDE_OAK_IN_PALLET_TOWN
	@ (their usual places, for the scenes that don't set them)
	setobjectxyperm LOCALID_PALLET_SENDOFF_MOM, PALLET_MOM_SENDOFF_X, PALLET_MOM_SENDOFF_Y
	setobjectmovementtype LOCALID_PALLET_SENDOFF_MOM, MOVEMENT_TYPE_FACE_UP
	setobjectxyperm LOCALID_PALLET_PROF_OAK, PALLET_OAK_START_X, PALLET_OAK_START_Y
	setobjectmovementtype LOCALID_PALLET_PROF_OAK, MOVEMENT_TYPE_FACE_UP
	fadescreen FADE_FROM_BLACK
	releaseall
	end

@ (an older save coming into PALLET TOWN some other way: to the statue first)
DadStory_EventScript_MemorialGoThere::
	fadescreen FADE_TO_BLACK
	warp MAP_PALLET_TOWN, DAD_STORY_MEMORIAL_X, DAD_STORY_MEMORIAL_Y
	waitstate
	releaseall
	end

@ MOM at home after VIRIDIAN (then her usual rest)
DadStory_EventScript_MomAtHome::
	msgbox DadStory_Text_MomAfterViridian
	return

DadStory_EventScript_Sendoff::
	lockall
	textcolor NPC_TEXT_COLOR_FEMALE
	applymovement LOCALID_PALLET_SENDOFF_MOM, PalletTown_Movement_MomSendoffApproach
	waitmovement 0
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	msgbox DadStory_Text_MomSendoff
	closemessage
	applymovement LOCALID_PALLET_SENDOFF_MOM, PalletTown_Movement_MomSendoffGoHome
	waitmovement 0
	removeobject LOCALID_PALLET_SENDOFF_MOM
	setflag FLAG_HIDE_SENDOFF_MOM
	setvar VAR_DAD_STORY, DAD_STORY_ON_THE_ROAD
	releaseall
	end

@ ---- 2. NUGGET BRIDGE and Dad's ball -----------------------------------------------------------
@ The player can't say no: he asks until they say yes (and gives the MOVE EXTRACTOR)
DadStory_EventScript_MustJoin::
	msgbox MoveExtractor_Text_WantToJoin, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, DadStory_EventScript_Joined
	msgbox DadStory_Text_GruntInsists
	goto DadStory_EventScript_MustJoin

DadStory_EventScript_Joined::
	call MoveExtractor_EventScript_GruntSurprised
	return

@ Then the rival comes back up the bridge, and the player goes home (ends the script)
DadStory_EventScript_RivalSendsHome::
	fadedefaultbgm
	setvar VAR_MAP_SCENE_ROUTE24, 1
	goto_if_ge VAR_DAD_STORY, DAD_STORY_MOM_CALLING, DadStory_EventScript_ReleaseAll
	textcolor NPC_TEXT_COLOR_MALE
	getplayerxy VAR_0x8004, VAR_0x8005
	clearflag FLAG_HIDE_ROUTE24_RIVAL
	addobject LOCALID_ROUTE24_RIVAL
	setobjectxy LOCALID_ROUTE24_RIVAL, VAR_0x8004, 20
	playbgm MUS_RG_ENCOUNTER_RIVAL, 0
	applymovement LOCALID_ROUTE24_RIVAL, DadStory_Movement_RivalUpTheBridge
	waitmovement 0
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	msgbox DadStory_Text_RivalMomHasSomething
	closemessage
	playbgm MUS_RG_RIVAL_EXIT, 0
	applymovement LOCALID_ROUTE24_RIVAL, DadStory_Movement_RivalDownTheBridge
	waitmovement 0
	removeobject LOCALID_ROUTE24_RIVAL
	setflag FLAG_HIDE_ROUTE24_RIVAL
	fadedefaultbgm
	setvar VAR_DAD_STORY, DAD_STORY_MOM_CALLING
	warp MAP_PALLET_TOWN_PLAYERS_HOUSE_1F, 9, 5
	waitstate
	releaseall
	end

DadStory_EventScript_ReleaseAll::
	releaseall
	end

DadStory_Movement_RivalUpTheBridge:
	walk_up
	walk_up
	walk_up
	walk_up
	step_end

DadStory_Movement_RivalDownTheBridge:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	step_end

@ (the house's 1F) the player is home, beside MOM's chair
DadStory_EventScript_MomDadsBall::
	lockall
	textcolor NPC_TEXT_COLOR_FEMALE
	applymovement LOCALID_PLAYERS_HOUSE_1F_MOM, Common_Movement_WalkInPlaceFasterRight
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	msgbox DadStory_Text_MomDadsBall
	special DadStory_GetGymTypeBall
	giveitem VAR_RESULT
	msgbox DadStory_Text_MomMoveExtractor
	closemessage
	setvar VAR_DAD_STORY, DAD_STORY_GOT_DADS_BALL
	applymovement LOCALID_PLAYERS_HOUSE_1F_MOM, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	releaseall
	end

@ ---- 3. SILPH CO. and the basement --------------------------------------------------------------
@ (Silph Co. 11F, after GIOVANNI's post-battle lines) VAR_TEMP_1: 0 = the player stands at x 5, 1 = x 6
DadStory_EventScript_SilphGiovanniGives::
	msgbox DadStory_Text_GiovanniSilphGift
	giveitem ITEM_MASTER_BALL
	setflag FLAG_GOT_MASTER_BALL_FROM_SILPH
	call GymHack_EventScript_SilphGiovanniGift
	closemessage
	goto_if_ge VAR_DAD_STORY, DAD_STORY_SILPH_HOME, Common_EventScript_NopReturn
	@ MOM runs in, sees the player with GIOVANNI
	textcolor NPC_TEXT_COLOR_FEMALE
	clearflag FLAG_HIDE_SILPH_MOM
	addobject LOCALID_SILPH_CO_11F_MOM
	call_if_eq VAR_TEMP_1, 0, DadStory_EventScript_SilphMomLeft
	applymovement LOCALID_SILPH_CO_11F_MOM, DadStory_Movement_MomRunIn
	waitmovement 0
	playse SE_PIN
	applymovement LOCALID_SILPH_CO_11F_MOM, Common_Movement_ExclamationMark
	waitmovement 0
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	checkplayergender
	call_if_eq VAR_RESULT, MALE, DadStory_EventScript_MomSilphBoy
	call_if_eq VAR_RESULT, FEMALE, DadStory_EventScript_MomSilphGirl
	closemessage
	setvar VAR_DAD_STORY, DAD_STORY_SILPH_HOME
	return

DadStory_EventScript_SilphMomLeft::
	setobjectxy LOCALID_SILPH_CO_11F_MOM, 5, 18
	return

DadStory_EventScript_MomSilphBoy::
	msgbox DadStory_Text_MomSilphBoy
	return

DadStory_EventScript_MomSilphGirl::
	msgbox DadStory_Text_MomSilphGirl
	return

DadStory_Movement_MomRunIn:
	walk_fast_up
	walk_fast_up
	step_end

@ (Silph Co. 11F's ON_TRANSITION)
DadStory_EventScript_SilphTransition::
	setflag FLAG_HIDE_SILPH_MOM
	return

@ (Silph Co. 11F, at the end of the GIOVANNI scene, the screen black) home with MOM
DadStory_EventScript_SilphWarpHome::
	warp MAP_PALLET_TOWN_PLAYERS_HOUSE_1F, 9, 5
	waitstate
	releaseall
	end

@ (the house's 1F, ON_LOAD) Until MOM shows it, a bookcase stands over the stairwell down (the stairs'
@ warp square, beside it, gets the plain mat). Saves from before the story already past SILPH CO. just
@ get the stairs.
DadStory_EventScript_HouseOnLoad::
	goto_if_set FLAG_BASEMENT_REVEALED, DadStory_EventScript_End
	goto_if_eq VAR_DAD_STORY, DAD_STORY_SILPH_HOME, DadStory_EventScript_HideStairs
	goto_if_defeated TRAINER_BOSS_GIOVANNI_2, DadStory_EventScript_OldSaveStairs
DadStory_EventScript_HideStairs::
	setmetatile 7, 0, 0x02E, TRUE
	setmetatile 8, 0, 0x02F, TRUE
	setmetatile 7, 1, 0x036, TRUE
	setmetatile 8, 1, 0x037, TRUE
	setmetatile 7, 2, 0x03E, TRUE
	setmetatile 8, 2, 0x03F, TRUE
	setmetatile 9, 2, 0x01D, FALSE
	setmetatile 7, 3, 0x001, FALSE
	setmetatile 8, 3, 0x001, FALSE
	setmetatile 9, 3, 0x025, FALSE
	end

DadStory_EventScript_OldSaveStairs::
	setflag FLAG_BASEMENT_REVEALED
DadStory_EventScript_End::
	end

DadStory_EventScript_RevealStairs::
	setmetatile 7, 0, 0x021, TRUE
	setmetatile 8, 0, 0x022, TRUE
	setmetatile 7, 1, 0x029, TRUE
	setmetatile 8, 1, 0x02A, TRUE
	setmetatile 7, 2, 0x005, TRUE
	setmetatile 8, 2, 0x006, TRUE
	setmetatile 9, 2, 0x007, FALSE
	setmetatile 7, 3, 0x00D, TRUE
	setmetatile 8, 3, 0x00E, TRUE
	setmetatile 9, 3, 0x00F, FALSE
	special DrawWholeMapView
	return

@ (the house's 1F) home from SILPH CO.: MOM, then the bookcase
DadStory_EventScript_MomWhyWithHim::
	lockall
	textcolor NPC_TEXT_COLOR_FEMALE
	applymovement LOCALID_PLAYERS_HOUSE_1F_MOM, Common_Movement_WalkInPlaceFasterRight
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	msgbox DadStory_Text_MomWhyWithHim
	closemessage
	applymovement LOCALID_PLAYERS_HOUSE_1F_MOM, DadStory_Movement_MomToBookcase
	applymovement LOCALID_PLAYER, DadStory_Movement_PlayerToBookcase
	waitmovement 0
	delay 20
	setvar VAR_0x8004, 1   @ vertical pan
	setvar VAR_0x8005, 0   @ horizontal pan
	setvar VAR_0x8006, 6   @ num shakes
	setvar VAR_0x8007, 3   @ shake delay
	playse SE_M_STRENGTH
	special ShakeCamera
	waitstate
	call DadStory_EventScript_RevealStairs
	setflag FLAG_BASEMENT_REVEALED
	delay 40
	applymovement LOCALID_PLAYERS_HOUSE_1F_MOM, Common_Movement_WalkInPlaceFasterRight
	waitmovement 0
	delay 20
	fadescreen FADE_TO_BLACK
	warp MAP_PALLET_TOWN_PLAYERS_HOUSE_B1F, 12, 3
	waitstate
	releaseall
	end

DadStory_Movement_MomToBookcase:
	walk_up
	walk_up
	walk_in_place_faster_up
	step_end

DadStory_Movement_PlayerToBookcase:
	delay_16
	walk_up
	walk_up
	walk_in_place_faster_left
	step_end

@ (the basement's ON_TRANSITION)
DadStory_EventScript_BasementTransition::
	setflag FLAG_HIDE_BASEMENT_MOM
	goto_if_ne VAR_DAD_STORY, DAD_STORY_SILPH_HOME, Common_EventScript_NopReturn
	clearflag FLAG_HIDE_BASEMENT_MOM
	return

@ (the basement) MOM's story, then she goes back up
DadStory_EventScript_Basement::
	lockall
	textcolor NPC_TEXT_COLOR_FEMALE
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	msgbox DadStory_Text_MomBasement
	closemessage
	applymovement LOCALID_BASEMENT_MOM, DadStory_Movement_MomUpstairs
	waitmovement 0
	removeobject LOCALID_BASEMENT_MOM
	setflag FLAG_HIDE_BASEMENT_MOM
	setvar VAR_DAD_STORY, DAD_STORY_BASEMENT
	releaseall
	end

DadStory_Movement_MomUpstairs:
	walk_up
	walk_right
	set_invisible
	step_end

@ Dad's notes and notebook, as pages
DadStory_EventScript_ReadNotebook::
	lockall
	setvar VAR_0x8004, NOTE_PAGE_NOTEBOOK
	special ShowNotePage
	releaseall
	end

@ ---- 4. VIRIDIAN GYM ---------------------------------------------------------------------------
@ (after the first battle, its flags set) the badge and TM in a hurry, then his story and his last battle
DadStory_EventScript_GiovanniTakeThese::
	textcolor NPC_TEXT_COLOR_MALE
	msgbox DadStory_Text_GiovanniTakeThese
	playfanfare MUS_OBTAIN_BADGE
	message DadStory_Text_ReceivedEarthBadge
	waitmessage
	waitfanfare
	giveitem ITEM_TM26
	setflag FLAG_GOT_TM26_FROM_GIOVANNI
	goto_if_set FLAG_GOT_SILVER_CAP_FROM_GIOVANNI, DadStory_EventScript_GiovanniStory
	setvar VAR_0x8009, ITEM_BOTTLE_CAP
	call GymHack_EventScript_GiveStatItem
	setflag FLAG_GOT_SILVER_CAP_FROM_GIOVANNI
DadStory_EventScript_GiovanniStory::
	msgbox DadStory_Text_GiovanniYourFather
	setvar VAR_DAD_STORY, DAD_STORY_GIOVANNI
	goto DadStory_EventScript_GiovanniLastBattle

@ (talking to him again after losing his last battle)
DadStory_EventScript_GiovanniRematch::
	lock
	faceplayer
	textcolor NPC_TEXT_COLOR_MALE
	msgbox DadStory_Text_GiovanniRematch
DadStory_EventScript_GiovanniLastBattle::
	closemessage
	setflag FLAG_GYM_CHALLENGE_BATTLE   @ (B_FLAG_NO_WHITEOUT: losing comes back here)
	cleartrainerflag TRAINER_GIOVANNI_DAD_FINAL
	setvar VAR_LAST_TALKED, LOCALID_VIRIDIAN_GIOVANNI
	trainerbattle_no_intro TRAINER_GIOVANNI_DAD_FINAL, DadStory_Text_GiovanniFinalDefeat
	clearflag FLAG_GYM_CHALLENGE_BATTLE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, DadStory_EventScript_GiovanniFinale
	special HealPlayerParty
	msgbox DadStory_Text_GiovanniYouLost
	release
	end

DadStory_EventScript_GiovanniFinale::
	special DadStory_BufferAce
	msgbox DadStory_Text_GiovanniHow
	closemessage
	@ the player steps below GIOVANNI; MOM walks up beside them
	getplayerxy VAR_0x8004, VAR_0x8005
	call_if_eq VAR_0x8004, 1, DadStory_EventScript_PlayerFromLeft
	call_if_eq VAR_0x8004, 3, DadStory_EventScript_PlayerFromRight
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	delay 30
	textcolor NPC_TEXT_COLOR_FEMALE
	clearflag FLAG_HIDE_VIRIDIAN_GYM_MOM
	addobject LOCALID_VIRIDIAN_GYM_MOM
	applymovement LOCALID_VIRIDIAN_GYM_MOM, DadStory_Movement_MomUpToGiovanni
	waitmovement 0
	msgbox DadStory_Text_MomLastTime
	closemessage
	@ her POKé BALL: CHANSEY
	playse SE_BALL_THROW
	delay 20
	playse SE_BALL_OPEN
	clearflag FLAG_HIDE_VIRIDIAN_GYM_CHANSEY
	addobject LOCALID_VIRIDIAN_GYM_CHANSEY
	playmoncry SPECIES_CHANSEY, CRY_MODE_NORMAL
	waitmoncry
	delay 20
	setvar VAR_0x8004, 2   @ vertical pan
	setvar VAR_0x8005, 2   @ horizontal pan
	setvar VAR_0x8006, 16  @ num shakes
	setvar VAR_0x8007, 3   @ shake delay
	playse SE_M_EARTHQUAKE
	special ShakeCamera
	waitstate
	applymovement LOCALID_VIRIDIAN_GYM_CHANSEY, DadStory_Movement_ChanseyAttacks
	waitmovement 0
	playse SE_M_HYPER_BEAM
	fadescreen FADE_TO_WHITE
	removeobject LOCALID_VIRIDIAN_GIOVANNI
	setflag FLAG_HIDE_VIRIDIAN_GIOVANNI
	delay 60
	fadescreen FADE_FROM_WHITE
	delay 90
	playse SE_BALL_OPEN
	removeobject LOCALID_VIRIDIAN_GYM_CHANSEY
	setflag FLAG_HIDE_VIRIDIAN_GYM_CHANSEY
	delay 40
	applymovement LOCALID_VIRIDIAN_GYM_MOM, Common_Movement_WalkInPlaceFasterLeft
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterRight
	waitmovement 0
	msgbox DadStory_Text_MomDadsChansey
	special DadStory_GiveChansey
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	message DadStory_Text_ReceivedDadsChansey
	waitmessage
	waitfanfare
	call_if_eq VAR_RESULT, MON_GIVEN_TO_PC, DadStory_EventScript_ChanseyToPC
	msgbox DadStory_Text_ReceivedDadsNotebook
	setflag FLAG_GOT_DADS_CHANSEY
	setflag FLAG_BASEMENT_SWITCH_SHOWN
	setflag FLAG_GYM_APPROVAL_GIOVANNI
	setvar VAR_DAD_STORY, DAD_STORY_DONE
	@ what MOM knew; then home, where OAK is waiting at the CHANSEY statue
	msgbox DadStory_Text_MomConfession
	closemessage
	setflag FLAG_HIDE_VIRIDIAN_GYM_MOM
	fadescreen FADE_TO_BLACK
	warp MAP_PALLET_TOWN, DAD_STORY_MEMORIAL_X, DAD_STORY_MEMORIAL_Y
	waitstate
	release
	end

DadStory_EventScript_ChanseyToPC::
	msgbox DadStory_Text_ChanseySentToPC
	return

DadStory_EventScript_PlayerFromLeft::
	applymovement LOCALID_PLAYER, DadStory_Movement_PlayerFromLeft
	waitmovement 0
	return

DadStory_EventScript_PlayerFromRight::
	applymovement LOCALID_PLAYER, DadStory_Movement_PlayerFromRight
	waitmovement 0
	return

DadStory_Movement_PlayerFromLeft:
	walk_down
	walk_right
	step_end

DadStory_Movement_PlayerFromRight:
	walk_down
	walk_left
	step_end

DadStory_Movement_MomUpToGiovanni:
	walk_up
	walk_up
	walk_up
	walk_up
	walk_in_place_faster_up
	step_end

DadStory_Movement_ChanseyAttacks:
	walk_in_place_faster_left
	jump_in_place_left
	step_end

DadStory_Movement_MomLeaves:
	walk_down
	walk_down
	walk_down
	walk_down
	step_end

@ (the VIRIDIAN GYM's ON_TRANSITION)
DadStory_EventScript_ViridianGymTransition::
	setflag FLAG_HIDE_VIRIDIAN_GYM_MOM
	setflag FLAG_HIDE_VIRIDIAN_GYM_CHANSEY
	setflag FLAG_HIDE_VIRIDIAN_ARCHER
	goto_if_lt VAR_DAD_STORY, DAD_STORY_REMEMBERED, Common_EventScript_NopReturn
	clearflag FLAG_HIDE_VIRIDIAN_ARCHER
	setflag FLAG_HIDE_VIRIDIAN_GIOVANNI   @ (his spot is ARCHER's now)
	return

@ ARCHER, VIRIDIAN's LEADER after GIOVANNI: where he came from (once), then a word
DadStory_EventScript_Archer::
	lock
	faceplayer
	goto_if_ge VAR_DAD_STORY, DAD_STORY_MET_ARCHER, DadStory_EventScript_ArcherAgain
	msgbox DadStory_Text_ArcherIntro
	setvar VAR_DAD_STORY, DAD_STORY_MET_ARCHER
	release
	end

DadStory_EventScript_ArcherAgain::
	msgbox DadStory_Text_ArcherAgain
	release
	end
