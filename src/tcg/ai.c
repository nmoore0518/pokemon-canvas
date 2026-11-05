// Pokémon Canvas: the Trading Card Game AI's deck logic and its entry points, ported from the Game
// Boy Color game's engine/duel/ai/deck_ai.asm, decks/legendary_dragonite.asm, decks/general.asm,
// boss_deck_set_up.asm and pkmn_powers.asm (pret/poketcg) (TcgAi_ prefixed).
// Grandmaster Rod plays his deck's logic (AIActionTable_LegendaryDragonite); the player's decks,
// when the AI plays them (the headless AI-vs-AI runs: the game's opponents had their own logic for
// these), the general decks' (AIActionTable_GeneralDecks).
// Not ported: the AI's Pokémon Powers (Energy Trans, Rain Dance, Damage Swap, Peek, Heal, Cowardice:
// only the player's decks have them, and the AI doesn't use them).
#include "global.h"
#include "tcg.h"
#include "tcg_ai.h"

// ---- pkmn_powers.asm: not ported (see above) ------------------------------------------------

bool32 TcgAi_HandleAIPkmnPowers(void)
{
    return FALSE;
}

void TcgAi_HandleAIEnergyTrans(u32 mode)
{
}

// ---- boss_deck_set_up.asm --------------------------------------------------------------------

// two Basic Pokémon and two Energy cards (at least) in the opening hand, and four of each in it and
// the six cards after the Prizes (the game also meant to keep the cards wAICardListAvoidPrize lists
// out of the Prizes, but a bug ('cp a') skips it: so here)
static void CountBasicsAndEnergy(const u8 *deck, u32 n)
{
    while (n--)
    {
        Tcg_LoadCard1(*deck++);
        if (gTcg->loadedCard1->type < TCG_TYPE_ENERGY)
        {
            if (gTcg->loadedCard1->stage == TCG_BASIC)
                gTcgAi->setupBasicPokemonCount++;
        }
        else if (gTcg->loadedCard1->type != TCG_TYPE_TRAINER)
        {
            gTcgAi->setupEnergyCount++;
        }
    }
}

void TcgAi_SetUpBossStartingHandAndDeck(void)
{
    const u8 *deck = Tcg_TurnVar(TCG_DUELVARS_DECK_CARDS);
    u32 i;
    for (i = 0; i < TCG_STARTING_HAND_SIZE; i++)
    {
        u32 card = *Tcg_TurnVar(TCG_DUELVARS_HAND);
        Tcg_RemoveCardFromHand(card);
        Tcg_ReturnCardToDeck(card);
    }
    for (;;)
    {
        gTcgAi->setupBasicPokemonCount = 0;
        gTcgAi->setupEnergyCount = 0;
        CountBasicsAndEnergy(deck, TCG_STARTING_HAND_SIZE);
        if (gTcgAi->setupBasicPokemonCount >= 2 && gTcgAi->setupEnergyCount >= 2)
        {
            CountBasicsAndEnergy(deck + TCG_STARTING_HAND_SIZE + 6, 6);
            if (gTcgAi->setupBasicPokemonCount >= 4 && gTcgAi->setupEnergyCount >= 4)
                break;
        }
        Tcg_ShuffleDeck();
    }
    for (i = 0; i < TCG_STARTING_HAND_SIZE; i++)
    {
        u32 card = deck[i];
        Tcg_SearchCardInDeckAndAddToHand(card);
        Tcg_AddCardToHand(card);
    }
}

// ---- decks/legendary_dragonite.asm ----------------------------------------------------------

static const u8 sDragonite_ListArena[] = {TCG_CARD_KANGASKHAN, TCG_CARD_LAPRAS, TCG_CARD_CHARMANDER, TCG_CARD_DRATINI, TCG_CARD_MAGIKARP, 0};
static const u8 sDragonite_ListBench[] = {TCG_CARD_CHARMANDER, TCG_CARD_MAGIKARP, TCG_CARD_DRATINI, TCG_CARD_LAPRAS, TCG_CARD_KANGASKHAN, 0};
// (ai_energy: card, max energy, $80 + score)
static const u8 sDragonite_ListEnergy[] =
{
    TCG_CARD_CHARMANDER,     3, 0x80 + 1,
    TCG_CARD_CHARMELEON,     4, 0x80 + 1,
    TCG_CARD_CHARIZARD,      5, 0x80 + 0,
    TCG_CARD_MAGIKARP,       3, 0x80 + 1,
    TCG_CARD_GYARADOS,       4, 0x80 - 1,
    TCG_CARD_DRATINI,        2, 0x80 + 0,
    TCG_CARD_DRAGONAIR,      4, 0x80 + 0,
    TCG_CARD_DRAGONITE_LV41, 3, 0x80 - 1,
    TCG_CARD_KANGASKHAN,     2, 0x80 - 2,
    TCG_CARD_LAPRAS,         3, 0x80 + 0,
    0
};
static const u8 sDragonite_ListPrize[] = {TCG_CARD_GAMBLER, TCG_CARD_DRAGONITE_LV41, TCG_CARD_KANGASKHAN, 0};

static void Dragonite_StartDuel(void)
{
    TcgAi_InitAIDuelVars();
    gTcgAi->cardListAvoidPrize = sDragonite_ListPrize;
    gTcgAi->cardListArenaPriority = sDragonite_ListArena;
    gTcgAi->cardListBenchPriority = sDragonite_ListBench;
    gTcgAi->cardListPlayFromHandPriority = sDragonite_ListBench;
    // (the game's missing store_list_pointer: his retreat list is never stored)
    gTcgAi->cardListEnergyBonus = sDragonite_ListEnergy;
    TcgAi_SetUpBossStartingHandAndDeck();
    if (TcgAi_TrySetUpBossStartingPlayArea())
        TcgAi_PlayInitialBasicCards();
}

// the second half of the turn (after Professor Oak, again): Trainers, retreat, Energy, Pokémon.
// TRUE: the turn's ended.
static bool32 Dragonite_TurnBody(bool32 kangaskhanEnergy)
{
    TcgAi_ProcessHandTrainerCards(TCG_AI_TRAINER_CARD_PHASE_02);
    if (TcgAi_DecidePlayPokemonCard())
        return TRUE;
    TcgAi_ProcessHandTrainerCards(TCG_AI_TRAINER_CARD_PHASE_07);
    TcgAi_ProcessRetreat();
    TcgAi_ProcessHandTrainerCards(TCG_AI_TRAINER_CARD_PHASE_10);
    TcgAi_ProcessHandTrainerCards(TCG_AI_TRAINER_CARD_PHASE_11);
    if (!gTcg->alreadyPlayedEnergy)
    {
        // an Active Kangaskhan with no Energy gets one from the hand first
        bool32 attached = FALSE;
        if (kangaskhanEnergy && Tcg_GetCardIdFromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD)) == TCG_CARD_KANGASKHAN)
        {
            if (TcgAi_CreateEnergyCardListFromHand())
                attached = TRUE;    // (none in the hand: skip)
            else if (TcgAi_CountNumberOfEnergyCardsAttached(TCG_PLAY_AREA_ARENA) == 0)
            {
                gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
                attached = TcgAi_TryToPlayEnergyCard();
            }
        }
        if (!attached)
            TcgAi_ProcessAndTryToPlayEnergy();
    }
    TcgAi_DecidePlayPokemonCard();
    return FALSE;
}

static void Dragonite_DoTurn(void)
{
    TcgAi_InitAITurnVars();
    TcgAi_ProcessHandTrainerCards(TCG_AI_TRAINER_CARD_PHASE_01);
    if (TcgAi_HandleAIAntiMewtwoDeckStrategy())
    {
        if (Dragonite_TurnBody(TRUE))
            return;
        TcgAi_ProcessHandTrainerCards(TCG_AI_TRAINER_CARD_PHASE_15);
        if (gTcgAi->previousAIFlags & TCG_AI_FLAG_USED_PROFESSOR_OAK)
        {
            TcgAi_ProcessHandTrainerCards(TCG_AI_TRAINER_CARD_PHASE_01);
            if (Dragonite_TurnBody(FALSE))
                return;
        }
    }
    if (TcgAi_ProcessAndTryToUseAttack())
        return;
    Tcg_AIMakeDecision(TCG_OPPACTION_FINISH_NO_ATTACK);
}

// ---- decks/general.asm ------------------------------------------------------------------------

// AIMainTurnLogic's phases 02 to 12 and its Energy and Pokémon. TRUE: the turn's ended.
static bool32 General_TurnBody(void)
{
    u32 phase;
    if (TcgAi_HandleAIPkmnPowers())
        return TRUE;
    TcgAi_ProcessHandTrainerCards(TCG_AI_TRAINER_CARD_PHASE_02);
    TcgAi_ProcessHandTrainerCards(TCG_AI_TRAINER_CARD_PHASE_03);
    TcgAi_ProcessHandTrainerCards(TCG_AI_TRAINER_CARD_PHASE_04);
    if (TcgAi_DecidePlayPokemonCard())
        return TRUE;
    for (phase = TCG_AI_TRAINER_CARD_PHASE_05; phase <= TCG_AI_TRAINER_CARD_PHASE_08; phase++)
        TcgAi_ProcessHandTrainerCards(phase);
    TcgAi_ProcessRetreat();
    for (phase = TCG_AI_TRAINER_CARD_PHASE_10; phase <= TCG_AI_TRAINER_CARD_PHASE_12; phase++)
        TcgAi_ProcessHandTrainerCards(phase);
    if (!gTcg->alreadyPlayedEnergy)
        TcgAi_ProcessAndTryToPlayEnergy();
    TcgAi_DecidePlayPokemonCard();
    return FALSE;
}

static void General_DoTurn(void)
{
    TcgAi_InitAITurnVars();
    TcgAi_ProcessHandTrainerCards(TCG_AI_TRAINER_CARD_PHASE_01);
    if (TcgAi_HandleAIAntiMewtwoDeckStrategy())
    {
        if (General_TurnBody())
            return;
        if (TcgAi_HandleAIPkmnPowers())
            return;
        TcgAi_ProcessHandTrainerCards(TCG_AI_TRAINER_CARD_PHASE_13);
        TcgAi_ProcessHandTrainerCards(TCG_AI_TRAINER_CARD_PHASE_15);
        if (gTcgAi->previousAIFlags & TCG_AI_FLAG_USED_PROFESSOR_OAK)
        {
            TcgAi_ProcessHandTrainerCards(TCG_AI_TRAINER_CARD_PHASE_01);
            if (General_TurnBody())
                return;
            if (TcgAi_HandleAIPkmnPowers())
                return;
            TcgAi_ProcessHandTrainerCards(TCG_AI_TRAINER_CARD_PHASE_13);
        }
    }
    if (TcgAi_ProcessAndTryToUseAttack())
        return;
    Tcg_AIMakeDecision(TCG_OPPACTION_FINISH_NO_ATTACK);
}

static void General_StartDuel(void)
{
    TcgAi_InitAIDuelVars();
    TcgAi_PlayInitialBasicCards();
}

// ---- deck_ai.asm: AIDoAction -----------------------------------------------------------------

u32 TcgAi_DoAction(u32 action)
{
    bool32 rod;
    gTcgAi = TcgAi_ForDuelist(gTcg->whoseTurn);
    rod = (gTcgAi->deckId == TCG_DECKID_LEGENDARY_DRAGONITE);
    switch (action)
    {
    case TCG_AIACTION_DO_TURN:
        if (rod)
            Dragonite_DoTurn();
        else
            General_DoTurn();
        break;
    case TCG_AIACTION_START_DUEL:
        if (rod)
            Dragonite_StartDuel();
        else
            General_StartDuel();
        break;
    case TCG_AIACTION_FORCED_SWITCH:
    case TCG_AIACTION_KO_SWITCH:
        return TcgAi_DecideBenchPokemonToSwitchTo();
    case TCG_AIACTION_TAKE_PRIZE:
        Tcg_AIPickPrizeCards();
        break;
    }
    return 0;
}

// the duel's hooks (include/tcg.h)
u32 TcgAi_KOSwitch(void)
{
    return TcgAi_DoAction(TCG_AIACTION_KO_SWITCH);
}

u32 TcgAi_ForcedSwitch(void)
{
    return TcgAi_DoAction(TCG_AIACTION_FORCED_SWITCH);
}

void TcgAi_StartDuel(void)
{
    TcgAi_DoAction(TCG_AIACTION_START_DUEL);
}

void TcgAi_DoTurn(void)
{
    TcgAi_DoAction(TCG_AIACTION_DO_TURN);
}
