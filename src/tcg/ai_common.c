// Pokémon Canvas: the Trading Card Game AI's common routines, ported from the Game Boy Color game's
// engine/duel/ai/common.asm (pret/poketcg) (TcgAi_ prefixed). Deck indexes "if found" are returned,
// or 0xFF.
#include "global.h"
#include "tcg.h"
#include "tcg_ai.h"

static inline u32 TurnCardId(u32 deckIndex)
{
    return Tcg_GetCardIdFromDeckIndex(deckIndex);
}

// (the Mewtwo Lv53 mill strategy: no Mewtwo here, so wAIBarrierFlagCounter is never set, and this
// returns the game's carry: go on as usual)
bool32 TcgAi_HandleAIAntiMewtwoDeckStrategy(void)
{
    return TRUE;
}

// the attached Energy card to discard from the turn holder's Pokémon at `location`: the first that's
// no use to it (else the first). 0xFF: none.
u32 TcgAi_PickEnergyCardToDiscard(u32 location)
{
    const u8 *c;
    gTcg->tempPlayAreaLocation = location;
    Tcg_CreateArenaOrBenchEnergyCardList(location);
    Tcg_GetPlayAreaCardAttachedEnergies(location);
    if (gTcg->totalAttachedEnergies == 0)
        return 0xFF;
    gTcgAi->tempCardId = TurnCardId(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + location));
    gTcgAi->tempCardType = Tcg_Card(gTcgAi->tempCardId)->type | TCG_TYPE_ENERGY;
    for (c = gTcg->duelTempList; *c != 0xFF; c++)
        if (!TcgAi_CheckIfEnergyIsUseful(*c))
            return *c;
    return gTcg->duelTempList[0];
}

// the attached Energy card to remove from the turn holder's Pokémon at `location` (an effect on the
// opponent's): a Double Colorless, else the first that's useful to it, else the first. 0xFF: none.
u32 TcgAi_PickAttachedEnergyCardToRemove(u32 location)
{
    const u8 *c;
    gTcg->tempPlayAreaLocation = location;
    Tcg_CreateArenaOrBenchEnergyCardList(location);
    Tcg_GetPlayAreaCardAttachedEnergies(location);
    if (gTcg->totalAttachedEnergies == 0)
        return 0xFF;
    gTcgAi->tempCardId = TurnCardId(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + location));
    gTcgAi->tempCardType = Tcg_Card(gTcgAi->tempCardId)->type | TCG_TYPE_ENERGY;
    for (c = gTcg->duelTempList; *c != 0xFF; c++)
        if (TurnCardId(*c) == TCG_CARD_DOUBLE_COLORLESS_ENERGY)
            return *c;
    for (c = gTcg->duelTempList; *c != 0xFF; c++)
        if (TcgAi_CheckIfEnergyIsUseful(*c))
            return *c;
    return gTcg->duelTempList[0];
}

// two attached Energy cards to remove from the Pokémon at hTempPlayAreaLocation_ff9d (wTempAI,
// wCurCardCanAttack): Double Colorless first, then useful ones, then any. FALSE: it has fewer than 2.
bool32 TcgAi_PickTwoAttachedEnergyCards(void)
{
    u32 location = gTcg->tempPlayAreaLocation;
    const u8 *c;
    Tcg_CreateArenaOrBenchEnergyCardList(location);
    if (TcgAi_CountNumberOfEnergyCardsAttached(location) < 2)
        return FALSE;
    gTcgAi->tempCardId = TurnCardId(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + location));
    gTcgAi->tempCardType = Tcg_Card(gTcgAi->tempCardId)->type | TCG_TYPE_ENERGY;
    gTcgAi->tempAI = 0xFF;
    gTcgAi->curCardCanAttack = 0xFF;
    for (c = gTcg->duelTempList; *c != 0xFF; c++)
    {
        if (TurnCardId(*c) != TCG_CARD_DOUBLE_COLORLESS_ENERGY)
            continue;
        if (gTcgAi->tempAI == 0xFF)
        {
            gTcgAi->tempAI = *c;
            continue;
        }
        gTcgAi->curCardCanAttack = *c;
        return TRUE;
    }
    for (c = gTcg->duelTempList; *c != 0xFF; c++)
    {
        if (!TcgAi_CheckIfEnergyIsUseful(*c))
            continue;
        if (gTcgAi->tempAI == 0xFF)
        {
            gTcgAi->tempAI = *c;
            continue;
        }
        gTcgAi->curCardCanAttack = *c;
        return TRUE;
    }
    c = gTcg->duelTempList;
    if (gTcgAi->tempAI == 0xFF)
    {
        gTcgAi->tempAI = c[0];
        gTcgAi->curCardCanAttack = c[1];
        return TRUE;
    }
    while (*c == gTcgAi->tempAI)
        c++;
    gTcgAi->curCardCanAttack = *c;
    return TRUE;
}

// the Energy cards in the turn holder's hand
u32 TcgAi_CountOppEnergyCardsInHand(void)
{
    if (TcgAi_CreateEnergyCardListFromHand())
        return 0;
    return Tcg_CountCardsInDuelTempList();
}

// LookForCardIDInDeck_GivenCardIDInHandAndPlayArea: `deckCard` in the deck, if `handPlayCard` is in the
// hand or play area and `deckCard` isn't (the deck index; 0xFF otherwise)
u32 TcgAi_LookForCardIDInDeck_GivenCardIDInHandAndPlayArea(u32 deckCard, u32 handPlayCard)
{
    gTcgAi->tempAI = handPlayCard;
    gTcgAi->curCardCanAttack = deckCard;
    gTcgAi->tempAIPokemonCard = TcgAi_LookForCardIDInLocation(TCG_CARD_LOCATION_DECK, deckCard);
    if (gTcgAi->tempAIPokemonCard == 0xFF)
        return 0xFF;
    if (TcgAi_LookForCardIDInHandAndPlayArea(gTcgAi->tempAI, gTcgAi->tempAI) == 0xFF)
        return 0xFF;
    if (TcgAi_LookForCardIDInHandAndPlayArea(gTcgAi->curCardCanAttack, gTcgAi->curCardCanAttack) != 0xFF)
        return 0xFF;
    return gTcgAi->tempAIPokemonCard;
}

// a card in the hand (its deck index) or play area (its location): not 0xFF if found
u32 TcgAi_LookForCardIDInHandAndPlayArea(u32 handCard, u32 playCard)
{
    u32 found = TcgAi_LookForCardIDInHandList(handCard);
    if (found != 0xFF)
        return found;
    return TcgAi_LookForCardIDInPlayArea(playCard, TCG_PLAY_AREA_ARENA);
}

// `deckCard` in the deck, if `handCard` is in the hand and `deckCard` isn't in the hand or play area
u32 TcgAi_LookForCardIDInDeck_GivenCardIDInHand(u32 deckCard, u32 handCard)
{
    gTcgAi->tempAI = handCard;
    gTcgAi->curCardCanAttack = deckCard;
    gTcgAi->tempAIPokemonCard = TcgAi_LookForCardIDInLocation(TCG_CARD_LOCATION_DECK, deckCard);
    if (gTcgAi->tempAIPokemonCard == 0xFF)
        return 0xFF;
    if (TcgAi_LookForCardIDInHandList(gTcgAi->tempAI) == 0xFF)
        return 0xFF;
    if (TcgAi_LookForCardIDInHandAndPlayArea(gTcgAi->curCardCanAttack, gTcgAi->curCardCanAttack) != 0xFF)
        return 0xFF;
    return gTcgAi->tempAIPokemonCard;
}

// for Pokémon Trader: `deckCard` from the deck (its deck index), for a hand Pokémon that isn't
// `excludeCard` (gTcgAi->wce1a... the game's e: the hand card's deck index in gTcgAi->tempAIPokemonCard).
// 0xFF: no trade.
u32 TcgAi_LookForCardIDToTradeWithDifferentHandCard(u32 deckCard, u32 excludeCard)
{
    const u8 *c;
    u32 deckIndex;
    gTcgAi->curCardCanAttack = excludeCard;
    gTcgAi->tempAI = deckCard;
    if (TcgAi_LookForCardIDInHandList(deckCard) != 0xFF)
        return 0xFF;
    deckIndex = TcgAi_LookForCardIDInLocation(TCG_CARD_LOCATION_DECK, gTcgAi->tempAI);
    if (deckIndex == 0xFF)
        return 0xFF;
    gTcgAi->tempAI = deckIndex;
    Tcg_CreateHandCardList();
    for (c = gTcg->duelTempList; *c != 0xFF; c++)
    {
        Tcg_LoadCard1(*c);
        if (gTcg->loadedCard1Id == gTcgAi->curCardCanAttack || gTcg->loadedCard1->type >= TCG_TYPE_ENERGY)
            continue;
        gTcgAi->tempAIPokemonCard = *c;
        return gTcgAi->tempAI;
    }
    return 0xFF;
}

// (despite its name: TRUE when the hand has at least two of `cardId`; the second's deck index in
// hTempCardIndex_ff98)
bool32 TcgAi_CheckIfHasCardIDInHand(u32 cardId)
{
    const u8 *c;
    bool32 one = FALSE;
    gTcgAi->tempCardIdToLook = cardId;
    Tcg_CreateHandCardList();
    for (c = gTcg->duelTempList; *c != 0xFF; c++)
    {
        gTcg->tempCardIndex = *c;
        Tcg_LoadCard1(*c);
        if (gTcg->loadedCard1Id != cardId)
            continue;
        if (one)
            return TRUE;
        one = TRUE;
    }
    return FALSE;
}

// the Pokémon in the turn holder's play area and hand
u32 TcgAi_CountPokemonCardsInHandAndInPlayArea(void)
{
    const u8 *c;
    gTcgAi->tempAI = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    Tcg_CreateHandCardList();
    for (c = gTcg->duelTempList; *c != 0xFF; c++)
        if (Tcg_Card(TurnCardId(*c))->type < TCG_TYPE_ENERGY)
            gTcgAi->tempAI++;
    return gTcgAi->tempAI;
}

// a Pokémon card the hand has two of (the deck index of the last such second card found; 0xFF: none)
u32 TcgAi_FindDuplicatePokemonCards(void)
{
    const u8 *a, *b;
    gTcgAi->tempAI = 0xFF;
    Tcg_CreateHandCardList();
    for (a = gTcg->duelTempList; *a != 0xFF; a++)
    {
        u32 id = TurnCardId(*a);
        for (b = a + 1; *b != 0xFF; b++)
        {
            if (TurnCardId(*b) != id)
                continue;
            if (Tcg_Card(id)->type < TCG_TYPE_ENERGY)
                gTcgAi->tempAI = *b;
            break;
        }
    }
    return gTcgAi->tempAI;
}

// TRUE: the attack the AI would use has HIGH_RECOIL
bool32 TcgAi_CheckIfAttackIsHighRecoil(void)
{
    if (!TcgAi_ProcessButDontUseAttack())
        return FALSE;
    Tcg_CopyAttackDataAndDamage_FromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD), gTcg->selectedAttack);
    return !Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG1_ADDRESS | TCG_HIGH_RECOIL_F);
}
