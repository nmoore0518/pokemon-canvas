IndigoPlateau_PokemonCenter_1F_Frlg_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, IndigoPlateau_PokemonCenter_1F_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, CableClub_OnResume
	.byte 0

IndigoPlateau_PokemonCenter_1F_OnTransition::
	setrespawn HEAL_LOCATION_INDIGO_PLATEAU
	end

@ Gym Trainer hack: FireRed's guard blocked the door ("Lorelei is absent") from the National Dex
@ until the Sevii Islands were done. This hack has the National Dex from the start and no Sevii
@ Islands, so the door is always open.
@ She also offers a way straight to the Champion once the player has lost to him twice in a row
@ (VAR_CHAMPION_LOSSES, counted in the Champion's room): they've beaten the Elite Four twice.
IndigoPlateau_PokemonCenter_1F_EventScript_DoorGuard::
	lock
	faceplayer
	goto_if_ge VAR_CHAMPION_LOSSES, 2, IndigoPlateau_PokemonCenter_1F_EventScript_ChampionSkipOffer
	msgbox IndigoPlateau_PokemonCenter_1F_Text_FaceEliteFourGoodLuck
	release
	end

IndigoPlateau_PokemonCenter_1F_EventScript_ChampionSkipOffer::
	msgbox IndigoPlateau_PokemonCenter_1F_Text_ChampionSkipOffer, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, IndigoPlateau_PokemonCenter_1F_EventScript_ChampionSkipNo
	special GymHack_CheckTeamSpecial
	goto_if_ne VAR_RESULT, GYM_HACK_TEAM_OK, IndigoPlateau_PokemonCenter_1F_EventScript_ChampionSkipTeam
	msgbox IndigoPlateau_PokemonCenter_1F_Text_ChampionSkipGo
	closemessage
	playse SE_EXIT
	warp MAP_POKEMON_LEAGUE_CHAMPIONS_ROOM, 0
	waitstate
	release
	end

IndigoPlateau_PokemonCenter_1F_EventScript_ChampionSkipNo::
	msgbox IndigoPlateau_PokemonCenter_1F_Text_FaceEliteFourGoodLuck
	release
	end

IndigoPlateau_PokemonCenter_1F_EventScript_ChampionSkipTeam::
	call EventScript_GymHack_ShowTeamRuleMessage
	release
	end

@ Gym Trainer hack: the Elite Four and Champion follow the team rules. They are
@ checked here, at the door, so the player can't get stuck inside the League.
IndigoPlateau_PokemonCenter_1F_EventScript_LeagueTeamCheck::
	lockall
	call GymCandidate_EventScript_LeagueApprovalCheck
	special GymHack_CheckTeamSpecial
	goto_if_eq VAR_RESULT, GYM_HACK_TEAM_OK, IndigoPlateau_PokemonCenter_1F_EventScript_LeagueTeamOk
	textcolor NPC_TEXT_COLOR_NEUTRAL
	call EventScript_GymHack_ShowTeamRuleMessage
	closemessage
	applymovement LOCALID_PLAYER, Common_Movement_WalkDown
	waitmovement 0
	releaseall
	end

IndigoPlateau_PokemonCenter_1F_EventScript_LeagueTeamOk::
	releaseall
	end

IndigoPlateau_PokemonCenter_1F_EventScript_GymGuy::
	msgbox IndigoPlateau_PokemonCenter_1F_Text_GymGuyAdvice, MSGBOX_NPC
	end

IndigoPlateau_PokemonCenter_1F_EventScript_Clerk::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart IndigoPlateau_PokemonCenter_1F_Items
	msgbox gText_PleaseComeAgain
	release
	end

	.align 2
IndigoPlateau_PokemonCenter_1F_Items::
	.2byte ITEM_ULTRA_BALL
	.2byte ITEM_GREAT_BALL
	.2byte ITEM_FULL_RESTORE
	.2byte ITEM_MAX_POTION
	.2byte ITEM_REVIVE
	.2byte ITEM_FULL_HEAL
	.2byte ITEM_MAX_REPEL
	.2byte ITEM_NONE
	release
	end

IndigoPlateau_PokemonCenter_1F_EventScript_Nurse::
	lock
	faceplayer
	call EventScript_PkmnCenterNurse_Frlg
	release
	end

IndigoPlateau_PokemonCenter_1F_Text_GymGuyAdvice::
	.string "Yo!\n"
	.string "Champ in the making!\p"
	.string "At the POKéMON LEAGUE, you will\n"
	.string "face the ELITE FOUR all in a row.\p"
	.string "If you lose, you have to start all\n"
	.string "over again!\p"
	.string "This is it!\n"
	.string "Go for it!$"

IndigoPlateau_PokemonCenter_1F_Text_ChampionSkipOffer::
	.string "You again! You've beaten the ELITE\n"
	.string "FOUR twice and reached the CHAMPION.\p"
	.string "The LEAGUE has seen enough. I can\n"
	.string "take you straight to the CHAMPION.\p"
	.string "Shall I?$"

IndigoPlateau_PokemonCenter_1F_Text_ChampionSkipGo::
	.string "Right this way. Good luck!$"

IndigoPlateau_PokemonCenter_1F_Text_FaceEliteFourGoodLuck::
	.string "From here on, you face the\n"
	.string "ELITE FOUR one by one.\p"
	.string "If you win, a door opens to the\n"
	.string "next TRAINER. Good luck!$"

IndigoPlateau_PokemonCenter_1F_Text_LoreleiIsAbsentClosedForTimeBeing::
	.string "I am so sorry, but you've wasted\n"
	.string "your time coming here.\p"
	.string "LORELEI of the ELITE FOUR is\n"
	.string "absent, you see.\p"
	.string "As a result, the POKéMON LEAGUE is\n"
	.string "closed for the time being.$"

IndigoPlateau_PokemonCenter_1F_Text_AgathaWhuppedUs::
	.string "AGATHA's GHOST-type POKéMON are\n"
	.string "horrifically terrifying in toughness.\p"
	.string "I took my FIGHTING-type POKéMON\n"
	.string "and raised them to the max.\p"
	.string "I went at AGATHA feeling pretty\n"
	.string "confident, but she whupped us.\p"
	.string "That old lady's also got a really\n"
	.string "short fuse, too.\p"
	.string "It doesn't take anything to get\n"
	.string "that scary lady hollering.$"

IndigoPlateau_PokemonCenter_1F_Text_LancesCousinGymLeaderFarAway::
	.string "Maybe becoming an ELITE FOUR\n"
	.string "member is in the blood.\p"
	.string "From what I've heard, LANCE has\n"
	.string "a cousin who's a GYM LEADER\l"
	.string "somewhere far away.$"

