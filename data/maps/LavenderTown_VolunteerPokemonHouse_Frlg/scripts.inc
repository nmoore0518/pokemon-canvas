LavenderTown_VolunteerPokemonHouse_Frlg_MapScripts::
	.byte 0

LavenderTown_VolunteerPokemonHouse_EventScript_MrFuji::
	lock
	faceplayer
	goto_if_set FLAG_GOT_POKE_FLUTE, LavenderTown_VolunteerPokemonHouse_EventScript_FujiWardOrFlute
	msgbox LavenderTown_VolunteerPokemonHouse_Text_IdLikeYouToHaveThis
	checkitemspace ITEM_POKE_FLUTE
	goto_if_eq VAR_RESULT, FALSE, LavenderTown_VolunteerPokemonHouse_EventScript_NoRoomForPokeFlute
	setflag FLAG_GOT_POKE_FLUTE
	giveitem_msg LavenderTown_VolunteerPokemonHouse_Text_ReceivedPokeFluteFromMrFuji, ITEM_POKE_FLUTE, 1, MUS_RG_OBTAIN_KEY_ITEM
	msgbox LavenderTown_VolunteerPokemonHouse_Text_ExplainPokeFlute
	msgbox LavenderTown_VolunteerPokemonHouse_Text_FujiSomethingElse
	goto LavenderTown_VolunteerPokemonHouse_EventScript_FujiAskBattle

@ Pokémon Canvas: MR. FUJI's SPIRIT WARD, for a trainer who beats him (src/gym_challenge.c: GymChallenge_SetupFuji)
LavenderTown_VolunteerPokemonHouse_EventScript_FujiWardOrFlute::
	checkitem ITEM_SPIRIT_WARD
	goto_if_eq VAR_RESULT, TRUE, LavenderTown_VolunteerPokemonHouse_EventScript_AlreadyHavePokeFlute
	msgbox LavenderTown_VolunteerPokemonHouse_Text_FujiStillWaiting
LavenderTown_VolunteerPokemonHouse_EventScript_FujiAskBattle::
	msgbox LavenderTown_VolunteerPokemonHouse_Text_FujiAskBattle, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, LavenderTown_VolunteerPokemonHouse_EventScript_FujiLater
	msgbox LavenderTown_VolunteerPokemonHouse_Text_FujiBattleIntro
	closemessage
	special GymChallenge_SetupFuji
	cleartrainerflag TRAINER_GYM_CHALLENGER
	setflag FLAG_GYM_CHALLENGE_BATTLE
	trainerbattle_no_intro TRAINER_GYM_CHALLENGER, gGymChallengeDefeatText
	clearflag FLAG_GYM_CHALLENGE_BATTLE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_ne VAR_RESULT, B_OUTCOME_WON, LavenderTown_VolunteerPokemonHouse_EventScript_FujiLost
	msgbox LavenderTown_VolunteerPokemonHouse_Text_FujiWon
	giveitem ITEM_SPIRIT_WARD
	msgbox LavenderTown_VolunteerPokemonHouse_Text_ExplainSpiritWard
	release
	end

LavenderTown_VolunteerPokemonHouse_EventScript_FujiLost::
	special HealPlayerParty
	msgbox LavenderTown_VolunteerPokemonHouse_Text_FujiLost
	release
	end

LavenderTown_VolunteerPokemonHouse_EventScript_FujiLater::
	msgbox LavenderTown_VolunteerPokemonHouse_Text_FujiLater
	release
	end

LavenderTown_VolunteerPokemonHouse_EventScript_AlreadyHavePokeFlute::
	msgbox LavenderTown_VolunteerPokemonHouse_Text_HasPokeFluteHelpedYou
	release
	end

LavenderTown_VolunteerPokemonHouse_EventScript_NoRoomForPokeFlute::
	msgbox LavenderTown_VolunteerPokemonHouse_Text_MustMakeRoomForThis
	release
	end

LavenderTown_VolunteerPokemonHouse_EventScript_LittleGirl::
	lock
	faceplayer
	famechecker FAMECHECKER_MRFUJI, 0
	msgbox LavenderTown_VolunteerPokemonHouse_Text_MrFujiLooksAfterOrphanedMons
	release
	end

LavenderTown_VolunteerPokemonHouse_EventScript_LittleBoy::
	msgbox LavenderTown_VolunteerPokemonHouse_Text_MonsNiceToHug, MSGBOX_NPC
	end

LavenderTown_VolunteerPokemonHouse_EventScript_Youngster::
	lock
	faceplayer
	goto_if_set FLAG_RESCUED_MR_FUJI, LavenderTown_VolunteerPokemonHouse_EventScript_YoungsterFujiBack
	msgbox LavenderTown_VolunteerPokemonHouse_Text_WhereDidMrFujiGo
	release
	end

LavenderTown_VolunteerPokemonHouse_EventScript_YoungsterFujiBack::
	msgbox LavenderTown_VolunteerPokemonHouse_Text_MrFujiWasPrayingForCubonesMother
	release
	end

LavenderTown_VolunteerPokemonHouse_EventScript_Nidorino::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_NIDORINO, CRY_MODE_NORMAL
	msgbox LavenderTown_VolunteerPokemonHouse_Text_Nidorino
	waitmoncry
	release
	end

LavenderTown_VolunteerPokemonHouse_EventScript_Psyduck::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_PSYDUCK, CRY_MODE_NORMAL
	msgbox LavenderTown_VolunteerPokemonHouse_Text_Psyduck
	waitmoncry
	release
	end

LavenderTown_VolunteerPokemonHouse_EventScript_PokemonFanMagazine::
	lockall
	famechecker FAMECHECKER_MRFUJI, 2
	msgbox LavenderTown_VolunteerPokemonHouse_Text_GrandPrizeDrawingClipped
	releaseall
	end

LavenderTown_VolunteerPokemonHouse_EventScript_Bookshelf::
	msgbox LavenderTown_VolunteerPokemonHouse_Text_PokemonMagazinesLineShelf, MSGBOX_SIGN
	end

LavenderTown_PokemonCenter_1F_Text_HearMrFujiNotFromAroundHere::
	.string "I recently moved to this town.\p"
	.string "I hear that MR. FUJI's not from\n"
	.string "these parts originally, either.$"

LavenderTown_VolunteerPokemonHouse_Text_WhereDidMrFujiGo::
	.string "That's odd, MR. FUJI isn't here.\n"
	.string "Where'd he go?$"

LavenderTown_VolunteerPokemonHouse_Text_MrFujiWasPrayingForCubonesMother::
	.string "MR. FUJI had been praying alone\n"
	.string "for CUBONE's mother.$"

LavenderTown_VolunteerPokemonHouse_Text_MrFujiLooksAfterOrphanedMons::
	.string "This is really MR. FUJI's house.\p"
	.string "He's really kind.\p"
	.string "He looks after abandoned and\n"
	.string "orphaned POKéMON.$"

LavenderTown_VolunteerPokemonHouse_Text_MonsNiceToHug::
	.string "It's so warm!\n"
	.string "POKéMON are so nice to hug.$"

LavenderTown_VolunteerPokemonHouse_Text_Nidorino::
	.string "NIDORINO: Gaoo!$"

LavenderTown_VolunteerPokemonHouse_Text_Psyduck::
	.string "PSYDUCK: Gwappa!$"

LavenderTown_VolunteerPokemonHouse_Text_IdLikeYouToHaveThis::
	.string "MR. FUJI: {PLAYER}…\p"
	.string "Your POKéDEX quest is one that\n"
	.string "requires strong dedication.\p"
	.string "Without deep love for POKéMON,\n"
	.string "your quest may fail.\p"
	.string "I'm not sure if this will help you,\n"
	.string "but I'd like you to have it.$"

LavenderTown_VolunteerPokemonHouse_Text_ReceivedPokeFluteFromMrFuji::
	.string "{PLAYER} received a POKé FLUTE\n"
	.string "from MR. FUJI.$"

LavenderTown_VolunteerPokemonHouse_Text_ExplainPokeFlute::
	.string "Upon hearing the POKé FLUTE,\n"
	.string "sleeping POKéMON will spring awake.\p"
	.string "Try using it on POKéMON that are\n"
	.string "sleeping obstacles.$"

LavenderTown_VolunteerPokemonHouse_Text_MustMakeRoomForThis::
	.string "You must make room for this!$"

LavenderTown_VolunteerPokemonHouse_Text_HasPokeFluteHelpedYou::
	.string "MR. FUJI: Has my POKé FLUTE\n"
	.string "helped you?$"

LavenderTown_VolunteerPokemonHouse_Text_GrandPrizeDrawingClipped::
	.string "POKéMON FAN MAGAZINE\n"
	.string "Monthly Grand Prize Drawing!\p"
	.string "The application form is…\p"
	.string "Gone! It's been clipped out.\n"
	.string "Someone must have applied already.$"

LavenderTown_VolunteerPokemonHouse_Text_PokemonMagazinesLineShelf::
	.string "POKéMON magazines line the shelf.\p"
	.string "POKéMON INSIDER…\p"
	.string "POKéMON FAN…$"


@ (Claude's draft, Oct 31)
LavenderTown_VolunteerPokemonHouse_Text_FujiSomethingElse::
	.string "MR. FUJI: There is one more thing.\p"
	.string "When I cared for the orphans up in\n"
	.string "the TOWER, I carried a charm to\l"
	.string "keep the wild POKéMON away.\p"
	.string "It is called the SPIRIT WARD. It\n"
	.string "never wears out. I can hold it up,\l"
	.string "or put it away, as I please.\p"
	.string "But it is not a thing to give\n"
	.string "lightly. You must earn it.$"

LavenderTown_VolunteerPokemonHouse_Text_FujiStillWaiting::
	.string "MR. FUJI: The SPIRIT WARD is still\n"
	.string "waiting for its next keeper.$"

LavenderTown_VolunteerPokemonHouse_Text_FujiAskBattle::
	.string "Show me how you and your POKéMON\n"
	.string "stand together. Will you battle\l"
	.string "an old man?$"

LavenderTown_VolunteerPokemonHouse_Text_FujiBattleIntro::
	.string "MR. FUJI: These POKéMON watch over\n"
	.string "the little ones here. They will not\l"
	.string "go easy on you.$"

LavenderTown_VolunteerPokemonHouse_Text_FujiWon::
	.string "MR. FUJI: Your POKéMON fought for\n"
	.string "you, and you fought for them.\p"
	.string "That is all the SPIRIT WARD has\n"
	.string "ever asked of its keeper.$"

LavenderTown_VolunteerPokemonHouse_Text_ExplainSpiritWard::
	.string "Hold it up, and wild POKéMON weaker\n"
	.string "than your lead will stay away.\p"
	.string "Put it away when you want to meet\n"
	.string "them again. It never runs out.\p"
	.string "You'll find it with your TOOLS. You\n"
	.string "can register it to SELECT, too.$"

LavenderTown_VolunteerPokemonHouse_Text_FujiLost::
	.string "MR. FUJI: Not yet, I think.\p"
	.string "Let me tend to your POKéMON. Come\n"
	.string "back when you are ready.$"

LavenderTown_VolunteerPokemonHouse_Text_FujiLater::
	.string "MR. FUJI: Whenever you are ready.\n"
	.string "I am not going anywhere.$"
