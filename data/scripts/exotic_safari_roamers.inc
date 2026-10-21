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
