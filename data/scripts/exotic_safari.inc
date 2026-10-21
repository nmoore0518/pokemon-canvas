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

@ A legendary that stays put (until it's caught): after a battle that didn't catch it
ExoticSafari_EventScript_LegendStays::
	msgbox ExoticSafari_Text_LegendStays
	release
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

ExoticSafari_Text_LegendStays:
	.string "It's still watching you…$"

ExoticSafari_Text_RoamerRanOff:
	.string "It ran off to roam somewhere else\n"
	.string "in the zone!$"
