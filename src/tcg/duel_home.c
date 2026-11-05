// Pokémon Canvas: the Trading Card Game duel's basic helpers, ported from the Game Boy Color game's
// home/duel.asm (pret/poketcg), function by function, under the same names (Tcg_ prefixed). Deck
// indexes (0-59) and duel variables (DUELVARS_*) work as in the game. Where the game returns carry,
// these return TRUE.
#include "global.h"
#include "tcg.h"
#include "random.h"

EWRAM_DATA static struct TcgDuel sTcgDuel = {0};
struct TcgDuel *const gTcg = &sTcgDuel;   // (const: initialised data outside EWRAM is dropped in the tests)

#define TCG_PLAY_AREA(loc)  (TCG_CARD_LOCATION_PLAY_AREA | (loc))

// Random: 0..n-1 (the game's Random)
u32 Tcg_Random(u32 n)
{
    return n ? Random() % n : 0;
}

u8 *Tcg_TurnVar(u32 offset)
{
    return &gTcg->vars[gTcg->whoseTurn][offset & 0xFF];
}

u8 *Tcg_NonTurnVar(u32 offset)
{
    return &gTcg->vars[gTcg->whoseTurn ^ 1][offset & 0xFF];
}

void Tcg_SwapTurn(void)
{
    gTcg->whoseTurn ^= 1;
}

const struct TcgCard *Tcg_Card(u32 cardId)
{
    return &gTcgCards[cardId < TCG_NUM_CARDS ? cardId : TCG_CARD_NONE];
}

// _GetCardIDFromDeckIndex: the turn holder's deck
u32 Tcg_GetCardIdFromDeckIndex(u32 deckIndex)
{
    return gTcg->deck[gTcg->whoseTurn][deckIndex % TCG_DECK_SIZE];
}

void Tcg_LoadCard1(u32 deckIndex)
{
    gTcg->loadedCard1Id = Tcg_GetCardIdFromDeckIndex(deckIndex);
    gTcg->loadedCard1 = Tcg_Card(gTcg->loadedCard1Id);
}

void Tcg_LoadCard2(u32 deckIndex)
{
    gTcg->loadedCard2Id = Tcg_GetCardIdFromDeckIndex(deckIndex);
    gTcg->loadedCard2 = Tcg_Card(gTcg->loadedCard2Id);
}

// the turn holder's prizes not drawn yet
u32 Tcg_CountPrizes(void)
{
    u32 bits = *Tcg_TurnVar(TCG_DUELVARS_PRIZES), n = 0;
    while (bits)
    {
        n += bits & 1;
        bits >>= 1;
    }
    return n;
}

// ShuffleCards: each card swapped with a random one (of the `count` from `cards`)
void Tcg_ShuffleCards(u8 *cards, u32 count)
{
    u32 i;
    for (i = 0; i < count; i++)
    {
        u32 j = Tcg_Random(count);
        u8 t = cards[j];
        cards[j] = cards[i];
        cards[i] = t;
    }
}

// the turn holder's deck (the cards still in it)
void Tcg_ShuffleDeck(void)
{
    u32 notInDeck = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_NOT_IN_DECK);
    Tcg_ShuffleCards(Tcg_TurnVar(TCG_DUELVARS_DECK_CARDS + notInDeck), TCG_DECK_SIZE - notInDeck);
}

// The turn holder's top card, its location CARD_LOCATION_JUST_DRAWN (Tcg_AddCardToHand next).
// TRUE: the deck's empty.
bool32 Tcg_DrawCardFromDeck(u8 *deckIndex)
{
    u8 *n = Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_NOT_IN_DECK);
    u32 card;
    if (*n >= TCG_DECK_SIZE)
        return TRUE;
    card = *Tcg_TurnVar(TCG_DUELVARS_DECK_CARDS + *n);
    (*n)++;
    *Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + card) = TCG_CARD_LOCATION_JUST_DRAWN;
    if (deckIndex)
        *deckIndex = card;
    return FALSE;
}

// a card onto the top of the turn holder's deck
void Tcg_ReturnCardToDeck(u32 deckIndex)
{
    u8 *n = Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_NOT_IN_DECK);
    (*n)--;
    *Tcg_TurnVar(TCG_DUELVARS_DECK_CARDS + *n) = deckIndex;
    *Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + deckIndex) = TCG_CARD_LOCATION_DECK;
}

// a card taken out of the turn holder's deck (the cards above it move down one), its location
// CARD_LOCATION_JUST_DRAWN (Tcg_AddCardToHand next)
void Tcg_SearchCardInDeckAndAddToHand(u32 deckIndex)
{
    u8 *n = Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_NOT_IN_DECK);
    u32 inDeck = TCG_DECK_SIZE - *n, b;
    u8 *cards = Tcg_TurnVar(TCG_DUELVARS_DECK_CARDS);
    s32 src, dst;
    (*n)++;
    *Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + deckIndex) |= TCG_CARD_LOCATION_JUST_DRAWN;
    // (from the bottom card up, every card but this one shifts toward the bottom)
    dst = src = TCG_DECK_SIZE - 1;
    for (b = 0; b < inDeck; b++, src--)
    {
        if (cards[src] == deckIndex)
            continue;
        cards[dst--] = cards[src];
    }
}

void Tcg_AddCardToHand(u32 deckIndex)
{
    u8 *n = Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND);
    *Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + deckIndex) = TCG_CARD_LOCATION_HAND;
    (*n)++;
    *Tcg_TurnVar(TCG_DUELVARS_HAND + *n - 1) = deckIndex;
}

void Tcg_RemoveCardFromHand(u32 deckIndex)
{
    u8 *n = Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND);
    u8 *hand = Tcg_TurnVar(TCG_DUELVARS_HAND);
    u32 i, j, count = *n;
    for (i = j = 0; i < count; i++)
    {
        if (hand[i] == deckIndex)
            (*n)--;
        else
            hand[j++] = hand[i];
    }
}

void Tcg_PutCardInDiscardPile(u32 deckIndex)
{
    u8 *n = Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_DISCARD_PILE);
    *Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + deckIndex) = TCG_CARD_LOCATION_DISCARD_PILE;
    (*n)++;
    *Tcg_TurnVar(TCG_DUELVARS_DECK_CARDS + *n - 1) = deckIndex;
}

// (only if it's in the hand)
void Tcg_MoveHandCardToDiscardPile(u32 deckIndex)
{
    if ((*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + deckIndex) & ~TCG_CARD_LOCATION_JUST_DRAWN) != TCG_CARD_LOCATION_HAND)
        return;
    Tcg_RemoveCardFromHand(deckIndex);
    Tcg_PutCardInDiscardPile(deckIndex);
}

// a card taken out of the discard pile, its location CARD_LOCATION_JUST_DRAWN (Tcg_AddCardToHand next)
void Tcg_MoveDiscardPileCardToHand(u32 deckIndex)
{
    u8 *n = Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_DISCARD_PILE);
    u8 *pile = Tcg_TurnVar(TCG_DUELVARS_DECK_CARDS);
    u32 i, j, count = *n;
    *Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + deckIndex) |= TCG_CARD_LOCATION_JUST_DRAWN;
    if (count == 0)
        return;
    (*n)--;
    for (i = j = 0; i < count; i++)
        if (pile[i] != deckIndex)
            pile[j++] = pile[i];
}

// CheckPrizeTaken (the game's z flag: drawn): TRUE if the turn holder's prize (0-7) is drawn
bool32 Tcg_IsPrizeTaken(u32 prize)
{
    return !(*Tcg_TurnVar(TCG_DUELVARS_PRIZES) & (1 << prize));
}

// duelTempList: the turn holder's discard pile (newest first). TRUE: it's empty.
bool32 Tcg_CreateDiscardPileCardList(void)
{
    u32 n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_DISCARD_PILE), i;
    for (i = 0; i < n; i++)
        gTcg->duelTempList[i] = *Tcg_TurnVar(TCG_DUELVARS_DECK_CARDS + n - 1 - i);
    gTcg->duelTempList[n] = 0xFF;
    return n == 0;
}

// duelTempList: the turn holder's deck, top first. TRUE: it's empty.
bool32 Tcg_CreateDeckCardList(void)
{
    u32 notInDeck = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_NOT_IN_DECK), i, n;
    if (notInDeck >= TCG_DECK_SIZE)
    {
        gTcg->duelTempList[0] = 0xFF;
        return TRUE;
    }
    n = TCG_DECK_SIZE - notInDeck;
    for (i = 0; i < n; i++)
        gTcg->duelTempList[i] = *Tcg_TurnVar(TCG_DUELVARS_DECK_CARDS + notInDeck + i);
    gTcg->duelTempList[n] = 0xFF;
    return FALSE;
}

// duelTempList: the energy cards attached to the turn holder's Pokémon at `location` (PLAY_AREA_*).
// Returns how many.
u32 Tcg_CreateArenaOrBenchEnergyCardList(u32 location)
{
    u32 i, n = 0;
    for (i = 0; i < TCG_DECK_SIZE; i++)
    {
        if (*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + i) != TCG_PLAY_AREA(location))
            continue;
        Tcg_LoadCard2(i);
        if (gTcg->loadedCard2->type & (1 << TCG_TYPE_ENERGY_F))
            gTcg->duelTempList[n++] = i;
    }
    gTcg->duelTempList[n] = 0xFF;
    return n;
}

// duelTempList: the turn holder's hand (newest first; not a card just drawn). TRUE: no cards in hand.
bool32 Tcg_CreateHandCardList(void)
{
    u32 n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND), i, k = 0;
    for (i = 0; i < n; i++)
    {
        u32 card = *Tcg_TurnVar(TCG_DUELVARS_HAND + n - 1 - i);
        if (*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + card) & TCG_CARD_LOCATION_JUST_DRAWN)
            continue;
        gTcg->duelTempList[k++] = card;
    }
    gTcg->duelTempList[k] = 0xFF;
    return n == 0;
}

// (a $ff-terminated list of deck indexes: lowest card ID first)
static void SortCardsInListByID(u8 *list)
{
    for (; *list != 0xFF; list++)
    {
        u8 *lowest = list, *p, t;
        for (p = list + 1; *p != 0xFF; p++)
            if (Tcg_GetCardIdFromDeckIndex(*p) < Tcg_GetCardIdFromDeckIndex(*lowest))
                lowest = p;
        t = *list;
        *list = *lowest;
        *lowest = t;
    }
}

void Tcg_SortCardsInDuelTempListByID(void)
{
    SortCardsInListByID(gTcg->duelTempList);
}

// the turn holder's hand, sorted by card ID (the newest end of the hand gets the lowest IDs)
void Tcg_SortHandCardsByID(void)
{
    u32 n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND), i;
    u8 *hand = Tcg_TurnVar(TCG_DUELVARS_HAND);
    for (i = 0; i < n; i++)
        gTcg->duelTempList[i] = hand[n - 1 - i];
    gTcg->duelTempList[n] = 0xFF;
    Tcg_SortCardsInDuelTempListByID();
    for (i = 0; i < n; i++)
        hand[n - 1 - i] = gTcg->duelTempList[i];
}

// TRUE: no match
bool32 Tcg_RemoveCardFromDuelTempList(u32 deckIndex)
{
    u8 *src = gTcg->duelTempList, *dst = gTcg->duelTempList;
    u32 kept = 0;
    for (; *src != 0xFF; src++)
    {
        if (*src == deckIndex)
            continue;
        *dst++ = *src;
        kept++;
    }
    *dst = 0xFF;
    return kept == 0;   // (as in the game: "carry" when nothing's left)
}

u32 Tcg_CountCardsInDuelTempList(void)
{
    u32 n = 0;
    while (gTcg->duelTempList[n] != 0xFF)
        n++;
    return n;
}

// Can the turn holder's Pokémon at `location` evolve into the card `deckIndex`? TRUE: it can't
// (*playedThisTurn: because it was played, or evolved, this turn).
bool32 Tcg_CheckIfCanEvolveInto(u32 location, u32 deckIndex, bool32 *playedThisTurn)
{
    if (playedThisTurn)
        *playedThisTurn = FALSE;
    Tcg_LoadCard2(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + location));
    Tcg_LoadCard1(deckIndex);
    if (gTcg->loadedCard1->preEvoName != gTcg->loadedCard2->name)
        return TRUE;
    if (!(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_FLAGS + location) & TCG_CAN_EVOLVE_THIS_TURN))
    {
        if (playedThisTurn)
            *playedThisTurn = TRUE;
        return TRUE;
    }
    return FALSE;
}

// (Pokémon Breeder) Can the turn holder's Basic at `location` evolve straight into the Stage 2 card
// `deckIndex` this turn? TRUE: it can't.
bool32 Tcg_CheckIfCanEvolveInto_BasicToStage2(u32 location, u32 deckIndex)
{
    u32 i;
    const u8 *stage1Name = NULL;
    if (!(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_FLAGS + location) & TCG_CAN_EVOLVE_THIS_TURN))
        return TRUE;
    Tcg_LoadCard2(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + location));
    Tcg_LoadCard1(deckIndex);
    // (the Stage 1 card it evolves from: the one whose name is its pre-evolution's)
    for (i = 1; i < TCG_NUM_CARDS; i++)
    {
        if (gTcgCards[i].name == gTcg->loadedCard1->preEvoName)
        {
            stage1Name = gTcgCards[i].preEvoName;
            break;
        }
    }
    return stage1Name == NULL || stage1Name != gTcg->loadedCard2->name;
}

// Evolve the turn holder's Pokémon at tempPlayAreaLocation into the hand card tempCardIndex.
// TRUE: it can't.
bool32 Tcg_EvolvePokemonCardIfPossible(void)
{
    if (Tcg_CheckIfCanEvolveInto(gTcg->tempPlayAreaLocation, gTcg->tempCardIndex, NULL))
        return TRUE;
    Tcg_EvolvePokemonCard();
    return FALSE;
}

void Tcg_EvolvePokemonCard(void)
{
    u32 loc = gTcg->tempPlayAreaLocation;
    u8 *arena = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + loc);
    u8 *hp;
    gTcg->preEvolutionPokemonCard = *arena;
    Tcg_LoadCard2(*arena);
    *arena = gTcg->tempCardIndex;
    Tcg_LoadCard1(gTcg->tempCardIndex);
    Tcg_PutHandCardInPlayArea(gTcg->tempCardIndex, loc);
    // (its HP: the damage it had carries over)
    hp = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + loc);
    *hp = *hp + gTcg->loadedCard1->hp - gTcg->loadedCard2->hp;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_FLAGS + loc) = 0;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_CHANGED_TYPE + loc) = 0;
    if (loc == TCG_PLAY_AREA_ARENA)
        Tcg_ClearAllStatusConditions();
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STAGE + loc) = gTcg->loadedCard1->stage;
}

// the turn holder's Active Pokémon's status, substatuses and two-turn duel variables (not Headache)
void Tcg_ClearAllStatusConditions(void)
{
    u32 i;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS) = TCG_NO_STATUS;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS1) = 0;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS2) = 0;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_CHANGED_WEAKNESS) = 0;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_CHANGED_RESISTANCE) = 0;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS3) &= ~(1 << TCG_SUBSTATUS3_THIS_TURN_DOUBLE_DAMAGE_F);
    for (i = 0; i < 8; i++)
        *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_DISABLED_ATTACK_INDEX + i) = 0;
}

// A Pokémon from the hand into the Active spot or the first free Bench spot. TRUE: no room.
bool32 Tcg_PutHandPokemonCardInPlayArea(u32 deckIndex)
{
    u8 *n = Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    u32 loc = *n;
    if (loc >= TCG_MAX_PLAY_AREA)
        return TRUE;
    (*n)++;
    Tcg_PutHandCardInPlayArea(deckIndex, loc);
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + loc) = deckIndex;
    Tcg_LoadCard2(deckIndex);
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + loc) = gTcg->loadedCard2->hp;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_FLAGS + loc) = 0;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_CHANGED_TYPE + loc) = 0;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_ATTACHED_PLUSPOWER + loc) = 0;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_ATTACHED_DEFENDER + loc) = 0;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STAGE + loc) = gTcg->loadedCard2->stage;
    if (loc == TCG_PLAY_AREA_ARENA)
        Tcg_ClearAllStatusConditions();
    gTcg->tempPlayAreaLocation = loc;   // (the game returns it in a and e)
    return FALSE;
}

// a hand card (an Energy, a Trainer) put at a play area location (attached)
void Tcg_PutHandCardInPlayArea(u32 deckIndex, u32 location)
{
    Tcg_RemoveCardFromHand(deckIndex);
    *Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + deckIndex) = TCG_PLAY_AREA(location);
}

// the turn holder's Pokémon at `location`, and everything attached to it, to the discard pile
void Tcg_MovePlayAreaCardToDiscardPile(u32 location)
{
    u32 i;
    Tcg_EmptyPlayAreaSlot(location);
    (*Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA))--;
    for (i = 0; i < TCG_DECK_SIZE; i++)
        if (*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + i) == TCG_PLAY_AREA(location))
            Tcg_PutCardInDiscardPile(i);
}

void Tcg_EmptyPlayAreaSlot(u32 location)
{
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + location) = 0xFF;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + location) = 0;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STAGE + location) = 0;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_CHANGED_TYPE + location) = 0;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_ATTACHED_DEFENDER + location) = 0;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_ATTACHED_PLUSPOWER + location) = 0;
}

void Tcg_ShiftAllPokemonToFirstPlayAreaSlots(void)
{
    Tcg_ShiftTurnPokemonToFirstPlayAreaSlots();
    Tcg_SwapTurn();
    Tcg_ShiftTurnPokemonToFirstPlayAreaSlots();
    Tcg_SwapTurn();
}

void Tcg_ShiftTurnPokemonToFirstPlayAreaSlots(void)
{
    u32 d, e = TCG_PLAY_AREA_ARENA;
    for (d = TCG_PLAY_AREA_ARENA; d < TCG_MAX_PLAY_AREA; d++)
    {
        if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + d) & 0x80)
            continue;
        Tcg_SwapPlayAreaPokemon(d, e);
        e++;
    }
}

// the Active Pokémon (its statuses cleared first) and the one at `location` change places
void Tcg_SwapArenaWithBenchPokemon(u32 location)
{
    Tcg_ClearAllStatusConditions();
    Tcg_SwapPlayAreaPokemon(TCG_PLAY_AREA_ARENA, location);
}

void Tcg_SwapPlayAreaPokemon(u32 d, u32 e)
{
    static const u8 sVars[] = {TCG_DUELVARS_ARENA_CARD, TCG_DUELVARS_ARENA_CARD_HP, TCG_DUELVARS_ARENA_CARD_FLAGS,
                               TCG_DUELVARS_ARENA_CARD_STAGE, TCG_DUELVARS_ARENA_CARD_CHANGED_TYPE,
                               TCG_DUELVARS_ARENA_CARD_ATTACHED_PLUSPOWER, TCG_DUELVARS_ARENA_CARD_ATTACHED_DEFENDER};
    u32 i;
    if (d == e)
        return;
    for (i = 0; i < ARRAY_COUNT(sVars); i++)
    {
        u8 *a = Tcg_TurnVar(sVars[i] + e), *b = Tcg_TurnVar(sVars[i] + d), t = *a;
        *a = *b;
        *b = t;
    }
    for (i = 0; i < TCG_DECK_SIZE; i++)
    {
        u8 *loc = Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + i);
        if (*loc == TCG_PLAY_AREA(e))
            *loc = TCG_PLAY_AREA(d);
        else if (*loc == TCG_PLAY_AREA(d))
            *loc = TCG_PLAY_AREA(e);
    }
}
