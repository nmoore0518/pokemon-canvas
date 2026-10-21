@ The roaming legendaries (made by tools/gym_hack/build_exotic_safari.py)
ExoticSafari_EventScript_RoamerOkidogi::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_OKIDOGI
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_RoamerGone
	setflag FLAG_EXOTIC_LEGEND_OKIDOGI
	goto ExoticSafari_EventScript_RoamerGone

ExoticSafari_EventScript_RoamerMunkidori::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_MUNKIDORI
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_RoamerGone
	setflag FLAG_EXOTIC_LEGEND_MUNKIDORI
	goto ExoticSafari_EventScript_RoamerGone

ExoticSafari_EventScript_RoamerFezandipiti::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_FEZANDIPITI
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_RoamerGone
	setflag FLAG_EXOTIC_LEGEND_FEZANDIPITI
	goto ExoticSafari_EventScript_RoamerGone

ExoticSafari_EventScript_RoamerSpectrier::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_SPECTRIER
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_RoamerGone
	setflag FLAG_EXOTIC_LEGEND_SPECTRIER
	goto ExoticSafari_EventScript_RoamerGone

ExoticSafari_EventScript_RoamerCalyrex::
	lock
	faceplayer
	setvar VAR_0x8004, SPECIES_CALYREX
	call ExoticSafari_EventScript_LegendBattle
	goto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_RoamerGone
	setflag FLAG_EXOTIC_LEGEND_CALYREX
	goto ExoticSafari_EventScript_RoamerGone

ExoticSafari_EventScript_StakePurple::
	setvar VAR_0x8004, 0
	goto ExoticSafari_EventScript_Stake

ExoticSafari_EventScript_StakeYellow::
	setvar VAR_0x8004, 1
	goto ExoticSafari_EventScript_Stake

ExoticSafari_EventScript_StakeGreen::
	setvar VAR_0x8004, 2
	goto ExoticSafari_EventScript_Stake

ExoticSafari_EventScript_StakeBlue::
	setvar VAR_0x8004, 3
	goto ExoticSafari_EventScript_Stake
