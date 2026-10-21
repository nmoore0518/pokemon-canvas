@ The Exotic Pokémon Safari's areas: what their legendaries share (each area's scripts are made by
@ tools/gym_hack/build_exotic_safari.py; the roamers' placement is src/exotic_safari.c).

@ A legendary (VAR_0x8004: its species) cries, then the battle. VAR_RESULT: the outcome
ExoticSafari_EventScript_LegendBattle::
	copyvar VAR_0x8008, VAR_0x8004
	playmoncry VAR_0x8008, CRY_MODE_ENCOUNTER
	delay 40
	waitmoncry
	setvar VAR_0x8005, EXOTIC_SAFARI_LEGEND_LEVEL
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
	fadescreen FADE_TO_BLACK
	removeobject VAR_LAST_TALKED
	fadescreen FADE_FROM_BLACK
	msgbox ExoticSafari_Text_LegendGone
	release
	end

@ The spot where a legendary appears, while it isn't there (VAR_0x8004: which)
ExoticSafari_EventScript_Seal::
	special ExoticSafari_BufferSeal
	goto_if_eq VAR_RESULT, 0xFF, ExoticSafari_EventScript_ReleaseAll
	goto_if_eq VAR_RESULT, 0xFE, ExoticSafari_EventScript_SealAway
	switch VAR_RESULT
	case SEAL_STAKES, ExoticSafari_EventScript_SealStakes
	case SEAL_MASKS, ExoticSafari_EventScript_SealMasks
	case SEAL_ROAMERS, ExoticSafari_EventScript_SealRoamers
	msgbox ExoticSafari_Text_SealMastery
	releaseall
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
