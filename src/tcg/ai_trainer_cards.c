// Pokémon Canvas: the Trading Card Game AI's Trainer cards, ported from the Game Boy Color game's
// engine/duel/ai/trainer_cards.asm and data/duel/ai_trainer_card_logic.asm (pret/poketcg)
// (TcgAi_ prefixed): the hand's Trainers, at each phase of the AI's turn, each with a routine that
// decides whether to play it, and one that plays it.
// Ported so far: the Trainers in Grandmaster Rod's deck (Professor Oak, Pokémon Trader, Pokémon
// Breeder, Energy Retrieval, Super Energy Retrieval, Switch, Gambler). The rest of the game's table
// (the Trainers only in the player's decks: the AI only plays those in the headless AI-vs-AI runs)
// isn't here yet, so the AI doesn't play them.
#include "global.h"
#include "tcg.h"
#include "tcg_ai.h"

static inline u32 TurnCardId(u32 deckIndex)
{
    return Tcg_GetCardIdFromDeckIndex(deckIndex);
}

// RemoveCardFromList: the list's element at `c` taken out (the rest move up)
static void RemoveCardFromList(u8 *c)
{
    do
        c[0] = c[1];
    while (*c++ != 0xFF);
}

// FindDuplicateCards (trainer_cards.asm's): a card the list has two of (the last such second card;
// a non-Energy one first). 0xFF: none.
static u32 FindDuplicateCards(const u8 *list)
{
    const u8 *a, *b;
    gTcgAi->wce0f[0] = gTcgAi->wce0f[1] = 0xFF;
    for (a = list; *a != 0xFF; a++)
    {
        u32 id = TurnCardId(*a);
        for (b = a + 1; *b != 0xFF; b++)
        {
            if (TurnCardId(*b) != id)
                continue;
            if (Tcg_Card(id)->type >= TCG_TYPE_ENERGY)
                gTcgAi->wce0f[1] = *b;
            else
                gTcgAi->wce0f[0] = *b;
            break;
        }
    }
    if (gTcgAi->wce0f[0] != 0xFF)
        return gTcgAi->wce0f[0];
    return gTcgAi->wce0f[1];
}

// FindBasicEnergyCardsInLocation (common.asm): wDuelTempList: the basic Energy cards at `location`.
// FALSE: none.
static bool32 FindBasicEnergyCardsInLocation(u32 location)
{
    u32 e, n = 0;
    gTcgAi->tempAI = location;
    for (e = 0; e < TCG_DECK_SIZE; e++)
    {
        u32 id;
        if (*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + e) != location)
            continue;
        id = TurnCardId(e);
        // (the game: card ID below Double Colorless's, the basic Energy cards' IDs)
        if (id >= TCG_CARD_GRASS_ENERGY && id < TCG_CARD_DOUBLE_COLORLESS_ENERGY)
            gTcg->duelTempList[n++] = e;
    }
    if (n == 0)
        return FALSE;
    gTcg->duelTempList[n] = 0xFF;
    return TRUE;
}

// ---- Switch ---------------------------------------------------------------------------------

static bool32 AIDecide_Switch(u8 *param)
{
    u32 cost, attached, loc;
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    if (gTcgAi->playEnergyCardForRetreat)
    {
        cost = Tcg_GetPlayAreaCardRetreatCost();
        attached = TcgAi_CountNumberOfEnergyCardsAttached(TCG_PLAY_AREA_ARENA);
        if (cost >= attached && cost - attached >= 2)
            goto do_switch;
    }
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    cost = Tcg_GetPlayAreaCardRetreatCost();
    if (cost >= 3)
        goto do_switch;
    if (TcgAi_CountNumberOfEnergyCardsAttached(TCG_PLAY_AREA_ARENA) < cost)
        goto do_switch;
    return FALSE;
do_switch:
    loc = TcgAi_DecideBenchPokemonToSwitchTo();
    *param = loc;
    return loc != 0xFF;
}

static void AIPlay_Switch(void)
{
    gTcgAi->currentAIFlags |= TCG_AI_FLAG_USED_SWITCH;
    gTcg->tempCardIndexFf9f = gTcgAi->trainerCardToPlay;
    gTcg->temp = gTcgAi->trainerCardParameter;
    Tcg_AIMakeDecision(TCG_OPPACTION_EXECUTE_TRAINER_EFFECTS);
    gTcgAi->retreatScore = 0;
}

// ---- Gambler (the AI only plays it in the game's Mewtwo mill and Imakuni decks: never, here) --

static bool32 AIDecide_Gambler(u8 *param)
{
    return FALSE;
}

static void AIPlay_Gambler(void)
{
    gTcgAi->currentAIFlags |= TCG_AI_FLAG_MODIFIED_HAND;
    gTcg->tempCardIndexFf9f = gTcgAi->trainerCardToPlay;
    Tcg_AIMakeDecision(TCG_OPPACTION_EXECUTE_TRAINER_EFFECTS);
}

// ---- Professor Oak --------------------------------------------------------------------------

// TRUE: an evolution of the Pokémon at `location` is in the hand (wce08: one's anywhere)
static bool32 LookForEvolution(u32 location)
{
    u32 d;
    gTcgAi->wce08[0] = FALSE;
    for (d = 0; d < TCG_DECK_SIZE; d++)
    {
        if (Tcg_CheckIfCanEvolveInto(location, d, NULL))
            continue;
        gTcgAi->wce08[0] = TRUE;
        if (*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + d) == TCG_CARD_LOCATION_HAND)
            return TRUE;
    }
    return FALSE;
}

static bool32 AIDecide_ProfessorOak(u8 *param)
{
    u32 notInDeck = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_NOT_IN_DECK), n, e;
    u8 score;
    const u8 *c;
    if (notInDeck >= TCG_DECK_SIZE - 6)
        return FALSE;
    // (Legendary Articuno's, Excavation's and Wonders of Science's: not Rod's)
    if (notInDeck >= TCG_DECK_SIZE - 14)
        return FALSE;
    score = 30;
    n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND);
    if (n < 4)
        score += 50;
    else if (n >= 9)
        score -= 30;
    if (TcgAi_CreateEnergyCardListFromHand())
        score += 40;
    if (Tcg_CountTurnDuelistPokemonWithActivePkmnPower(TCG_CARD_BLASTOISE)
     && TcgAi_LookForCardIDInHand(TCG_CARD_WATER_ENERGY) == 0xFF)
        score += 10;
    // (the game's bug: it skips the Pokémon, and counts the first Energy or Trainer, meaning Basics)
    Tcg_CreateHandCardList();
    for (c = gTcg->duelTempList; *c != 0xFF; c++)
    {
        Tcg_LoadCard1(*c);
        if (gTcg->loadedCard1->type < TCG_TYPE_ENERGY)
            continue;
        if (gTcg->loadedCard1->stage == TCG_BASIC)
        {
            score += 10;
            break;
        }
    }
    // an evolution somewhere, but none in the hand
    gTcgAi->wce0f[0] = gTcgAi->wce0f[1] = 0;
    for (e = 0, n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA); e < n; e++)
    {
        if (LookForEvolution(e))
            gTcgAi->wce0f[0] = TRUE;
        if (gTcgAi->wce08[0] == TRUE)
            gTcgAi->wce0f[1] = TRUE;
    }
    if (gTcgAi->wce0f[1] && !gTcgAi->wce0f[0])
        score += 10;
    gTcgAi->wce06 = score;
    return score >= 60;
}

static void AIPlay_ProfessorOak(void)
{
    gTcgAi->currentAIFlags |= TCG_AI_FLAG_USED_PROFESSOR_OAK | TCG_AI_FLAG_MODIFIED_HAND;
    gTcg->tempCardIndexFf9f = gTcgAi->trainerCardToPlay;
    Tcg_AIMakeDecision(TCG_OPPACTION_EXECUTE_TRAINER_EFFECTS);
}

// ---- Pokémon Trader -------------------------------------------------------------------------

// Rod's trades: a Pokémon of his lines from the deck (wce1a), for a hand card he has two of
static bool32 AIDecide_PokemonTrader_LegendaryDragonite(u8 *param)
{
    static const u8 sHandAndPlay[][2] =    // (deck card, hand or play area card)
    {
        {TCG_CARD_GYARADOS, TCG_CARD_MAGIKARP},
    };
    static const u8 sTrades[][3] =      // (deck card, the card it's for, in the hand (0) or hand and play area (1))
    {
        {TCG_CARD_GYARADOS, TCG_CARD_MAGIKARP, 1},
        {TCG_CARD_MAGIKARP, TCG_CARD_GYARADOS, 0},
        {TCG_CARD_DRAGONAIR, TCG_CARD_DRATINI, 1},
        {TCG_CARD_DRAGONITE_LV41, TCG_CARD_DRAGONAIR, 1},
        {TCG_CARD_DRATINI, TCG_CARD_DRAGONAIR, 0},
        {TCG_CARD_DRAGONAIR, TCG_CARD_DRAGONITE_LV41, 0},
        {TCG_CARD_CHARMELEON, TCG_CARD_CHARMANDER, 1},
        {TCG_CARD_CHARIZARD, TCG_CARD_CHARMELEON, 1},
        {TCG_CARD_CHARMANDER, TCG_CARD_CHARMELEON, 0},
        {TCG_CARD_CHARMELEON, TCG_CARD_CHARIZARD, 0},
    };
    static const u8 sGive[] = {TCG_CARD_DRAGONAIR, TCG_CARD_CHARMELEON, TCG_CARD_GYARADOS, TCG_CARD_MAGIKARP, TCG_CARD_CHARMANDER, TCG_CARD_DRATINI};
    u32 i, found = 0xFF;
    (void)sHandAndPlay;
    if (TcgAi_CountOppEnergyCardsInHandAndAttached() < 5 || TcgAi_CountPokemonCardsInHandAndInPlayArea() < 5)
    {
        found = TcgAi_LookForCardIDInLocation(TCG_CARD_LOCATION_DECK, TCG_CARD_KANGASKHAN);
        if (found == 0xFF)
            return FALSE;
    }
    else
    {
        for (i = 0; i < ARRAY_COUNT(sTrades) && found == 0xFF; i++)
        {
            if (sTrades[i][2])
                found = TcgAi_LookForCardIDInDeck_GivenCardIDInHandAndPlayArea(sTrades[i][0], sTrades[i][1]);
            else
                found = TcgAi_LookForCardIDInDeck_GivenCardIDInHand(sTrades[i][0], sTrades[i][1]);
        }
        if (found == 0xFF)
            return FALSE;
    }
    gTcgAi->wce1a = found;
    for (i = 0; i < ARRAY_COUNT(sGive); i++)
    {
        if (TcgAi_CheckIfHasCardIDInHand(sGive[i]))
        {
            *param = gTcg->tempCardIndex;
            return TRUE;
        }
    }
    return FALSE;
}

static bool32 AIDecide_PokemonTrader(u8 *param)
{
    if (gTcgAi->deckId == TCG_DECKID_LEGENDARY_DRAGONITE)
        return AIDecide_PokemonTrader_LegendaryDragonite(param);
    return FALSE;   // (the other decks' with their own: not ours)
}

static void AIPlay_PokemonTrader(void)
{
    gTcg->tempCardIndexFf9f = gTcgAi->trainerCardToPlay;
    gTcg->temp = gTcgAi->trainerCardParameter;
    gTcg->tempPlayAreaLocationFfa1 = gTcgAi->wce1a;
    Tcg_AIMakeDecision(TCG_OPPACTION_EXECUTE_TRAINER_EFFECTS);
}

// ---- Pokémon Breeder ------------------------------------------------------------------------

// a Basic at `e` the Stage 2 `d` can evolve: its fitness (damage counters left << 4 | energy, up to 15)
static void CalculateFitness(u32 d, u32 e)
{
    u32 counters = TcgAi_ConvertHPToDamageCounters(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + e)), energy;
    Tcg_GetPlayAreaCardAttachedEnergies(e);
    energy = gTcg->totalAttachedEnergies;
    if (energy >= 16)
        energy = 15;
    gTcgAi->wce08[e] = ((counters << 4) & 0xF0) | energy;
    gTcgAi->wce0f[e] = d;
    gTcgAi->wce06++;
}

// TRUE: don't breed a Dragonite Lv41 here (benched: not 8 counters' damage or more in play to heal;
// Active: hardly hurt, or under 3 energy)
static bool32 HandleDragoniteLv41Evolution(u32 d, u32 e)
{
    u32 b, c;
    if (TurnCardId(d) != TCG_CARD_DRAGONITE_LV41)
        return FALSE;
    if (e != TCG_PLAY_AREA_ARENA)
    {
        for (c = 0, b = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA); b > 0; b--)
            c = (u8)(c + TcgAi_ConvertHPToDamageCounters(Tcg_GetCardDamageAndMaxHP(b - 1, NULL)));
        return c <= 7;
    }
    if (Tcg_GetCardDamageAndMaxHP(TCG_PLAY_AREA_ARENA, NULL) < 5)
        return TRUE;
    return TcgAi_CountNumberOfEnergyCardsAttached(TCG_PLAY_AREA_ARENA) < 3;
}

static bool32 AIDecide_PokemonBreeder(u8 *param)
{
    const u8 *c;
    u32 n, e, best;
    if (Tcg_IsPrehistoricPowerActive())
        return FALSE;
    memset(gTcgAi->wce08, 0, 7);
    gTcgAi->wce06 = 0;
    n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    // a Stage 2 with a Pokémon Power first (of the game's list: Venusaur, Blastoise, Vileplume, Alakazam)
    Tcg_CreateHandCardList();
    for (c = gTcg->duelTempList; *c != 0xFF; c++)
    {
        u32 id;
        Tcg_LoadCard1(*c);
        id = gTcg->loadedCard1Id;
        if (id != TCG_CARD_VENUSAUR_LV67 && id != TCG_CARD_BLASTOISE && id != TCG_CARD_VILEPLUME && id != TCG_CARD_ALAKAZAM)
            continue;
        for (e = 0; e < n; e++)
            if (!Tcg_CheckIfCanEvolveInto_BasicToStage2(e, *c))
                CalculateFitness(*c, e);
    }
    if (gTcgAi->wce06)
    {
        gTcgAi->wce06 = 0;
        for (e = 0; e < n; e++)
        {
            if (gTcgAi->wce08[e] > gTcgAi->wce06)
            {
                gTcgAi->wce06 = gTcgAi->wce08[e];
                gTcgAi->wce07 = e;
            }
        }
        gTcgAi->wce1a = gTcgAi->wce0f[gTcgAi->wce07 % TCG_MAX_PLAY_AREA_POKEMON];
        *param = gTcgAi->wce07;
        return TRUE;
    }
    // .check_evolution_and_dragonite: any Stage 2, onto a Basic with 2 energy or more
    memset(gTcgAi->wce08, 0, 7);
    gTcgAi->wce06 = 0;
    Tcg_CreateHandCardList();
    for (c = gTcg->duelTempList; *c != 0xFF; c++)
        for (e = 0; e < n; e++)
            if (!Tcg_CheckIfCanEvolveInto_BasicToStage2(e, *c) && !HandleDragoniteLv41Evolution(*c, e))
                CalculateFitness(*c, e);
    if (gTcgAi->wce06 == 0)
        return FALSE;
    gTcgAi->wce06 = 0;
    best = gTcgAi->wce07 = 0xFF;
    for (e = 0; e < n; e++)
    {
        if (gTcgAi->wce08[e] <= gTcgAi->wce06 || (gTcgAi->wce08[e] & 0x0F) < 2)
            continue;
        gTcgAi->wce06 = gTcgAi->wce08[e];
        best = gTcgAi->wce07 = e;
    }
    if (best == 0xFF)
        return FALSE;
    gTcgAi->wce1a = gTcgAi->wce0f[best];
    *param = best;
    return TRUE;
}

static void AIPlay_PokemonBreeder(void)
{
    gTcg->tempCardIndexFf9f = gTcgAi->trainerCardToPlay;
    gTcg->tempPlayAreaLocationFfa1 = gTcgAi->trainerCardParameter;
    gTcg->temp = gTcgAi->wce1a;
    Tcg_AIMakeDecision(TCG_OPPACTION_EXECUTE_TRAINER_EFFECTS);
}

// ---- Energy Retrieval, Super Energy Retrieval ------------------------------------------------

// up to `max` basic Energy cards from the discard pile into gTcgAi->wce1a.. (useful ones to the
// Pokémon in play first): the number picked
static u32 PickEnergyFromDiscard(u8 *picks, u32 max)
{
    u32 n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA), e, count = 0;
    u8 *c;
    for (e = 0; e < max; e++)
        picks[e] = 0xFF;
    for (e = 0; e < n; e++)
    {
        gTcgAi->tempCardId = TurnCardId(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + e));
        Tcg_LoadCard1(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + e));
        gTcgAi->tempCardType = gTcg->loadedCard1->type | TCG_TYPE_ENERGY;
        for (c = gTcg->duelTempList; *c != 0xFF; c++)
        {
            if (!TcgAi_CheckIfEnergyIsUseful(*c))
                continue;
            picks[count++] = *c;
            if (count == max)
                return count;
            RemoveCardFromList(c);
            break;
        }
    }
    for (c = gTcg->duelTempList; *c != 0xFF;)
    {
        picks[count++] = *c;
        if (count == max)
            return count;
        RemoveCardFromList(c);
    }
    return count;
}

// (only when the hand has no Energy; Go Go Rain Dance's Blastoise check: not Rod's)
static bool32 AIDecide_EnergyRetrieval(u8 *param)
{
    u8 picks[2];
    u32 dup;
    if (!TcgAi_CreateEnergyCardListFromHand())
        return FALSE;
    Tcg_CreateHandCardList();
    dup = FindDuplicateCards(gTcg->duelTempList);
    if (dup == 0xFF)
        return FALSE;
    gTcgAi->wce06 = dup;
    if (!FindBasicEnergyCardsInLocation(TCG_CARD_LOCATION_DISCARD_PILE))
        return FALSE;
    gTcgAi->wce1c = 0xFF;
    if (PickEnergyFromDiscard(picks, 2) == 0)
        return FALSE;
    gTcgAi->wce1a = picks[0];
    gTcgAi->wce1b = picks[1];
    *param = gTcgAi->wce06;
    return TRUE;
}

static void AIPlay_EnergyRetrieval(void)
{
    gTcgAi->currentAIFlags |= TCG_AI_FLAG_MODIFIED_HAND;
    gTcg->tempCardIndexFf9f = gTcgAi->trainerCardToPlay;
    gTcg->temp = gTcgAi->trainerCardParameter;
    gTcg->tempPlayAreaLocationFfa1 = gTcgAi->wce1a;
    gTcg->tempRetreatCostCards[0] = gTcgAi->wce1b;
    if (gTcgAi->wce1b != 0xFF)
        gTcg->tempRetreatCostCards[1] = 0xFF;
    Tcg_AIMakeDecision(TCG_OPPACTION_EXECUTE_TRAINER_EFFECTS);
}

static bool32 AIDecide_SuperEnergyRetrieval(u8 *param)
{
    u8 picks[4];
    u32 dup1, dup2;
    u8 *c;
    if (!TcgAi_CreateEnergyCardListFromHand())
        return FALSE;
    Tcg_CreateHandCardList();
    dup1 = FindDuplicateCards(gTcg->duelTempList);
    if (dup1 == 0xFF)
        return FALSE;
    gTcgAi->wce06 = dup1;
    for (c = gTcg->duelTempList; *c != dup1; c++)
        ;
    RemoveCardFromList(c);
    dup2 = FindDuplicateCards(gTcg->duelTempList);
    if (dup2 == 0xFF)
        return FALSE;
    gTcgAi->wce08[0] = dup2;
    if (!FindBasicEnergyCardsInLocation(TCG_CARD_LOCATION_DISCARD_PILE))
        return FALSE;
    gTcgAi->wce1f = 0xFF;
    if (PickEnergyFromDiscard(picks, 4) == 0)
        return FALSE;
    gTcgAi->wce1b = picks[0];
    gTcgAi->wce1c = picks[1];
    gTcgAi->wce1d = picks[2];
    gTcgAi->wce1e = picks[3];
    gTcgAi->wce1a = gTcgAi->wce08[0];
    *param = gTcgAi->wce06;
    return TRUE;
}

static void AIPlay_SuperEnergyRetrieval(void)
{
    gTcgAi->currentAIFlags |= TCG_AI_FLAG_MODIFIED_HAND;
    gTcg->tempCardIndexFf9f = gTcgAi->trainerCardToPlay;
    gTcg->temp = gTcgAi->trainerCardParameter;
    gTcg->tempPlayAreaLocationFfa1 = gTcgAi->wce1a;
    gTcg->tempRetreatCostCards[0] = gTcgAi->wce1b;
    gTcg->tempRetreatCostCards[1] = gTcgAi->wce1c;
    if (gTcgAi->wce1c != 0xFF)
    {
        gTcg->tempRetreatCostCards[2] = gTcgAi->wce1d;
        if (gTcgAi->wce1d != 0xFF)
        {
            gTcg->tempRetreatCostCards[3] = gTcgAi->wce1e;
            if (gTcgAi->wce1e != 0xFF)
                gTcg->tempRetreatCostCards[4] = 0xFF;
        }
    }
    Tcg_AIMakeDecision(TCG_OPPACTION_EXECUTE_TRAINER_EFFECTS);
}

// ---- the table ------------------------------------------------------------------------------

struct TcgAiTrainerCardLogic
{
    u8 phase, cardId;
    bool32 (*decide)(u8 *param);
    void (*play)(void);
};

static const struct TcgAiTrainerCardLogic sAITrainerCardLogic[] =
{
    {TCG_AI_TRAINER_CARD_PHASE_09, TCG_CARD_SWITCH, AIDecide_Switch, AIPlay_Switch},
    {TCG_AI_TRAINER_CARD_PHASE_07, TCG_CARD_POKEMON_BREEDER, AIDecide_PokemonBreeder, AIPlay_PokemonBreeder},
    {TCG_AI_TRAINER_CARD_PHASE_15, TCG_CARD_PROFESSOR_OAK, AIDecide_ProfessorOak, AIPlay_ProfessorOak},
    {TCG_AI_TRAINER_CARD_PHASE_10, TCG_CARD_ENERGY_RETRIEVAL, AIDecide_EnergyRetrieval, AIPlay_EnergyRetrieval},
    {TCG_AI_TRAINER_CARD_PHASE_11, TCG_CARD_SUPER_ENERGY_RETRIEVAL, AIDecide_SuperEnergyRetrieval, AIPlay_SuperEnergyRetrieval},
    {TCG_AI_TRAINER_CARD_PHASE_01, TCG_CARD_GAMBLER, AIDecide_Gambler, AIPlay_Gambler},
    {TCG_AI_TRAINER_CARD_PHASE_02, TCG_CARD_POKEMON_TRADER, AIDecide_PokemonTrader, AIPlay_PokemonTrader},
};

// _AIProcessHandTrainerCards: each Trainer in the hand with logic for this phase, decided on and played
void TcgAi_ProcessHandTrainerCards(u32 phase)
{
    u8 hand[TCG_DUEL_TEMP_LIST_SIZE];
    const u8 *h;
    gTcgAi->trainerCardPhase = phase;
    Tcg_CreateHandCardList();
    memcpy(hand, gTcg->duelTempList, sizeof(hand));
    for (h = hand; *h != 0xFF; h++)
    {
        u32 i;
        gTcgAi->trainerCardToPlay = *h;
        for (i = 0; i < ARRAY_COUNT(sAITrainerCardLogic); i++)
        {
            const struct TcgAiTrainerCardLogic *l = &sAITrainerCardLogic[i];
            u8 param = 0;
            gTcgAi->currentAIFlags = 0;
            if (l->phase != phase)
                continue;
            gTcgAi->trainerLogicCard = l->cardId;
            Tcg_LoadCard1(gTcgAi->trainerCardToPlay);
            if (gTcg->loadedCard1Id == TCG_CARD_SWITCH && (gTcgAi->previousAIFlags & TCG_AI_FLAG_USED_SWITCH))
                continue;
            if (gTcg->loadedCard1Id != l->cardId)
                continue;
            gTcg->tempCardIndexFf9f = gTcgAi->trainerCardToPlay;
            if (Tcg_CheckCantUseTrainerDueToEffect())
                continue;
            Tcg_LoadNonPokemonCardEffectCommands();
            if (Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_INITIAL_EFFECT_1))
                continue;
            if (TcgAi_ChooseRandomlyNotToDoAction())
                continue;
            if (!l->decide(&param))
                continue;
            gTcgAi->trainerCardParameter = param;
            gTcg->tempCardIndexFf9f = gTcgAi->trainerCardToPlay;
            if (Tcg_AIMakeDecision(TCG_OPPACTION_PLAY_TRAINER))
                continue;
            l->play();
            gTcgAi->previousAIFlags |= gTcgAi->currentAIFlags;
            if (gTcgAi->currentAIFlags & TCG_AI_FLAG_MODIFIED_HAND)
            {
                // the hand's changed: from the top again
                Tcg_CreateHandCardList();
                memcpy(hand, gTcg->duelTempList, sizeof(hand));
                gTcgAi->previousAIFlags &= ~TCG_AI_FLAG_MODIFIED_HAND;
                h = hand - 1;
            }
            break;
        }
    }
}
