@ Pokémon Canvas: ARCHER sets a POKéMON's free TERA TYPE (src/gym_tera.c): any type the player has
@ mastered, or the gym's.
GymTera_EventScript_ArcherTune::
	msgbox DadStory_Text_ArcherTuneOffer, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, GymTera_EventScript_Later
	special ChoosePartyMon
	goto_if_ge VAR_0x8004, PARTY_SIZE, GymTera_EventScript_Later
	specialvar VAR_RESULT, ScriptGetPartyMonSpecies
	goto_if_eq VAR_RESULT, SPECIES_EGG, GymTera_EventScript_Egg
	special GymTera_BufferMonTera
	message DadStory_Text_ArcherTunePick
	waitmessage
	call GymTera_EventScript_PushTypes
	dynmultistack 0, 0, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, GymTera_EventScript_Later
	copyvar VAR_0x8005, VAR_RESULT
	special GymTera_SetType
	playse SE_SHINY
	msgbox DadStory_Text_ArcherTuned
	release
	end

GymTera_EventScript_Egg::
	msgbox DadStory_Text_ArcherTuneEgg
	release
	end

GymTera_EventScript_Later::
	msgbox DadStory_Text_ArcherTuneLater
	release
	end

GymTera_EventScript_PushTypes::
	setvar VAR_0x8005, TYPE_NORMAL
	special GymTera_IsTypeAllowed
	goto_if_eq VAR_RESULT, FALSE, GymTera_EventScript_Skip_NORMAL
	dynmultipush GymTera_Text_NORMAL, TYPE_NORMAL
GymTera_EventScript_Skip_NORMAL:
	setvar VAR_0x8005, TYPE_FIGHTING
	special GymTera_IsTypeAllowed
	goto_if_eq VAR_RESULT, FALSE, GymTera_EventScript_Skip_FIGHTING
	dynmultipush GymTera_Text_FIGHTING, TYPE_FIGHTING
GymTera_EventScript_Skip_FIGHTING:
	setvar VAR_0x8005, TYPE_FLYING
	special GymTera_IsTypeAllowed
	goto_if_eq VAR_RESULT, FALSE, GymTera_EventScript_Skip_FLYING
	dynmultipush GymTera_Text_FLYING, TYPE_FLYING
GymTera_EventScript_Skip_FLYING:
	setvar VAR_0x8005, TYPE_POISON
	special GymTera_IsTypeAllowed
	goto_if_eq VAR_RESULT, FALSE, GymTera_EventScript_Skip_POISON
	dynmultipush GymTera_Text_POISON, TYPE_POISON
GymTera_EventScript_Skip_POISON:
	setvar VAR_0x8005, TYPE_GROUND
	special GymTera_IsTypeAllowed
	goto_if_eq VAR_RESULT, FALSE, GymTera_EventScript_Skip_GROUND
	dynmultipush GymTera_Text_GROUND, TYPE_GROUND
GymTera_EventScript_Skip_GROUND:
	setvar VAR_0x8005, TYPE_ROCK
	special GymTera_IsTypeAllowed
	goto_if_eq VAR_RESULT, FALSE, GymTera_EventScript_Skip_ROCK
	dynmultipush GymTera_Text_ROCK, TYPE_ROCK
GymTera_EventScript_Skip_ROCK:
	setvar VAR_0x8005, TYPE_BUG
	special GymTera_IsTypeAllowed
	goto_if_eq VAR_RESULT, FALSE, GymTera_EventScript_Skip_BUG
	dynmultipush GymTera_Text_BUG, TYPE_BUG
GymTera_EventScript_Skip_BUG:
	setvar VAR_0x8005, TYPE_GHOST
	special GymTera_IsTypeAllowed
	goto_if_eq VAR_RESULT, FALSE, GymTera_EventScript_Skip_GHOST
	dynmultipush GymTera_Text_GHOST, TYPE_GHOST
GymTera_EventScript_Skip_GHOST:
	setvar VAR_0x8005, TYPE_STEEL
	special GymTera_IsTypeAllowed
	goto_if_eq VAR_RESULT, FALSE, GymTera_EventScript_Skip_STEEL
	dynmultipush GymTera_Text_STEEL, TYPE_STEEL
GymTera_EventScript_Skip_STEEL:
	setvar VAR_0x8005, TYPE_FIRE
	special GymTera_IsTypeAllowed
	goto_if_eq VAR_RESULT, FALSE, GymTera_EventScript_Skip_FIRE
	dynmultipush GymTera_Text_FIRE, TYPE_FIRE
GymTera_EventScript_Skip_FIRE:
	setvar VAR_0x8005, TYPE_WATER
	special GymTera_IsTypeAllowed
	goto_if_eq VAR_RESULT, FALSE, GymTera_EventScript_Skip_WATER
	dynmultipush GymTera_Text_WATER, TYPE_WATER
GymTera_EventScript_Skip_WATER:
	setvar VAR_0x8005, TYPE_GRASS
	special GymTera_IsTypeAllowed
	goto_if_eq VAR_RESULT, FALSE, GymTera_EventScript_Skip_GRASS
	dynmultipush GymTera_Text_GRASS, TYPE_GRASS
GymTera_EventScript_Skip_GRASS:
	setvar VAR_0x8005, TYPE_ELECTRIC
	special GymTera_IsTypeAllowed
	goto_if_eq VAR_RESULT, FALSE, GymTera_EventScript_Skip_ELECTRIC
	dynmultipush GymTera_Text_ELECTRIC, TYPE_ELECTRIC
GymTera_EventScript_Skip_ELECTRIC:
	setvar VAR_0x8005, TYPE_PSYCHIC
	special GymTera_IsTypeAllowed
	goto_if_eq VAR_RESULT, FALSE, GymTera_EventScript_Skip_PSYCHIC
	dynmultipush GymTera_Text_PSYCHIC, TYPE_PSYCHIC
GymTera_EventScript_Skip_PSYCHIC:
	setvar VAR_0x8005, TYPE_ICE
	special GymTera_IsTypeAllowed
	goto_if_eq VAR_RESULT, FALSE, GymTera_EventScript_Skip_ICE
	dynmultipush GymTera_Text_ICE, TYPE_ICE
GymTera_EventScript_Skip_ICE:
	setvar VAR_0x8005, TYPE_DRAGON
	special GymTera_IsTypeAllowed
	goto_if_eq VAR_RESULT, FALSE, GymTera_EventScript_Skip_DRAGON
	dynmultipush GymTera_Text_DRAGON, TYPE_DRAGON
GymTera_EventScript_Skip_DRAGON:
	setvar VAR_0x8005, TYPE_DARK
	special GymTera_IsTypeAllowed
	goto_if_eq VAR_RESULT, FALSE, GymTera_EventScript_Skip_DARK
	dynmultipush GymTera_Text_DARK, TYPE_DARK
GymTera_EventScript_Skip_DARK:
	setvar VAR_0x8005, TYPE_FAIRY
	special GymTera_IsTypeAllowed
	goto_if_eq VAR_RESULT, FALSE, GymTera_EventScript_Skip_FAIRY
	dynmultipush GymTera_Text_FAIRY, TYPE_FAIRY
GymTera_EventScript_Skip_FAIRY:
	return

GymTera_Text_NORMAL:
	.string "NORMAL$"

GymTera_Text_FIGHTING:
	.string "FIGHTING$"

GymTera_Text_FLYING:
	.string "FLYING$"

GymTera_Text_POISON:
	.string "POISON$"

GymTera_Text_GROUND:
	.string "GROUND$"

GymTera_Text_ROCK:
	.string "ROCK$"

GymTera_Text_BUG:
	.string "BUG$"

GymTera_Text_GHOST:
	.string "GHOST$"

GymTera_Text_STEEL:
	.string "STEEL$"

GymTera_Text_FIRE:
	.string "FIRE$"

GymTera_Text_WATER:
	.string "WATER$"

GymTera_Text_GRASS:
	.string "GRASS$"

GymTera_Text_ELECTRIC:
	.string "ELECTRIC$"

GymTera_Text_PSYCHIC:
	.string "PSYCHIC$"

GymTera_Text_ICE:
	.string "ICE$"

GymTera_Text_DRAGON:
	.string "DRAGON$"

GymTera_Text_DARK:
	.string "DARK$"

GymTera_Text_FAIRY:
	.string "FAIRY$"
