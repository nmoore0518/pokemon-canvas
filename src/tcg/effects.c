// Pokémon Canvas: the Trading Card Game's attack, Pokémon Power and Trainer effects, ported from the
// Game Boy Color game's engine/duel/effect_functions.asm (pret/poketcg), routine by routine, under the
// same names (Tcg_ prefixed): the ones the cards in our 8 decks use, and the helpers they call. Each
// replaces its weak stub in src/tcg/effects_stubs.c. As in the game, a routine's TRUE (the game's
// carry) means: can't be used (INITIAL_EFFECT_1), cancelled (the player's selections), and its text,
// gTcg->hlText. Link-duel calls (SerialSend, ExchangeRNG) are left out.
#include "global.h"
#include "tcg.h"
#include "tcg_ui.h"
#include "tcg_effect_functions.h"
#include "constants/tcg_text.h"

#define TCG_PLAY_AREA(loc)  (TCG_CARD_LOCATION_PLAY_AREA | (loc))

static inline u32 GetCardType(u32 cardId)
{
    return Tcg_Card(cardId)->type;
}

static inline u32 TurnDeckCardType(u32 deckIndex)
{
    return GetCardType(Tcg_GetCardIdFromDeckIndex(deckIndex));
}

static inline bool32 Fail(u16 text)
{
    gTcg->hlText = text;
    return TRUE;
}

// ATimes10 (8 bits, as the game's)
static inline u8 ATimes10(u32 a)
{
    return (u8)(a * 10);
}

static void AddToDamage(u32 a)
{
    gTcg->damage += (u8)a;
}

// ---- the status conditions ------------------------------------------------------------------

static bool32 SetNoEffectFromStatus(void)
{
    gTcg->effectFailed = TCG_EFFECT_FAILED_NO_EFFECT;
    return FALSE;
}

static bool32 SetWasUnsuccessful(void)
{
    gTcg->effectFailed = TCG_EFFECT_FAILED_UNSUCCESSFUL;
    return FALSE;
}

// the status `add` (keeping `keep` of the ones it has) queued for the defender (the non-turn holder's
// Active Pokémon). TRUE: queued. (Snorlax's Thick Skinned stops it; Clefairy Doll and Mysterious
// Fossil aren't in our decks.)
static bool32 QueueStatusCondition(u32 keep, u32 add)
{
    if (gTcg->whoseTurn == gTcg->whoseTurnReal && gTcg->tempNonTurnDuelistCardId == TCG_CARD_SNORLAX)
    {
        bool32 incapable;
        Tcg_SwapTurn();
        incapable = Tcg_CheckIsIncapableOfUsingPkmnPower(TCG_PLAY_AREA_ARENA);
        Tcg_SwapTurn();
        if (!incapable)
        {
            gTcg->noEffectFromWhichStatus = add;
            SetNoEffectFromStatus();
            return FALSE;
        }
    }
    if (gTcg->statusConditionQueueIndex + 3 >= sizeof(gTcg->statusConditionQueue))
        return FALSE;
    gTcg->statusConditionQueue[gTcg->statusConditionQueueIndex++] = TCG_QUEUE_DUELIST(gTcg->whoseTurn ^ 1);
    gTcg->statusConditionQueue[gTcg->statusConditionQueueIndex++] = keep;
    gTcg->statusConditionQueue[gTcg->statusConditionQueueIndex++] = add;
    return TRUE;
}

bool32 Tcg_PoisonEffect(void)
{
    return QueueStatusCondition(TCG_CNF_SLP_PRZ, TCG_POISONED);
}

static bool32 DoublePoisonEffect(void)
{
    return QueueStatusCondition(TCG_CNF_SLP_PRZ, TCG_DOUBLE_POISONED);
}

static bool32 ParalysisEffect(void)
{
    return QueueStatusCondition(TCG_PSN_DBLPSN, TCG_PARALYZED);
}

static bool32 ConfusionEffect(void)
{
    return QueueStatusCondition(TCG_PSN_DBLPSN, TCG_CONFUSED);
}

bool32 Tcg_SleepEffect(void)
{
    return QueueStatusCondition(TCG_PSN_DBLPSN, TCG_ASLEEP);
}

bool32 Tcg_Paralysis50PercentEffect(void)
{
    if (!Tcg_TossCoin(TCG_TEXT_ParalysisCheck))
        return FALSE;
    return ParalysisEffect();
}

bool32 Tcg_Confusion50PercentEffect(void)
{
    if (!Tcg_TossCoin(TCG_TEXT_ConfusionCheck))
        return FALSE;
    return ConfusionEffect();
}

// ---- helpers --------------------------------------------------------------------------------

static void ShuffleCardsInDeck(void)
{
    Tcg_PlayDeckShuffleAnimation();
    Tcg_ShuffleDeck();
}

// TRUE: the turn holder is the player (IsPlayerTurn's carry)
static bool32 IsPlayerTurn(void)
{
    return *Tcg_TurnVar(TCG_DUELVARS_DUELIST_TYPE) == TCG_DUELIST_TYPE_PLAYER;
}

// the AI's idea of the damage: wAIMinDamage = wDamage + min, wAIMaxDamage = wDamage + max, wDamage += a
// (not counting it, if the defender's poisoned: poison does the extra)
static bool32 UpdateExpectedAIDamage(u32 a, u32 min, u32 max)
{
    u8 damage = gTcg->damage;
    gTcg->aiMinDamage = damage + min;
    gTcg->aiMaxDamage = damage + max;
    gTcg->damage = (u8)(damage + a) | (gTcg->damage & 0xFF00);
    return FALSE;
}

static bool32 UpdateExpectedAIDamage_AccountForPoison(u32 a, u32 min, u32 max)
{
    if (*Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_STATUS) & (TCG_POISONED | TCG_DOUBLE_POISONED))
    {
        gTcg->aiMinDamage = gTcg->aiMaxDamage = gTcg->damage;
        return FALSE;
    }
    return UpdateExpectedAIDamage(a, min, max);
}

static bool32 SetExpectedAIDamage(u32 a, u32 min, u32 max)
{
    gTcg->damage = (u8)a;
    gTcg->aiMinDamage = min;
    gTcg->aiMaxDamage = max;
    return FALSE;
}

static void SetDefiniteDamage(u32 a)
{
    gTcg->damage = (u8)a;
    gTcg->aiMinDamage = gTcg->aiMaxDamage = (u8)a;
}

static bool32 SetDefiniteAIDamage(void)
{
    gTcg->aiMinDamage = gTcg->aiMaxDamage = gTcg->damage;
    return FALSE;
}

static void DrawPlayAreaScreenToShowChanges(u32 location)
{
    gTcg->tempPlayAreaLocation = location;
    Tcg_SetupPlayAreaScreen();
    Tcg_PrintPlayAreaCardList();
    Tcg_InitAndPrintPlayAreaCardInformationAndLocation_WithTextBox();
}

// `damage` to each of the turn holder's benched Pokémon
static void DealDamageToAllBenchedPokemon(u32 damage)
{
    u32 b, n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    for (b = 1; b < n; b++)
        Tcg_DealDamageToPlayAreaPokemon_RegularAnim(b, damage);
}

// a SUBSTATUS1 on the attacker (the turn holder's Active Pokémon)
static void ApplySubstatus1ToAttackingCard(u32 substatus)
{
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS1) = substatus;
}

// a SUBSTATUS2 on the defender, unless it's protected from effects
static void ApplySubstatus2ToDefendingCard(u32 substatus)
{
    if (Tcg_CheckNoDamageOrEffect())
    {
        Tcg_DrawDuelMainScene();
        if (gTcg->hlText)
            Tcg_DrawWideTextBox_PrintText(gTcg->hlText);
        return;
    }
    *Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS2) = substatus;
    *Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_SUBSTATUS2) = substatus;
}

// a random place in the turn holder's play area (the Active Pokémon too)
static u32 PickRandomPlayAreaCard(void)
{
    return Tcg_Random(*Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA));
}

// the next place in hTempList for a card picked (hCurSelectionItem counts them)
static u8 *GetNextPositionInTempList(void)
{
    return &gTcg->tempList[gTcg->curSelectionItem++ & 7];
}

// wDuelTempList: the turn holder's Energy cards of `type` attached to their Active Pokémon. Their number.
static u32 CreateListOfEnergyAttachedToArena(u32 type)
{
    u32 i, n = 0;
    for (i = 0; i < TCG_DECK_SIZE; i++)
        if (*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + i) == TCG_CARD_LOCATION_ARENA && TurnDeckCardType(i) == type)
            gTcg->duelTempList[n++] = i;
    gTcg->duelTempList[n] = 0xFF;
    return n;
}

// HandleSwitchDefendingPokemonEffect: the defender switched with the non-turn holder's bench
// Pokémon at `location` (0xFF: none)
static void HandleSwitchDefendingPokemonEffect(u32 location)
{
    if (location == 0xFF)
        return;
    if (*Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_HP) == 0)
        Tcg_HandleDestinyBondSubstatus();
    if (Tcg_CheckNoDamageOrEffect())
    {
        if (gTcg->hlText)
            Tcg_DrawWideTextBox_PrintText(gTcg->hlText);
        return;
    }
    Tcg_SwapTurn();
    Tcg_SwapArenaWithBenchPokemon(location);
    Tcg_SwapTurn();
    gTcg->duelDisplayedScreen = 0;
    gTcg->defendingWasForcedToSwitch = TRUE;
}

// TRUE: the defender's protected from effects (its text printed)
static bool32 HandleNoDamageOrEffect(void)
{
    if (!Tcg_CheckNoDamageOrEffect())
        return FALSE;
    if (gTcg->hlText)
        Tcg_DrawWideTextBox_PrintText(gTcg->hlText);
    return TRUE;
}

// the turn holder's Active Pokémon heals `amount` (not past its HP), with the animation
static void ApplyAndAnimateHPRecovery(u16 amount)
{
    u32 maxHp, hp, damage = Tcg_GetCardDamageAndMaxHP(TCG_PLAY_AREA_ARENA, NULL);
    u8 *hpVar;
    if (damage == 0)
        return;
    gTcg->loadedAttack.animation = TCG_ATK_ANIM_HEAL;
    // (the game's bug: h isn't set to hWhoseTurn, so the animation plays on whichever side h was)
    Tcg_PlayAttackAnimation(gTcg->whoseTurn, TCG_PLAY_AREA_ARENA, 1, amount < damage ? amount : damage);
    Tcg_GetCardDamageAndMaxHP(TCG_PLAY_AREA_ARENA, &maxHp);
    hpVar = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP);
    hp = *hpVar + amount;
    if (hp > maxHp)
        hp = maxHp;
    *hpVar = hp;
    Tcg_WaitAttackAnimation();
}

// TRUE: none of the turn holder's Pokémon in play has damage on it
static bool32 CheckIfPlayAreaHasAnyDamage(void)
{
    u32 e, n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    for (e = 0; e < n; e++)
        if (Tcg_GetCardDamageAndMaxHP(e, NULL))
            return FALSE;
    return TRUE;
}

// wDuelTempList: the Energy cards in the turn holder's discard pile (newest first); not Double
// Colorless if `onlyBasic`. TRUE: none.
static bool32 CreateEnergyCardListFromDiscardPile(bool32 onlyBasic)
{
    u32 i, n = 0, count = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_DISCARD_PILE);
    for (i = count; i > 0; i--)
    {
        u32 card = *Tcg_TurnVar(TCG_DUELVARS_DECK_CARDS + i - 1);
        u32 type;
        Tcg_LoadCard2(card);
        type = gTcg->loadedCard2->type;
        if (!(type & TCG_TYPE_ENERGY))
            continue;
        if (onlyBasic && type >= TCG_TYPE_ENERGY_DOUBLE_COLORLESS)
            continue;
        gTcg->duelTempList[n++] = card;
    }
    gTcg->duelTempList[n] = 0xFF;
    return n == 0;
}

// TRUE: the turn holder's deck is empty
static bool32 CheckIfDeckIsEmpty(void)
{
    gTcg->hlText = TCG_TEXT_NoCardsLeftInTheDeck;
    return *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_NOT_IN_DECK) >= TCG_DECK_SIZE;
}

// LookForCardsInDeck: the deck's cards (wDuelTempList) searched for what an effect wants (`search`,
// a SEARCHEFFECT_*; `cardId` for SEARCHEFFECT_CARD_ID). Found: `text`; not: "There is no <what> in
// the deck", and would the player like to look anyway? TRUE: they wouldn't.
static bool32 LookForCardsInDeck(u32 search, u32 cardId, u16 text, u16 what)
{
    const u8 *c;
    bool32 found = FALSE;
    for (c = gTcg->duelTempList; *c != 0xFF && !found; c++)
    {
        const struct TcgCard *card = Tcg_Card(Tcg_GetCardIdFromDeckIndex(*c));
        switch (search)
        {
        case TCG_SEARCHEFFECT_CARD_ID:
            found = Tcg_GetCardIdFromDeckIndex(*c) == cardId;
            break;
        case TCG_SEARCHEFFECT_BASIC_FIGHTING:
            found = card->type == TCG_TYPE_PKMN_FIGHTING && card->stage == TCG_BASIC;
            break;
        case TCG_SEARCHEFFECT_BASIC_ENERGY:
            found = card->type != TCG_TYPE_ENERGY_DOUBLE_COLORLESS && (card->type & TCG_TYPE_ENERGY);
            break;
        case TCG_SEARCHEFFECT_POKEMON:
            found = card->type < TCG_TYPE_ENERGY;
            break;
        }
    }
    if (found)
    {
        Tcg_DrawWideTextBox_WaitForInput(text);
        return FALSE;
    }
    Tcg_LoadTxRam2(gTcgTexts[what]);
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ThereIsNoInTheDeck);
    return Tcg_YesOrNoMenuWithText_SetCursorToYes(TCG_TEXT_WouldYouLikeToCheckTheDeck);
}

// "You can select <n> more cards. Quit?" TRUE: no
static bool32 AskWhetherToQuitSelectingCards(u32 total)
{
    Tcg_LoadTxRam3((u8)(total - gTcg->curSelectionItem));
    return Tcg_YesOrNoMenuWithText(TCG_TEXT_YouCanSelectMoreCardsQuit);
}

// the non-turn holder picks the bench Pokémon to switch in (hTempPlayAreaLocation_ff9d): the AI, or
// the player
static void DuelistSelectForcedSwitch(void)
{
    if (*Tcg_NonTurnVar(TCG_DUELVARS_DUELIST_TYPE) != TCG_DUELIST_TYPE_PLAYER)
    {
        Tcg_SwapTurn();
        gTcg->tempPlayAreaLocation = TcgAi_ForcedSwitch();
        Tcg_SwapTurn();
        // (the AI looked at other attacks: the one in use reloaded)
        Tcg_CopyAttackDataAndDamage_FromCardID(gTcg->playerAttackingCardId, gTcg->playerAttackingCardIndex, gTcg->playerAttackingAttackIndex);
        Tcg_UpdateArenaCardIDsAndClearTwoTurnDuelVars();
        return;
    }
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_SelectPkmnOnBenchToSwitchWithActive);
    Tcg_SwapTurn();
    do
        gTcg->tempPlayAreaLocation = Tcg_OpenPlayAreaScreenForSelection(TRUE);
    while (gTcg->tempPlayAreaLocation == TCG_SELECTION_CANCEL);
    Tcg_SwapTurn();
}

// the AI's pick of an Energy card attached to the defender to discard (0xFF: none)
static u32 AIPickEnergyCardToDiscardFromDefendingPokemon(void)
{
    u32 color, result;
    const u8 *c;
    Tcg_SwapTurn();
    Tcg_GetPlayAreaCardAttachedEnergies(TCG_PLAY_AREA_ARENA);
    if (Tcg_CreateArenaOrBenchEnergyCardList(TCG_PLAY_AREA_ARENA) == 0)
    {
        Tcg_SwapTurn();
        return 0xFF;
    }
    Tcg_LoadCard1(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
    color = TCG_COLORLESS;
    if (gTcg->attachedEnergies[TCG_COLORLESS] == 0)
    {
        color = gTcg->loadedCard1->type;
        if (color >= TCG_COLORLESS || gTcg->attachedEnergies[color] == 0)
            goto choose_random;
    }
    for (c = gTcg->duelTempList; *c != 0xFF; c++)
    {
        Tcg_LoadCard2(*c);
        if ((gTcg->loadedCard2->type & TCG_TYPE_PKMN) == color)
        {
            result = *c;
            Tcg_SwapTurn();
            return result;
        }
    }
choose_random:
    Tcg_ShuffleCards(gTcg->duelTempList, Tcg_CountCardsInDuelTempList());
    result = gTcg->duelTempList[0];
    Tcg_SwapTurn();
    return result;
}

// the AI's pick for Amnesia: the defender's second attack if it has the energy for it, else its first
// (unless that's a Pokémon Power)
static u32 AIPickAttackForAmnesia(void)
{
    u32 attack;
    Tcg_SwapTurn();
    Tcg_GetPlayAreaCardAttachedEnergies(TCG_PLAY_AREA_ARENA);
    Tcg_HandleEnergyBurn();
    Tcg_LoadCard2(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
    attack = 0;     // (the game's e: PLAY_AREA_ARENA, 0, when there's no first attack)
    if (gTcg->loadedCard2->attacks[0].name != NULL)
    {
        attack = 1;
        if (Tcg_CheckIfEnoughEnergiesForGivenAttack(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD), 1))
        {
            attack = 0;
            if (gTcg->loadedCard2->attacks[0].category == TCG_POKEMON_POWER)
                attack = 1;
        }
    }
    Tcg_SwapTurn();
    return attack;
}

// the non-turn holder's bench Pokémon with the least HP left (the last of those tied)
static u32 AIFindTargetForBenchAttack(void)
{
    u32 b, n, best = TCG_PLAY_AREA_ARENA, bestHp = 255;
    Tcg_SwapTurn();
    n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    for (b = 1; b < n; b++)
    {
        u32 hp = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + b);
        if (hp <= bestHp)
        {
            bestHp = hp;
            best = b;
        }
    }
    Tcg_SwapTurn();
    return best;
}

// "<Pokémon> healed <n> damage!", and the turn holder's Pokémon at hTempPlayAreaLocation_ff9d heals it
static void HealPlayAreaCardHP(u32 amount)
{
    Tcg_ResetAttackAnimationIsPlaying();
    gTcg->loadedAttack.animation = TCG_ATK_ANIM_HEALING_WIND_PLAY_AREA;
    Tcg_PlayAttackAnimation(gTcg->whoseTurn, gTcg->tempPlayAreaLocation, 1, amount);   // (the amount shows on the card)
    Tcg_WaitAttackAnimation();
    Tcg_LoadTxRam3(amount);
    gTcg->txRam2[0] = NULL;
    Tcg_LoadCard1(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcg->tempPlayAreaLocation));
    Tcg_CopyCardNameAndLevel();
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_PokemonHealedDamage);
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + gTcg->tempPlayAreaLocation) += amount;
}

static void PlayTrainerEffectAnimation(u32 anim)
{
    gTcg->loadedAttack.animation = anim;
    Tcg_ResetAttackAnimationIsPlaying();
    Tcg_PlayAttackAnimation(gTcg->whoseTurn, TCG_PLAY_AREA_ARENA, 0, 0);
    Tcg_WaitAttackAnimation();
}

// TRUE: no Energy attached to any of the turn holder's Pokémon
static bool32 CheckIfThereAreAnyEnergyCardsAttached(void)
{
    u32 i;
    for (i = 0; i < TCG_DECK_SIZE; i++)
    {
        u32 type;
        if (!(*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + i) & TCG_CARD_LOCATION_PLAY_AREA))
            continue;
        Tcg_LoadCard2(i);
        type = gTcg->loadedCard2->type;
        if (type != TCG_TYPE_TRAINER && type >= TCG_TYPE_ENERGY)
            return FALSE;
    }
    return TRUE;
}

// the player picks a Pokémon in play, then one of its Energy cards: hTemp_ffa0 (where),
// hTempPlayAreaLocation_ffa1 (the card). TRUE: cancelled.
static bool32 HandlePokemonAndEnergySelectionScreen(void)
{
    for (;;)
    {
        u32 loc = Tcg_OpenPlayAreaScreenForSelection(FALSE), card;
        if (loc == TCG_SELECTION_CANCEL)
            return TRUE;
        gTcg->tempPlayAreaLocation = loc;
        Tcg_GetPlayAreaCardAttachedEnergies(loc);
        if (gTcg->totalAttachedEnergies == 0)
        {
            Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_NoEnergyCards);
            continue;
        }
        Tcg_CreateArenaOrBenchEnergyCardList(loc);
        Tcg_DisplayEnergyDiscardScreen(loc);
        card = Tcg_HandleEnergyDiscardMenuInput();
        gTcg->temp = gTcg->tempPlayAreaLocation;
        gTcg->tempPlayAreaLocationFfa1 = gTcg->tempCardIndex;
        return card == TCG_SELECTION_CANCEL;
    }
}

// the player picks 2 hand cards (not the Trainer being played) into hTempList. TRUE: cancelled.
static bool32 HandlePlayerSelection2HandCards(u16 text, u16 infoText)
{
    Tcg_DrawWideTextBox_WaitForInput(text);
    Tcg_CreateHandCardList();
    Tcg_RemoveCardFromDuelTempList(gTcg->tempCardIndexFf9f);
    gTcg->curSelectionItem = 0;
    while (gTcg->curSelectionItem < 2)
    {
        u32 card;
        Tcg_InitAndDrawCardListScreenLayout(TRUE);
        Tcg_SetCardListInfoBoxText(infoText);
        card = Tcg_DisplayCardList();
        if (card == TCG_SELECTION_CANCEL)
            return TRUE;
        *GetNextPositionInTempList() = card;
        Tcg_RemoveCardFromDuelTempList(card);
    }
    return FALSE;
}

static bool32 HandlePlayerSelection2HandCardsToDiscard(void)
{
    return HandlePlayerSelection2HandCards(TCG_TEXT_Choose2CardsFromHandToDiscard, TCG_TEXT_ChooseTheCardToDiscard);
}

// the player picks an Energy card attached to the defender (hTemp_ffa0; 0xFF: it has none)
static bool32 HandleEnergyDiscardEffectSelection(void)
{
    Tcg_SwapTurn();
    if (Tcg_CreateArenaOrBenchEnergyCardList(TCG_PLAY_AREA_ARENA) == 0)
    {
        Tcg_SwapTurn();
        gTcg->temp = 0xFF;
        return FALSE;
    }
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ChooseDiscardEnergyCardFromOpponent);
    Tcg_DisplayEnergyDiscardScreen(TCG_PLAY_AREA_ARENA);
    while (Tcg_HandleEnergyDiscardMenuInput() == TCG_SELECTION_CANCEL)
        ;
    Tcg_SwapTurn();
    gTcg->temp = gTcg->tempCardIndex;
    return FALSE;
}

// the player picks a Fire Energy attached to their Active Pokémon to discard (hTemp_ffa0). TRUE: cancelled.
static bool32 PlayerPickFireEnergyCardToDiscard(void)
{
    u32 card;
    CreateListOfEnergyAttachedToArena(TCG_TYPE_ENERGY_FIRE);
    Tcg_DisplayEnergyDiscardScreen(TCG_PLAY_AREA_ARENA);
    card = Tcg_HandleEnergyDiscardMenuInput();
    gTcg->temp = gTcg->tempCardIndex;
    return card == TCG_SELECTION_CANCEL;
}

static bool32 AIPickFireEnergyCardToDiscard(void)
{
    CreateListOfEnergyAttachedToArena(TCG_TYPE_ENERGY_FIRE);
    gTcg->temp = gTcg->duelTempList[0];
    return FALSE;
}

static bool32 CheckFireEnergy(void)
{
    Tcg_GetPlayAreaCardAttachedEnergies(TCG_PLAY_AREA_ARENA);
    gTcg->hlText = TCG_TEXT_NotEnoughFireEnergy;
    return gTcg->attachedEnergies[TCG_FIRE] < 1;
}

static bool32 DiscardTemp(void)
{
    Tcg_PutCardInDiscardPile(gTcg->temp);
    return FALSE;
}

// a coin for "heads: no damage next turn" (Withdraw, Scrunch): the substatus and its animation
static bool32 NoDamageNextTurnIfHeads(u32 anim, u32 substatus)
{
    if (!Tcg_TossCoin(TCG_TEXT_IfHeadsNoDamageNextTurn))
        return SetWasUnsuccessful();
    gTcg->loadedAttack.animation = anim;
    ApplySubstatus1ToAttackingCard(substatus);
    return FALSE;
}

// `count` coins, `perHeads` damage each heads ("<n> damage x heads")
static bool32 CoinsTimesDamage(u32 count, u32 perHeads)
{
    u32 heads;
    Tcg_LoadTxRam3(perHeads);
    heads = Tcg_TossCoinATimes(count, TCG_TEXT_DamageCheckIfHeadsXDamage);
    SetDefiniteDamage(ATimes10(heads * (perHeads / 10)));
    return FALSE;
}

// a coin: heads, `bonus` more damage
static bool32 DamageBoostIfHeads(u32 bonus)
{
    Tcg_LoadTxRam3(bonus);
    if (Tcg_TossCoin(TCG_TEXT_DamageCheckIfHeadsPlusDamage))
        AddToDamage(bonus);
    return FALSE;
}

// draw a card (Fetch, Pay Day): the player sees it
static bool32 Draw1Card(void)
{
    u8 card;
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_Draw1CardFromTheDeck);
    Tcg_DisplayDrawOneCardScreen();
    if (Tcg_DrawCardFromDeck(&card))
        return TRUE;
    Tcg_AddCardToHand(card);
    Tcg_LoadCard1(card);
    if (gTcg->duelistType == TCG_DUELIST_TYPE_PLAYER)
        Tcg_OpenCardPage_FromHand();
    return FALSE;
}

// draw `n` cards
static void DrawNCards(u32 n)
{
    u8 card;
    Tcg_DisplayDrawNCardsScreen(n);
    while (n--)
    {
        if (Tcg_DrawCardFromDeck(&card))
            break;
        Tcg_AddCardToHand(card);
    }
}

// ---- Grass ----------------------------------------------------------------------------------

bool32 Tcg_IvysaurPoisonPowder_AIEffect(void)
{
    return UpdateExpectedAIDamage_AccountForPoison(10, 10, 10);
}

bool32 Tcg_GloomPoisonPowder_AIEffect(void)
{
    return UpdateExpectedAIDamage_AccountForPoison(10, 10, 10);
}

// the Defending Pokémon and the user are confused
bool32 Tcg_FoulOdorEffect(void)
{
    ConfusionEffect();
    Tcg_SwapTurn();
    ConfusionEffect();
    Tcg_SwapTurn();
    return FALSE;
}

bool32 Tcg_BulbasaurLeechSeedEffect(void)
{
    if (gTcg->dealtDamage == 0)
        return FALSE;
    ApplyAndAnimateHPRecovery(10);
    return FALSE;
}

bool32 Tcg_ExeggcuteLeechSeedEffect(void)
{
    if ((gTcg->dealtDamage & 0xFF) == 0)    // (the game checks only the low byte here)
        return FALSE;
    ApplyAndAnimateHPRecovery(10);
    return FALSE;
}

bool32 Tcg_Sprout_CheckDeckAndPlayArea(void)
{
    if (CheckIfDeckIsEmpty())
        return TRUE;
    gTcg->hlText = TCG_TEXT_NoSpaceOnTheBench;
    return *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) >= TCG_MAX_PLAY_AREA_POKEMON;
}

// TRUE: an Oddish is still in the turn holder's deck
static bool32 IsOddishInDeck(void)
{
    u32 i;
    for (i = 0; i < TCG_DECK_SIZE; i++)
        if (*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + i) == TCG_CARD_LOCATION_DECK && Tcg_GetCardIdFromDeckIndex(i) == TCG_CARD_ODDISH)
            return TRUE;
    return FALSE;
}

bool32 Tcg_Sprout_PlayerSelectEffect(void)
{
    gTcg->temp = 0xFF;
    Tcg_CreateDeckCardList();
    if (LookForCardsInDeck(TCG_SEARCHEFFECT_CARD_ID, TCG_CARD_ODDISH, TCG_TEXT_ChooseAnOddishFromDeck, TCG_TEXT_Oddish))
        return TRUE;
    Tcg_InitAndDrawCardListScreenLayout(TRUE);
    Tcg_SetCardListHeaderText(TCG_TEXT_ChooseAnOddish, TCG_TEXT_DuelistDeck);
    for (;;)
    {
        u32 card = Tcg_DisplayCardList();
        if (card != TCG_SELECTION_CANCEL)
        {
            if (Tcg_GetCardIdFromDeckIndex(card) == TCG_CARD_ODDISH)
            {
                gTcg->temp = card;
                return FALSE;
            }
        }
        else if (!IsOddishInDeck())
        {
            gTcg->temp = 0xFF;
            return FALSE;
        }
        Tcg_PlaySFX_InvalidChoice();
    }
}

bool32 Tcg_Sprout_AISelectEffect(void)
{
    const u8 *c;
    Tcg_CreateDeckCardList();
    for (c = gTcg->duelTempList;; c++)
    {
        gTcg->temp = *c;
        if (*c == 0xFF || Tcg_GetCardIdFromDeckIndex(*c) == TCG_CARD_ODDISH)
            return FALSE;
    }
}

bool32 Tcg_Sprout_PutInPlayAreaEffect(void)
{
    if (gTcg->temp != 0xFF)
    {
        Tcg_SearchCardInDeckAndAddToHand(gTcg->temp);
        Tcg_AddCardToHand(gTcg->temp);
        Tcg_PutHandPokemonCardInPlayArea(gTcg->temp);
        if (!IsPlayerTurn())
            Tcg_DisplayCardDetailScreen(gTcg->temp, TCG_TEXT_PlacedOnTheBench);
    }
    ShuffleCardsInDeck();
    return FALSE;
}

bool32 Tcg_Teleport_CheckBench(void)
{
    gTcg->hlText = TCG_TEXT_ThereAreNoPokemonOnBench;
    return *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) < 2;
}

bool32 Tcg_Teleport_PlayerSelectEffect(void)
{
    u32 loc;
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_SelectPkmnOnBenchToSwitchWithActive);
    do
        loc = Tcg_OpenPlayAreaScreenForSelection(TRUE);
    while (loc == TCG_SELECTION_CANCEL);
    gTcg->temp = loc;
    return FALSE;
}

// (the game's: a random place in play, which can be the Active Pokémon itself)
bool32 Tcg_Teleport_AISelectEffect(void)
{
    gTcg->temp = Tcg_Random(*Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA));
    return FALSE;
}

bool32 Tcg_Teleport_SwitchEffect(void)
{
    Tcg_SwapArenaWithBenchPokemon(gTcg->temp);
    gTcg->duelDisplayedScreen = 0;
    return FALSE;
}

static u16 SetDamageToATimes20(u32 a)
{
    gTcg->damage = a * 20;
    return gTcg->damage;
}

bool32 Tcg_BigEggsplosion_AIEffect(void)
{
    u16 max;
    Tcg_GetPlayAreaCardAttachedEnergies(gTcg->tempPlayAreaLocation);
    max = SetDamageToATimes20(gTcg->totalAttachedEnergies);
    if (max > 255)
        max = 255;
    gTcg->aiMaxDamage = max;
    gTcg->damage = (max & 0xFF) >> 1;
    gTcg->aiMinDamage = 0;
    return FALSE;
}

// coins for each Energy attached: 20 damage each heads
bool32 Tcg_BigEggsplosion_MultiplierEffect(void)
{
    Tcg_GetPlayAreaCardAttachedEnergies(TCG_PLAY_AREA_ARENA);
    Tcg_LoadTxRam3(20);
    SetDamageToATimes20(Tcg_TossCoinATimes(gTcg->totalAttachedEnergies, TCG_TEXT_DamageCheckIfHeadsXDamage));
    return FALSE;
}

// TRUE: Venusaur can't use Energy Trans (no Grass Energy in play, or asleep...)
bool32 Tcg_EnergyTrans_CheckPlayArea(void)
{
    u32 i;
    gTcg->temp = gTcg->tempPlayAreaLocation;
    if (Tcg_CheckIsIncapableOfUsingPkmnPower(gTcg->tempPlayAreaLocation))
        return TRUE;
    for (i = 0; i < TCG_DECK_SIZE; i++)
        if ((*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + i) & TCG_CARD_LOCATION_PLAY_AREA) && TurnDeckCardType(i) == TCG_TYPE_ENERGY_GRASS)
            return FALSE;
    return Fail(TCG_TEXT_NoGrassEnergy);
}

bool32 Tcg_EnergyTrans_PrintProcedure(void)
{
    Tcg_DrawWholeScreenTextBox(TCG_TEXT_ProcedureForEnergyTransfer);
    return FALSE;
}

// a Grass Energy attached to the turn holder's Pokémon at `location` (0xFF: none)
static u32 CheckIfCardHasGrassEnergyAttached(u32 location)
{
    u32 i;
    for (i = 0; i < TCG_DECK_SIZE; i++)
        if (*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + i) == TCG_PLAY_AREA(location) && TurnDeckCardType(i) == TCG_TYPE_ENERGY_GRASS)
            return i;
    return 0xFF;
}

// the player moves Grass Energy about their Pokémon as they like (B to stop)
bool32 Tcg_EnergyTrans_TransferEffect(void)
{
    if (*Tcg_TurnVar(TCG_DUELVARS_DUELIST_TYPE) != TCG_DUELIST_TYPE_PLAYER)
    {
        Tcg_SetupPlayAreaScreen();
        Tcg_PrintPlayAreaCardList();
        return FALSE;
    }
    gTcg->curSelectionItem = 0;
    Tcg_SetupPlayAreaScreen();
    for (;;)
    {
        u32 from, to, card;
        Tcg_PrintPlayAreaCardList();
        // taking a Grass Energy
        for (;;)
        {
            from = Tcg_HandlePlayAreaMenuInput(gTcg->curSelectionItem);
            if (from == TCG_SELECTION_CANCEL)
                return FALSE;
            gTcg->aiPkmnPowerEffectParam = from;
            gTcg->curSelectionItem = from;
            card = CheckIfCardHasGrassEnergyAttached(from);
            if (card != 0xFF)
                break;
            Tcg_PlaySFX_InvalidChoice();
        }
        gTcg->playAreaEffectTarget = card;      // (hAIEnergyTransEnergyCard)
        Tcg_AddCardToHand(card);                // (taken off to draw the play area without it)
        Tcg_PrintPlayAreaCardList();
        Tcg_PutHandCardInPlayArea(card, from);
        Tcg_DrawSymbolOnPlayAreaCursor(from, TCG_SYM_GRASS);
        // placing it
        to = Tcg_HandlePlayAreaMenuInput(gTcg->curSelectionItem);
        if (to != TCG_SELECTION_CANCEL)
        {
            gTcg->curSelectionItem = to;
            gTcg->aiEnergyTransPlayAreaLocation = to;
            Tcg_AddCardToHand(card);
            Tcg_PutHandCardInPlayArea(card, to);
        }
        Tcg_DrawSymbolOnPlayAreaCursor(from, TCG_SYM_SPACE);
        Tcg_EraseCursor();
    }
}

bool32 Tcg_EnergyTrans_AIEffect(void)
{
    Tcg_AddCardToHand(gTcg->playAreaEffectTarget);
    Tcg_PutHandCardInPlayArea(gTcg->playAreaEffectTarget, gTcg->aiEnergyTransPlayAreaLocation);
    Tcg_PrintPlayAreaCardList();
    return FALSE;
}

bool32 Tcg_Heal_OncePerTurnCheck(void)
{
    gTcg->temp = gTcg->tempPlayAreaLocation;
    if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_FLAGS + gTcg->tempPlayAreaLocation) & TCG_USED_PKMN_POWER_THIS_TURN)
        return Fail(TCG_TEXT_OnlyOncePerTurn);
    if (CheckIfPlayAreaHasAnyDamage())
        return Fail(TCG_TEXT_NoPokemonWithDamageCounters);
    return Tcg_CheckIsIncapableOfUsingPkmnPower(gTcg->tempPlayAreaLocation);
}

// Vileplume's Heal: a coin; heads, a damage counter off one of the turn holder's Pokémon
bool32 Tcg_Heal_RemoveDamageEffect(void)
{
    gTcg->aiPkmnPowerEffectParam = Tcg_TossCoin(TCG_TEXT_IfHeadsHealIsSuccessful);
    if (gTcg->aiPkmnPowerEffectParam && *Tcg_TurnVar(TCG_DUELVARS_DUELIST_TYPE) == TCG_DUELIST_TYPE_PLAYER)
    {
        Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ChoosePkmnToRemoveDamageCounter);
        for (;;)
        {
            u32 loc = Tcg_OpenPlayAreaScreenForSelection(FALSE);
            if (loc == TCG_SELECTION_CANCEL)
                continue;
            gTcg->playAreaEffectTarget = loc;
            if (Tcg_GetCardDamageAndMaxHP(loc, NULL))
                break;
        }
    }
    // (the Pokémon Power's used, whichever way the coin landed)
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_FLAGS + gTcg->temp) |= TCG_USED_PKMN_POWER_THIS_TURN;
    if (!gTcg->aiPkmnPowerEffectParam)
        return FALSE;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + gTcg->playAreaEffectTarget) += 10;
    DrawPlayAreaScreenToShowChanges(gTcg->playAreaEffectTarget);
    return FALSE;
}

bool32 Tcg_PetalDance_AIEffect(void)
{
    return SetExpectedAIDamage(120 / 2, 0, 120);
}

// 3 coins, 40 damage each heads; then the user's confused
bool32 Tcg_PetalDance_MultiplierEffect(void)
{
    CoinsTimesDamage(3, 40);
    Tcg_SwapTurn();
    ConfusionEffect();
    Tcg_SwapTurn();
    return FALSE;
}

// ---- Water ----------------------------------------------------------------------------------

bool32 Tcg_SquirtleWithdrawEffect(void)
{
    return NoDamageNextTurnIfHeads(TCG_ATK_ANIM_PROTECT, TCG_SUBSTATUS1_NO_DAMAGE_WITHDRAW);
}

bool32 Tcg_WartortleWithdrawEffect(void)
{
    return NoDamageNextTurnIfHeads(TCG_ATK_ANIM_PROTECT, TCG_SUBSTATUS1_NO_DAMAGE_WITHDRAW);
}

bool32 Tcg_RainDanceEffect(void)
{
    return TRUE;
}

// 10 more damage for each Water Energy beyond the attack's cost (up to 2): `water` of the cost is
// Water, `colorless` Colorless (paid with Water if that's all there is)
static bool32 ApplyExtraWaterEnergyDamageBonus(u32 water, u32 colorless)
{
    s32 extra;
    if (gTcg->metronomeEnergyCost)
    {
        colorless = gTcg->metronomeEnergyCost;
        water = 0;
    }
    Tcg_GetPlayAreaCardAttachedEnergies(gTcg->tempPlayAreaLocation);
    if (colorless && gTcg->totalAttachedEnergies == gTcg->attachedEnergies[TCG_WATER])
        water += colorless;
    extra = (s32)gTcg->attachedEnergies[TCG_WATER] - (s32)water;
    if (extra > 0)
    {
        if (extra >= 3)
            extra = 2;
        AddToDamage(ATimes10(extra));
    }
    gTcg->aiMinDamage = gTcg->aiMaxDamage = gTcg->damage;
    return FALSE;
}

bool32 Tcg_HydroPumpEffect(void)
{
    return ApplyExtraWaterEnergyDamageBonus(3, 0);
}

bool32 Tcg_SeadraWaterGunEffect(void)
{
    return ApplyExtraWaterEnergyDamageBonus(1, 1);
}

bool32 Tcg_LaprasWaterGunEffect(void)
{
    return ApplyExtraWaterEnergyDamageBonus(1, 0);
}

bool32 Tcg_HorseaSmokescreenEffect(void)
{
    ApplySubstatus2ToDefendingCard(TCG_SUBSTATUS2_SMOKESCREEN);
    return FALSE;
}

bool32 Tcg_SeadraAgilityEffect(void)
{
    if (!Tcg_TossCoin(TCG_TEXT_IfHeadsDoNotReceiveDamageOrEffect))
        return FALSE;
    gTcg->loadedAttack.animation = TCG_ATK_ANIM_AGILITY_PROTECT;
    ApplySubstatus1ToAttackingCard(TCG_SUBSTATUS1_AGILITY);
    return FALSE;
}

// Flail: 10 damage for each damage counter on the user
bool32 Tcg_MagikarpFlail_HPCheck(void)
{
    SetDefiniteDamage(Tcg_GetCardDamageAndMaxHP(TCG_PLAY_AREA_ARENA, NULL));
    return FALSE;
}

bool32 Tcg_MagikarpFlail_AIEffect(void)
{
    Tcg_MagikarpFlail_HPCheck();
    return SetDefiniteAIDamage();
}

// ---- Fire -----------------------------------------------------------------------------------

bool32 Tcg_Ember_CheckEnergy(void)                          { return CheckFireEnergy(); }
bool32 Tcg_Ember_PlayerSelectEffect(void)                   { return PlayerPickFireEnergyCardToDiscard(); }
bool32 Tcg_Ember_AISelectEffect(void)                       { return AIPickFireEnergyCardToDiscard(); }
bool32 Tcg_Ember_DiscardEffect(void)                        { return DiscardTemp(); }
bool32 Tcg_CharmeleonFlamethrower_CheckEnergy(void)         { return CheckFireEnergy(); }
bool32 Tcg_CharmeleonFlamethrower_PlayerSelectEffect(void)  { return PlayerPickFireEnergyCardToDiscard(); }
bool32 Tcg_CharmeleonFlamethrower_AISelectEffect(void)      { return AIPickFireEnergyCardToDiscard(); }
bool32 Tcg_CharmeleonFlamethrower_DiscardEffect(void)       { return DiscardTemp(); }
bool32 Tcg_ArcanineFlamethrower_CheckEnergy(void)           { return CheckFireEnergy(); }
bool32 Tcg_ArcanineFlamethrower_PlayerSelectEffect(void)    { return PlayerPickFireEnergyCardToDiscard(); }
bool32 Tcg_ArcanineFlamethrower_AISelectEffect(void)        { return AIPickFireEnergyCardToDiscard(); }
bool32 Tcg_ArcanineFlamethrower_DiscardEffect(void)         { return DiscardTemp(); }
bool32 Tcg_MagmarFlamethrower_CheckEnergy(void)             { return CheckFireEnergy(); }
bool32 Tcg_MagmarFlamethrower_PlayerSelectEffect(void)      { return PlayerPickFireEnergyCardToDiscard(); }
bool32 Tcg_MagmarFlamethrower_AISelectEffect(void)          { return AIPickFireEnergyCardToDiscard(); }
bool32 Tcg_MagmarFlamethrower_DiscardEffect(void)           { return DiscardTemp(); }

bool32 Tcg_EnergyBurnEffect(void)
{
    return TRUE;
}

bool32 Tcg_TakeDownEffect(void)
{
    Tcg_DealRecoilDamageToSelf(30);
    return FALSE;
}

// TRUE: fewer than 2 Energy cards attached to the Active Pokémon
bool32 Tcg_FireSpin_CheckEnergy(void)
{
    Tcg_CreateArenaOrBenchEnergyCardList(TCG_PLAY_AREA_ARENA);
    gTcg->hlText = TCG_TEXT_NotEnoughEnergyCards;
    return Tcg_CountCardsInDuelTempList() < 2;
}

bool32 Tcg_FireSpin_PlayerSelectEffect(void)
{
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ChooseAndDiscard2EnergyCards);
    gTcg->curSelectionItem = 0;
    Tcg_CreateArenaOrBenchEnergyCardList(TCG_PLAY_AREA_ARENA);
    Tcg_SortCardsInDuelTempListByID();
    Tcg_DisplayEnergyDiscardScreen(TCG_PLAY_AREA_ARENA);
    gTcg->energyDiscardMenuDenominator = 2;
    for (;;)
    {
        u32 card = Tcg_HandleEnergyDiscardMenuInput();
        if (card == TCG_SELECTION_CANCEL)
            return TRUE;
        *GetNextPositionInTempList() = card;
        gTcg->energyDiscardMenuNumerator++;
        if (gTcg->curSelectionItem >= 2)
            return FALSE;
        Tcg_RemoveCardFromDuelTempList(card);
        Tcg_DisplayEnergyDiscardMenu();
    }
}

bool32 Tcg_FireSpin_AISelectEffect(void)
{
    Tcg_CreateArenaOrBenchEnergyCardList(TCG_PLAY_AREA_ARENA);
    gTcg->tempList[0] = gTcg->duelTempList[0];
    gTcg->tempList[1] = gTcg->duelTempList[1];
    return FALSE;
}

bool32 Tcg_FireSpin_DiscardEffect(void)
{
    Tcg_PutCardInDiscardPile(gTcg->tempList[0]);
    Tcg_PutCardInDiscardPile(gTcg->tempList[1]);
    return FALSE;
}

// ---- Lightning ------------------------------------------------------------------------------

bool32 Tcg_Thunderpunch_AIEffect(void)
{
    return SetExpectedAIDamage((30 + 40) / 2, 30, 40);
}

// heads: 10 more damage; tails: 10 to the user (Thunderpunch_RecoilEffect)
bool32 Tcg_Thunderpunch_ModifierEffect(void)
{
    gTcg->temp = Tcg_TossCoin(TCG_TEXT_IfHeadPlus10IfTails10ToYourself);
    if (gTcg->temp)
        AddToDamage(10);
    return FALSE;
}

bool32 Tcg_Thunderpunch_RecoilEffect(void)
{
    if (!gTcg->temp)
        Tcg_DealRecoilDamageToSelf(10);
    return FALSE;
}

bool32 Tcg_MagnemiteSelfdestructEffect(void)
{
    Tcg_DealRecoilDamageToSelf(40);
    gTcg->isDamageToSelf = TRUE;
    DealDamageToAllBenchedPokemon(10);
    Tcg_SwapTurn();
    gTcg->isDamageToSelf = FALSE;
    DealDamageToAllBenchedPokemon(10);
    Tcg_SwapTurn();
    return FALSE;
}

bool32 Tcg_MagnetonLv28SelfdestructEffect(void)
{
    Tcg_DealRecoilDamageToSelf(80);
    gTcg->isDamageToSelf = TRUE;
    DealDamageToAllBenchedPokemon(20);
    Tcg_SwapTurn();
    gTcg->isDamageToSelf = FALSE;
    DealDamageToAllBenchedPokemon(20);
    Tcg_SwapTurn();
    return FALSE;
}

bool32 Tcg_ElectrodeSonicboom_UnaffectedByColorEffect(void)
{
    gTcg->damage |= 1 << (8 + TCG_UNAFFECTED_BY_WEAKNESS_RESISTANCE_F);
    return FALSE;
}

bool32 Tcg_ElectrodeSonicboom_NullEffect(void)
{
    return FALSE;
}

bool32 Tcg_EnergySpike_DeckCheck(void)
{
    return CheckIfDeckIsEmpty();
}

// TRUE: a Basic Energy is still in the turn holder's deck
static bool32 IsBasicEnergyInDeck(void)
{
    u32 i;
    for (i = 0; i < TCG_DECK_SIZE; i++)
    {
        u32 type;
        if (*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + i) != TCG_CARD_LOCATION_DECK)
            continue;
        type = TurnDeckCardType(i);
        if ((type & TCG_TYPE_ENERGY) && type < TCG_TYPE_ENERGY_DOUBLE_COLORLESS)
            return TRUE;
    }
    return FALSE;
}

bool32 Tcg_EnergySpike_PlayerSelectEffect(void)
{
    gTcg->temp = 0xFF;
    Tcg_CreateDeckCardList();
    if (LookForCardsInDeck(TCG_SEARCHEFFECT_BASIC_ENERGY, 0, TCG_TEXT_Choose1BasicEnergyCardFromDeck, TCG_TEXT_BasicEnergy))
        return TRUE;
    Tcg_InitAndDrawCardListScreenLayout(TRUE);
    Tcg_SetCardListHeaderText(TCG_TEXT_ChooseBasicEnergyCard, TCG_TEXT_DuelistDeck);
    for (;;)
    {
        u32 card = Tcg_DisplayCardList(), type, loc;
        if (card == TCG_SELECTION_CANCEL)
        {
            if (!IsBasicEnergyInDeck())
            {
                gTcg->temp = 0xFF;
                return FALSE;
            }
            Tcg_PlaySFX_InvalidChoice();
            continue;
        }
        type = TurnDeckCardType(card);
        if (type >= TCG_TYPE_ENERGY_DOUBLE_COLORLESS || !(type & TCG_TYPE_ENERGY))
            continue;
        gTcg->temp = gTcg->tempCardIndex;
        Tcg_EmptyScreen();
        Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ChoosePokemonToAttachEnergyCard);
        do
            loc = Tcg_OpenPlayAreaScreenForSelection(FALSE);
        while (loc == TCG_SELECTION_CANCEL);
        gTcg->tempPlayAreaLocationFfa1 = loc;
        return FALSE;
    }
}

// (the AI's choice is made in AISelectSpecialAttackParameters)
bool32 Tcg_EnergySpike_AISelectEffect(void)
{
    gTcg->temp = 0xFF;
    return FALSE;
}

bool32 Tcg_EnergySpike_AttachEnergyEffect(void)
{
    if (gTcg->temp != 0xFF)
    {
        Tcg_SearchCardInDeckAndAddToHand(gTcg->temp);
        Tcg_AddCardToHand(gTcg->temp);
        Tcg_PutHandCardInPlayArea(gTcg->temp, gTcg->tempPlayAreaLocationFfa1);
        if (!IsPlayerTurn())
        {
            Tcg_LoadCard1(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcg->tempPlayAreaLocationFfa1));
            gTcg->txRam2[1] = gTcg->loadedCard1->name;
            Tcg_DisplayCardDetailScreen(gTcg->temp, TCG_TEXT_AttachedEnergyToPokemon);
        }
    }
    ShuffleCardsInDeck();
    return FALSE;
}

// ---- Psychic --------------------------------------------------------------------------------

bool32 Tcg_InvisibleWallEffect(void)
{
    return TRUE;
}

// Meditate: 10 more damage for each damage counter on the defender
bool32 Tcg_MrMimeMeditate_DamageBoostEffect(void)
{
    u32 damage;
    Tcg_SwapTurn();
    damage = Tcg_GetCardDamageAndMaxHP(TCG_PLAY_AREA_ARENA, NULL);
    Tcg_SwapTurn();
    AddToDamage(damage);
    return FALSE;
}

bool32 Tcg_MrMimeMeditate_AIEffect(void)
{
    Tcg_MrMimeMeditate_DamageBoostEffect();
    return SetDefiniteAIDamage();
}

bool32 Tcg_DamageSwap_CheckDamage(void)
{
    gTcg->temp = gTcg->tempPlayAreaLocation;
    if (CheckIfPlayAreaHasAnyDamage())
        return Fail(TCG_TEXT_NoPokemonWithDamageCounters);
    return Tcg_CheckIsIncapableOfUsingPkmnPower(gTcg->tempPlayAreaLocation);
}

// a damage counter from hTempPlayAreaLocation_ffa1 to hPlayAreaEffectTarget, unless it would knock
// it out. TRUE: it would.
static bool32 TryGiveDamageCounter_DamageSwap(void)
{
    u8 *hp = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + gTcg->playAreaEffectTarget);
    if ((u8)(*hp - 10) == 0)
        return TRUE;
    *hp -= 10;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + gTcg->tempPlayAreaLocationFfa1) += 10;
    return FALSE;
}

// Alakazam's Damage Swap: the player moves damage counters about as they like (B to stop)
bool32 Tcg_DamageSwap_SelectAndSwapEffect(void)
{
    if (*Tcg_TurnVar(TCG_DUELVARS_DUELIST_TYPE) != TCG_DUELIST_TYPE_PLAYER)
    {
        Tcg_SetupPlayAreaScreen();
        Tcg_PrintPlayAreaCardList();
        return FALSE;
    }
    Tcg_DrawWholeScreenTextBox(TCG_TEXT_ProcedureForDamageSwap);
    gTcg->curSelectionItem = 0;
    Tcg_SetupPlayAreaScreen();
    for (;;)
    {
        u32 from, to;
        u8 *hp, oldHp;
        Tcg_PrintPlayAreaCardList();
        for (;;)
        {
            from = Tcg_HandlePlayAreaMenuInput(gTcg->curSelectionItem);
            if (from == TCG_SELECTION_CANCEL)
                return FALSE;
            gTcg->tempPlayAreaLocationFfa1 = from;
            gTcg->curSelectionItem = from;
            if (Tcg_GetCardDamageAndMaxHP(from, NULL))
                break;
            Tcg_PlaySFX_InvalidChoice();
        }
        // (the counter taken off to draw the play area without it)
        hp = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + from);
        oldHp = *hp;
        *hp += 10;
        Tcg_PrintPlayAreaCardList();
        *hp = oldHp;
        Tcg_DrawSymbolOnPlayAreaCursor(from, TCG_SYM_HP_NOK);
        for (;;)
        {
            to = Tcg_HandlePlayAreaMenuInput(gTcg->curSelectionItem);
            if (to == TCG_SELECTION_CANCEL)
                break;
            gTcg->playAreaEffectTarget = to;
            gTcg->curSelectionItem = to;
            if (!TryGiveDamageCounter_DamageSwap())
                break;
        }
        Tcg_DrawSymbolOnPlayAreaCursor(from, TCG_SYM_SPACE);
        Tcg_EraseCursor();
    }
}

bool32 Tcg_DamageSwap_SwapEffect(void)
{
    if (TryGiveDamageCounter_DamageSwap())
        return TRUE;
    Tcg_PrintPlayAreaCardList();
    return FALSE;
}

// TRUE: Kadabra can't use Recover (no Psychic Energy, or no damage)
bool32 Tcg_KadabraRecover_CheckEnergyHP(void)
{
    Tcg_GetPlayAreaCardAttachedEnergies(TCG_PLAY_AREA_ARENA);
    if (gTcg->attachedEnergies[TCG_PSYCHIC] < 1)
        return Fail(TCG_TEXT_NotEnoughPsychicEnergy);
    gTcg->hlText = TCG_TEXT_NoDamageCounters;
    return Tcg_GetCardDamageAndMaxHP(TCG_PLAY_AREA_ARENA, NULL) < 10;
}

bool32 Tcg_KadabraRecover_PlayerSelectEffect(void)
{
    CreateListOfEnergyAttachedToArena(TCG_TYPE_ENERGY_PSYCHIC);
    Tcg_DisplayEnergyDiscardScreen(TCG_PLAY_AREA_ARENA);
    if (Tcg_HandleEnergyDiscardMenuInput() == TCG_SELECTION_CANCEL)
        return TRUE;
    gTcg->temp = gTcg->tempCardIndex;
    return FALSE;
}

bool32 Tcg_KadabraRecover_AISelectEffect(void)
{
    CreateListOfEnergyAttachedToArena(TCG_TYPE_ENERGY_PSYCHIC);
    gTcg->temp = gTcg->duelTempList[0];
    return FALSE;
}

bool32 Tcg_KadabraRecover_DiscardEffect(void)
{
    return DiscardTemp();
}

bool32 Tcg_KadabraRecover_HealEffect(void)
{
    ApplyAndAnimateHPRecovery(Tcg_GetCardDamageAndMaxHP(TCG_PLAY_AREA_ARENA, NULL));
    return FALSE;
}

// ---- Fighting -------------------------------------------------------------------------------

bool32 Tcg_PrimeapeFurySwipes_AIEffect(void)
{
    return SetExpectedAIDamage(60 / 2, 0, 60);
}

bool32 Tcg_PrimeapeFurySwipes_MultiplierEffect(void)
{
    return CoinsTimesDamage(3, 20);
}

// a coin: tails, the user's confused
bool32 Tcg_TantrumEffect(void)
{
    if (Tcg_TossCoin(TCG_TEXT_IfTailsYourPokemonBecomesConfused))
        return TRUE;
    gTcg->loadedAttack.animation = TCG_ATK_ANIM_MULTIPLE_SLASH;
    Tcg_SwapTurn();
    ConfusionEffect();
    Tcg_SwapTurn();
    return FALSE;
}

bool32 Tcg_StrikesBackEffect(void)
{
    return TRUE;
}

// Karate Chop: 10 less damage for each damage counter on the user (not below 0)
bool32 Tcg_KarateChop_DamageSubtractionEffect(void)
{
    gTcg->damage -= Tcg_GetCardDamageAndMaxHP(TCG_PLAY_AREA_ARENA, NULL);
    if (gTcg->damage & 0x8000)
        SetDefiniteDamage(0);
    return FALSE;
}

bool32 Tcg_KarateChop_AIEffect(void)
{
    Tcg_KarateChop_DamageSubtractionEffect();
    return SetDefiniteAIDamage();
}

bool32 Tcg_SubmissionEffect(void)
{
    Tcg_DealRecoilDamageToSelf(20);
    return FALSE;
}

bool32 Tcg_StretchKick_CheckBench(void)
{
    gTcg->hlText = TCG_TEXT_EffectNoPokemonOnTheBench;
    return *Tcg_NonTurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) < 2;
}

bool32 Tcg_StretchKick_PlayerSelectEffect(void)
{
    u32 loc;
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ChoosePkmnInTheBenchToGiveDamage);
    Tcg_SwapTurn();
    do
        loc = Tcg_OpenPlayAreaScreenForSelection(TRUE);
    while (loc == TCG_SELECTION_CANCEL);
    gTcg->temp = loc;
    Tcg_SwapTurn();
    return FALSE;
}

bool32 Tcg_StretchKick_AISelectEffect(void)
{
    gTcg->temp = AIFindTargetForBenchAttack();
    return FALSE;
}

bool32 Tcg_StretchKick_BenchDamageEffect(void)
{
    Tcg_SwapTurn();
    Tcg_DealDamageToPlayAreaPokemon_RegularAnim(gTcg->temp, 20);
    Tcg_SwapTurn();
    return FALSE;
}

// ---- Colorless ------------------------------------------------------------------------------

bool32 Tcg_Peek_OncePerTurnCheck(void)
{
    gTcg->temp = gTcg->tempPlayAreaLocation;
    if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_FLAGS + gTcg->tempPlayAreaLocation) & TCG_USED_PKMN_POWER_THIS_TURN)
        return Fail(TCG_TEXT_OnlyOncePerTurn);
    return Tcg_CheckIsIncapableOfUsingPkmnPower(gTcg->tempPlayAreaLocation);
}

// Peek: a look at one of the opponent's prizes, their top card, or a random card in their hand
bool32 Tcg_Peek_SelectEffect(void)
{
    u32 param;
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_FLAGS + gTcg->temp) |= TCG_USED_PKMN_POWER_THIS_TURN;
    if (*Tcg_TurnVar(TCG_DUELVARS_DUELIST_TYPE) == TCG_DUELIST_TYPE_PLAYER)
    {
        Tcg_FinishQueuedAnimations();
        gTcg->aiPkmnPowerEffectParam = Tcg_HandlePeekSelection();
        return FALSE;
    }
    // the AI's choice (made by the AI: hAIPkmnPowerEffectParam)
    param = gTcg->aiPkmnPowerEffectParam;
    if ((param & TCG_AI_PEEK_TARGET_HAND) && (param & ~TCG_AI_PEEK_TARGET_HAND & 0xFF) < 0x40)
    {
        Tcg_SwapTurn();
        Tcg_DisplayCardDetailScreen(param & ~TCG_AI_PEEK_TARGET_HAND & 0xFF, TCG_TEXT_PeekWasUsedToLookInYourHand);
        Tcg_SwapTurn();
        return FALSE;
    }
    Tcg_FinishQueuedAnimations();
    Tcg_SwapTurn();
    Tcg_DrawAIPeekScreen(param ^ 0x80);
    Tcg_SwapTurn();
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_CardPeekWasUsedOn);
    return FALSE;
}

static bool32 WhirlwindSelect(void)
{
    if (*Tcg_NonTurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) < 2)
    {
        gTcg->temp = 0xFF;
        return FALSE;
    }
    DuelistSelectForcedSwitch();
    gTcg->temp = gTcg->tempPlayAreaLocation;
    return FALSE;
}

bool32 Tcg_PidgeyWhirlwind_SelectEffect(void)       { return WhirlwindSelect(); }
bool32 Tcg_PidgeottoWhirlwind_SelectEffect(void)    { return WhirlwindSelect(); }

bool32 Tcg_PidgeyWhirlwind_SwitchEffect(void)
{
    HandleSwitchDefendingPokemonEffect(gTcg->temp);
    return FALSE;
}

bool32 Tcg_PidgeottoWhirlwind_SwitchEffect(void)
{
    HandleSwitchDefendingPokemonEffect(gTcg->temp);
    return FALSE;
}

// -- Mirror Move: the attack used on the user last turn, back at the attacker

static bool32 MirrorMove_AIEffect(void)
{
    gTcg->aiMinDamage = gTcg->aiMaxDamage = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_DAMAGE);
    return FALSE;
}

static bool32 MirrorMove_InitialEffect1(void)
{
    if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_DAMAGE) | *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_DAMAGE + 1)
     | *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_STATUS))
        return FALSE;
    if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_SUBSTATUS2))
        return FALSE;
    return Fail(TCG_TEXT_YouDidNotReceiveAnAttackToMirrorMove);
}

// the player picks the defender's attack it won't be able to use (hTemp_ffa0)
static bool32 PlayerPickAttackForAmnesia(void)
{
    u32 deckIndex, attack;
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ChooseAttackOpponentWillNotBeAbleToUse);
    Tcg_HandleDefendingPokemonAttackSelection(&deckIndex, &attack);
    gTcg->temp = attack;
    return FALSE;
}

static bool32 MirrorMove_InitialEffect2(void)
{
    u32 effect;
    gTcg->temp = 0xFF;
    effect = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_EFFECT);
    if (effect == TCG_LAST_TURN_EFFECT_AMNESIA)
        return PlayerPickAttackForAmnesia();
    return FALSE;
}

static bool32 MirrorMove_PlayerSelection(void)
{
    if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_EFFECT) == TCG_LAST_TURN_EFFECT_DISCARD_ENERGY)
        return HandleEnergyDiscardEffectSelection();
    return FALSE;
}

static bool32 MirrorMove_AISelection(void)
{
    gTcg->temp = 0xFF;
    switch (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_EFFECT))
    {
    case TCG_LAST_TURN_EFFECT_DISCARD_ENERGY:
        gTcg->temp = AIPickEnergyCardToDiscardFromDefendingPokemon();
        break;
    case TCG_LAST_TURN_EFFECT_AMNESIA:
        gTcg->temp = AIPickAttackForAmnesia();
        break;
    }
    return FALSE;
}

// Amnesia on the defender's attack hTemp_ffa0
static void ApplyAmnesiaToAttack(void)
{
    ApplySubstatus2ToDefendingCard(TCG_SUBSTATUS2_AMNESIA);
    if (gTcg->noDamageOrEffect)
        return;
    *Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_DISABLED_ATTACK_INDEX) = gTcg->temp;
    *Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_EFFECT) = TCG_LAST_TURN_EFFECT_AMNESIA;
    if (IsPlayerTurn())
        return;
    Tcg_SwapTurn();
    Tcg_LoadCard1(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
    Tcg_LoadTxRam2(gTcg->loadedCard1->attacks[gTcg->temp ? 1 : 0].name);
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_WasChosenForTheEffectOfAmnesia);
    Tcg_SwapTurn();
}

static void MirrorMove_ExecuteStatusEffect(u32 status)
{
    u32 poison = status & TCG_PSN_DBLPSN;
    if (poison == TCG_DOUBLE_POISONED)
        DoublePoisonEffect();
    if (poison == TCG_POISONED)
        Tcg_PoisonEffect();
    switch (status & TCG_CNF_SLP_PRZ)
    {
    case TCG_CONFUSED:
        ConfusionEffect();
        break;
    case TCG_ASLEEP:
        Tcg_SleepEffect();
        break;
    case TCG_PARALYZED:
        ParalysisEffect();
        break;
    }
}

static bool32 MirrorMove_BeforeDamage(void)
{
    u32 status;
    if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_EFFECT) == TCG_LAST_TURN_EFFECT_AMNESIA)
    {
        ApplyAmnesiaToAttack();
        return FALSE;
    }
    gTcg->damage = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_DAMAGE)
                 | (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_DAMAGE + 1) << 8);
    if (gTcg->damage)
        gTcg->loadedAttack.animation = TCG_ATK_ANIM_HIT;
    status = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_STATUS);
    if (status)
        MirrorMove_ExecuteStatusEffect(status);
    *Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS2) = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_SUBSTATUS2);
    return FALSE;
}

static bool32 MirrorMove_AfterDamage(void)
{
    static const u16 sColorToTextSymbol[] =
    {
        TCG_TEXT_FireSymbol, TCG_TEXT_GrassSymbol, TCG_TEXT_LightningSymbol,
        TCG_TEXT_WaterSymbol, TCG_TEXT_FightingSymbol, TCG_TEXT_PsychicSymbol,
    };
    u32 weakness, color;
    if (gTcg->noDamageOrEffect)
        return FALSE;
    if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_EFFECT) == TCG_LAST_TURN_EFFECT_DISCARD_ENERGY)
    {
        Tcg_SwapTurn();
        Tcg_PutCardInDiscardPile(gTcg->temp);
        *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_EFFECT) = TCG_LAST_TURN_EFFECT_DISCARD_ENERGY;
        Tcg_SwapTurn();
    }
    // .change_weakness (Conversion 1's: none of our cards uses it, but here it is)
    weakness = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_CHANGE_WEAK);
    if (weakness == 0)
        return FALSE;
    Tcg_SwapTurn();
    Tcg_LoadCard2(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
    Tcg_SwapTurn();
    if (gTcg->loadedCard2->weakness == 0)
        return FALSE;
    *Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_CHANGED_WEAKNESS) = weakness;
    for (color = 0; color < 8 && !((weakness << color) & 0x80); color++)
        ;
    Tcg_SwapTurn();
    Tcg_LoadCard1(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
    gTcg->txRam2[0] = gTcg->loadedCard1->name;
    gTcg->txRam2[1] = gTcgTexts[sColorToTextSymbol[color % ARRAY_COUNT(sColorToTextSymbol)]];
    Tcg_DrawWideTextBox_PrintText(TCG_TEXT_ChangedTheWeaknessOfPokemonToColor);
    Tcg_SwapTurn();
    return FALSE;
}

bool32 Tcg_PidgeottoMirrorMove_AIEffect(void)          { return MirrorMove_AIEffect(); }
bool32 Tcg_PidgeottoMirrorMove_InitialEffect1(void)    { return MirrorMove_InitialEffect1(); }
bool32 Tcg_PidgeottoMirrorMove_InitialEffect2(void)    { return MirrorMove_InitialEffect2(); }
bool32 Tcg_PidgeottoMirrorMove_PlayerSelection(void)   { return MirrorMove_PlayerSelection(); }
bool32 Tcg_PidgeottoMirrorMove_AISelection(void)       { return MirrorMove_AISelection(); }
bool32 Tcg_PidgeottoMirrorMove_BeforeDamage(void)      { return MirrorMove_BeforeDamage(); }
bool32 Tcg_PidgeottoMirrorMove_AfterDamage(void)       { return MirrorMove_AfterDamage(); }

// 30 damage to one of the opponent's Pokémon, at random
bool32 Tcg_SlicingWindEffect(void)
{
    Tcg_SwapTurn();
    Tcg_DealDamageToPlayAreaPokemon_RegularAnim(PickRandomPlayAreaCard(), 30);
    Tcg_SwapTurn();
    return FALSE;
}

bool32 Tcg_Gale_LoadAnimation(void)
{
    gTcg->loadedAttack.animation = TCG_ATK_ANIM_GALE;
    return FALSE;
}

// the turn holder's Active Pokémon switched with a random benched one. TRUE: they have none.
static bool32 SwitchWithRandomBenchPokemon(void)
{
    u32 n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    if (n < 2)
        return TRUE;
    Tcg_SwapArenaWithBenchPokemon(Tcg_Random(n - 1) + 1);
    gTcg->duelDisplayedScreen = 0;
    return FALSE;
}

// Gale: the defender (unless protected) and the user both switched with random benched Pokémon
bool32 Tcg_Gale_SwitchEffect(void)
{
    if (!HandleNoDamageOrEffect())
    {
        if (*Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_HP) == 0)
            Tcg_HandleDestinyBondSubstatus();
        Tcg_SwapTurn();
        if (!SwitchWithRandomBenchPokemon())
            gTcg->dealtDamage = 0;
        Tcg_SwapTurn();
    }
    SwitchWithRandomBenchPokemon();
    return FALSE;
}

// half the defender's remaining HP, rounded up to a multiple of 10
bool32 Tcg_SuperFang_HalfHPEffect(void)
{
    u32 a = *Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_HP) >> 1;
    if (a & 1)
        a += 5;
    SetDefiniteDamage(a);
    return FALSE;
}

bool32 Tcg_SuperFang_AIEffect(void)
{
    Tcg_SuperFang_HalfHPEffect();
    return SetDefiniteAIDamage();
}

bool32 Tcg_FirstAid_DamageCheck(void)
{
    gTcg->hlText = TCG_TEXT_NoDamageCounters;
    return Tcg_GetCardDamageAndMaxHP(TCG_PLAY_AREA_ARENA, NULL) < 10;
}

bool32 Tcg_FirstAid_HealEffect(void)
{
    ApplyAndAnimateHPRecovery(10);
    return FALSE;
}

bool32 Tcg_JigglypuffDoubleEdgeEffect(void)
{
    Tcg_DealRecoilDamageToSelf(20);
    return FALSE;
}

// 10 more damage for each of the user's benched Pokémon
bool32 Tcg_DoTheWaveEffect(void)
{
    AddToDamage(ATimes10(*Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) - 1));
    return FALSE;
}

bool32 Tcg_PayDayEffect(void)
{
    if (!Tcg_TossCoin(TCG_TEXT_IfHeadsDraw1CardFromDeck))
        return FALSE;
    return Draw1Card();
}

bool32 Tcg_PounceEffect(void)
{
    ApplySubstatus2ToDefendingCard(TCG_SUBSTATUS2_POUNCE);
    return FALSE;
}

bool32 Tcg_ScrunchEffect(void)
{
    return NoDamageNextTurnIfHeads(TCG_ATK_ANIM_SCRUNCH, TCG_SUBSTATUS1_NO_DAMAGE_SCRUNCH);
}

bool32 Tcg_ChanseyDoubleEdgeEffect(void)
{
    Tcg_DealRecoilDamageToSelf(80);
    return FALSE;
}

bool32 Tcg_FetchEffect(void)
{
    return Draw1Card();
}

bool32 Tcg_CometPunch_AIEffect(void)
{
    return SetExpectedAIDamage((20 * 4) / 2, 0, 80);
}

bool32 Tcg_CometPunch_MultiplierEffect(void)
{
    return CoinsTimesDamage(4, 20);
}

bool32 Tcg_TaurosStomp_AIEffect(void)
{
    return SetExpectedAIDamage((20 + 30) / 2, 20, 30);
}

bool32 Tcg_TaurosStomp_DamageBoostEffect(void)
{
    return DamageBoostIfHeads(10);
}

bool32 Tcg_Rampage_AIEffect(void)
{
    AddToDamage(Tcg_GetCardDamageAndMaxHP(TCG_PLAY_AREA_ARENA, NULL));
    return SetDefiniteAIDamage();
}

// Rampage: 10 more damage for each damage counter on the user; a coin, tails: it's confused
bool32 Tcg_Rampage_Confusion50PercentEffect(void)
{
    AddToDamage(Tcg_GetCardDamageAndMaxHP(TCG_PLAY_AREA_ARENA, NULL));
    if (Tcg_TossCoin(TCG_TEXT_IfTailsYourPokemonBecomesConfused))
        return TRUE;
    Tcg_SwapTurn();
    ConfusionEffect();
    Tcg_SwapTurn();
    return FALSE;
}

bool32 Tcg_ThickSkinnedEffect(void)
{
    return TRUE;
}

// ---- Rod's dragons --------------------------------------------------------------------------

bool32 Tcg_DragonairSlam_AIEffect(void)
{
    return SetExpectedAIDamage((30 * 2) / 2, 0, 60);
}

bool32 Tcg_DragonairSlam_MultiplierEffect(void)
{
    return CoinsTimesDamage(2, 30);
}

bool32 Tcg_DragonairHyperBeam_PlayerSelectEffect(void)
{
    return HandleEnergyDiscardEffectSelection();
}

bool32 Tcg_DragonairHyperBeam_AISelectEffect(void)
{
    gTcg->temp = AIPickEnergyCardToDiscardFromDefendingPokemon();
    return FALSE;
}

// an Energy card off the defender (unless it's protected)
bool32 Tcg_DragonairHyperBeam_DiscardEffect(void)
{
    if (HandleNoDamageOrEffect())
        return TRUE;
    if (gTcg->temp == 0xFF)
        return FALSE;
    Tcg_SwapTurn();
    Tcg_PutCardInDiscardPile(gTcg->temp);
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_EFFECT) = TCG_LAST_TURN_EFFECT_DISCARD_ENERGY;
    Tcg_SwapTurn();
    return FALSE;
}

bool32 Tcg_HealingWind_InitialEffect(void)
{
    return TRUE;
}

// Dragonite's Healing Wind (when it's played): 2 damage counters off each of the turn holder's Pokémon
bool32 Tcg_HealingWind_PlayAreaHealEffect(void)
{
    u32 e, n;
    Tcg_PlayAttackAnimation(gTcg->whoseTurn, gTcg->tempPlayAreaLocation, 0, 0);
    Tcg_WaitAttackAnimation();
    gTcg->loadedAttack.animation = TCG_ATK_ANIM_HEALING_WIND_PLAY_AREA;
    n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    for (e = 0; e < n; e++)
    {
        u32 damage, heal = 20;
        gTcg->tempPlayAreaLocation = e;
        damage = Tcg_GetCardDamageAndMaxHP(e, NULL);
        if (damage == 0)
            continue;
        if (damage < heal)
            heal = damage;
        *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + e) += heal;
        Tcg_PlayAttackAnimation(gTcg->whoseTurn, e, 1, heal);
        Tcg_WaitAttackAnimation();
    }
    return FALSE;
}

bool32 Tcg_DragoniteLv41Slam_AIEffect(void)
{
    return SetExpectedAIDamage((30 * 2) / 2, 0, 60);
}

bool32 Tcg_DragoniteLv41Slam_MultiplierEffect(void)
{
    return CoinsTimesDamage(2, 30);
}

// ---- Trainers -------------------------------------------------------------------------------

// the hand discarded, 7 drawn
bool32 Tcg_ProfessorOakEffect(void)
{
    const u8 *c;
    Tcg_CreateHandCardList();
    Tcg_SortCardsInDuelTempListByID();
    for (c = gTcg->duelTempList; *c != 0xFF; c++)
    {
        Tcg_RemoveCardFromHand(*c);
        Tcg_PutCardInDiscardPile(*c);
    }
    DrawNCards(7);
    return FALSE;
}

bool32 Tcg_BillEffect(void)
{
    u32 n;
    Tcg_DisplayDrawNCardsScreen(2);
    for (n = 0; n < 2; n++)
    {
        u8 card;
        if (Tcg_DrawCardFromDeck(&card))
            break;
        gTcg->tempCardIndex = card;
        Tcg_AddCardToHand(card);
        if (IsPlayerTurn())
            Tcg_DisplayPlayerDrawCardScreen();
    }
    return FALSE;
}

// a coin: heads 8 cards, tails 1, after the hand's shuffled back into the deck
bool32 Tcg_GamblerEffect(void)
{
    const u8 *c;
    gTcg->temp = Tcg_TossCoin(TCG_TEXT_CardCheckIfHeads8CardsIfTails1Card);
    Tcg_RemoveCardFromHand(gTcg->tempCardIndexFf9f);
    Tcg_PutCardInDiscardPile(gTcg->tempCardIndexFf9f);
    Tcg_CreateHandCardList();
    Tcg_SortCardsInDuelTempListByID();
    for (c = gTcg->duelTempList; *c != 0xFF; c++)
    {
        Tcg_RemoveCardFromHand(*c);
        Tcg_ReturnCardToDeck(*c);
    }
    ShuffleCardsInDeck();
    DrawNCards(gTcg->temp ? 8 : 1);
    return FALSE;
}

// wDuelTempList: the Pokémon cards in the turn holder's hand. TRUE: none.
static bool32 CreatePokemonCardListFromHand(void)
{
    u32 i, n = 0, count = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND);
    for (i = 0; i < count; i++)
    {
        u32 card = *Tcg_TurnVar(TCG_DUELVARS_HAND + i);
        Tcg_LoadCard2(card);
        if (gTcg->loadedCard2->type < TCG_TYPE_ENERGY)
            gTcg->duelTempList[n++] = card;
    }
    gTcg->duelTempList[n] = 0xFF;
    return n == 0;
}

bool32 Tcg_PokemonTrader_HandDeckCheck(void)
{
    gTcg->hlText = TCG_TEXT_ThereAreNoCardsInHandThatYouCanChange;
    if (*Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND) < 2)
        return TRUE;
    return CreatePokemonCardListFromHand();
}

bool32 Tcg_PokemonTrader_PlayerHandSelection(void)
{
    u32 card;
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ChooseCardFromYourHandToSwitch);
    CreatePokemonCardListFromHand();
    Tcg_InitAndDrawCardListScreenLayout(TRUE);
    Tcg_SetCardListHeaderText(TCG_TEXT_ChooseCardToSwitch, TCG_TEXT_DuelistHand);
    card = Tcg_DisplayCardList();
    gTcg->temp = card;
    return card == TCG_SELECTION_CANCEL;
}

bool32 Tcg_PokemonTrader_PlayerDeckSelection(void)
{
    // (the hand card goes into the deck for a moment, so it can be picked)
    Tcg_RemoveCardFromHand(gTcg->temp);
    Tcg_ReturnCardToDeck(gTcg->temp);
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ChooseBasicOrEvolutionPokemonCardFromDeck);
    Tcg_CreateDeckCardList();
    Tcg_InitAndDrawCardListScreenLayout(TRUE);
    Tcg_SetCardListHeaderText(TCG_TEXT_ChoosePokemonCard, TCG_TEXT_DuelistDeck);
    for (;;)
    {
        u32 card = Tcg_DisplayCardList();
        if (card == TCG_SELECTION_CANCEL)
            continue;
        Tcg_LoadCard2(card);
        if (gTcg->loadedCard2->type >= TCG_TYPE_ENERGY)
            continue;
        gTcg->tempPlayAreaLocationFfa1 = card;
        Tcg_SearchCardInDeckAndAddToHand(gTcg->temp);
        Tcg_AddCardToHand(gTcg->temp);
        return FALSE;
    }
}

bool32 Tcg_PokemonTrader_TradeCardsEffect(void)
{
    Tcg_RemoveCardFromHand(gTcg->temp);
    Tcg_ReturnCardToDeck(gTcg->temp);
    Tcg_SearchCardInDeckAndAddToHand(gTcg->tempPlayAreaLocationFfa1);
    Tcg_AddCardToHand(gTcg->tempPlayAreaLocationFfa1);
    if (!IsPlayerTurn())
    {
        Tcg_DisplayCardDetailScreen(gTcg->temp, TCG_TEXT_PokemonWasReturnedToDeck);
        Tcg_DisplayCardDetailScreen(gTcg->tempPlayAreaLocationFfa1, TCG_TEXT_WasPlacedInTheHand);
    }
    ShuffleCardsInDeck();
    return FALSE;
}

// wDuelTempList: the Stage 2 cards in the turn holder's hand that can evolve one of their Basics in
// play with Pokémon Breeder. TRUE: none.
static bool32 CreatePlayableStage2PokemonCardListFromHand(void)
{
    u32 i, n = 0, count;
    if (Tcg_CreateHandCardList())
        return TRUE;
    count = Tcg_CountCardsInDuelTempList();
    for (i = 0; i < count; i++)
    {
        u32 card = gTcg->duelTempList[i], e, inPlay;
        Tcg_LoadCard2(card);
        if (gTcg->loadedCard2->type >= TCG_TYPE_ENERGY || gTcg->loadedCard2->stage != TCG_STAGE2)
            continue;
        inPlay = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
        for (e = 0; e < inPlay; e++)
            if (!Tcg_CheckIfCanEvolveInto_BasicToStage2(e, card))
                break;
        if (e < inPlay)
            gTcg->duelTempList[n++] = card;
    }
    gTcg->duelTempList[n] = 0xFF;
    return n == 0;
}

bool32 Tcg_PokemonBreeder_HandPlayAreaCheck(void)
{
    if (CreatePlayableStage2PokemonCardListFromHand())
        return Fail(TCG_TEXT_ConditionsForEvolvingToStage2NotFulfilled);
    return Tcg_IsPrehistoricPowerActive();
}

bool32 Tcg_PokemonBreeder_PlayerSelection(void)
{
    u32 card;
    CreatePlayableStage2PokemonCardListFromHand();
    Tcg_InitAndDrawCardListScreenLayout(TRUE);
    Tcg_SetCardListHeaderText(TCG_TEXT_PleaseSelectCard, TCG_TEXT_DuelistHand);
    card = Tcg_DisplayCardList();
    if (card == TCG_SELECTION_CANCEL)
        return TRUE;
    gTcg->temp = card;
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ChooseBasicPokemonToEvolve);
    for (;;)
    {
        u32 loc = Tcg_OpenPlayAreaScreenForSelection(FALSE);
        if (loc == TCG_SELECTION_CANCEL)
            return TRUE;
        gTcg->tempPlayAreaLocationFfa1 = loc;
        if (!Tcg_CheckIfCanEvolveInto_BasicToStage2(loc, gTcg->temp))
            return FALSE;
    }
}

bool32 Tcg_PokemonBreeder_EvolveEffect(void)
{
    u8 trainer = gTcg->tempCardIndexFf9f;
    gTcg->tempCardIndex = gTcg->temp;
    gTcg->tempPlayAreaLocation = gTcg->tempPlayAreaLocationFfa1;
    Tcg_LoadCard1(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcg->tempPlayAreaLocation));
    Tcg_LoadTxRam2(gTcg->loadedCard1->name);
    Tcg_EvolvePokemonCard();
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STAGE + gTcg->tempPlayAreaLocation) = TCG_STAGE2_WITHOUT_STAGE1;
    Tcg_LoadCard1(gTcg->tempCardIndex);
    Tcg_CopyCardNameAndLevel();
    gTcg->txRam2[1] = NULL;
    Tcg_DrawLargePictureOfCard();
    Tcg_PlaySFX(TCG_SFX_POKEMON_EVOLUTION);
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_PokemonEvolvedIntoPokemon);
    Tcg_ProcessPlayedPokemonCard();
    gTcg->tempCardIndexFf9f = trainer;
    return FALSE;
}

bool32 Tcg_EnergyRetrieval_HandEnergyCheck(void)
{
    if (*Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND) < 2)
        return Fail(TCG_TEXT_NotEnoughCardsInHand);
    gTcg->hlText = TCG_TEXT_ThereAreNoBasicEnergyCardsInDiscardPile;
    return CreateEnergyCardListFromDiscardPile(TRUE);
}

bool32 Tcg_EnergyRetrieval_PlayerHandSelection(void)
{
    u32 card;
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ChooseCardToDiscardFromHand);
    Tcg_CreateHandCardList();
    Tcg_RemoveCardFromDuelTempList(gTcg->tempCardIndexFf9f);
    Tcg_InitAndDrawCardListScreenLayout(TRUE);
    card = Tcg_DisplayCardList();
    gTcg->tempList[0] = gTcg->tempCardIndex;
    return card == TCG_SELECTION_CANCEL;
}

bool32 Tcg_EnergyRetrieval_PlayerDiscardPileSelection(void)
{
    gTcg->curSelectionItem = 1;     // (after the hand card)
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_Choose2BasicEnergyCardsFromDiscardPile);
    CreateEnergyCardListFromDiscardPile(TRUE);
    for (;;)
    {
        u32 card;
        Tcg_InitAndDrawCardListScreenLayout(FALSE);
        Tcg_SetCardListHeaderText(TCG_TEXT_PleaseSelectCard, TCG_TEXT_PlayerDiscardPile);
        card = Tcg_DisplayCardList();
        if (card == TCG_SELECTION_CANCEL)
        {
            if (AskWhetherToQuitSelectingCards(2 + 1))
                continue;
            break;
        }
        *GetNextPositionInTempList() = card;
        if (Tcg_RemoveCardFromDuelTempList(card))
            break;
        if (gTcg->curSelectionItem >= 2 + 1)
            break;
    }
    *GetNextPositionInTempList() = 0xFF;
    return FALSE;
}

// the hand card hTempList[0] discarded; the Energy cards after it, from the discard pile to the hand
static bool32 DiscardAndAddToHand(u32 discards)
{
    const u8 *c = gTcg->tempList;
    u32 n = 0;
    while (discards--)
    {
        Tcg_RemoveCardFromHand(*c);
        Tcg_PutCardInDiscardPile(*c);
        c++;
    }
    for (; ; c++)
    {
        gTcg->duelTempList[n++] = *c;
        if (*c == 0xFF)
            break;
        Tcg_MoveDiscardPileCardToHand(*c);
        Tcg_AddCardToHand(*c);
    }
    if (!IsPlayerTurn())
        Tcg_DisplayCardListDetails();
    return FALSE;
}

bool32 Tcg_EnergyRetrieval_DiscardAndAddToHandEffect(void)
{
    return DiscardAndAddToHand(1);
}

bool32 Tcg_SuperEnergyRetrieval_HandEnergyCheck(void)
{
    if (*Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND) < 3)
        return Fail(TCG_TEXT_NotEnoughCardsInHand);
    gTcg->hlText = TCG_TEXT_ThereAreNoBasicEnergyCardsInDiscardPile;
    return CreateEnergyCardListFromDiscardPile(TRUE);
}

bool32 Tcg_SuperEnergyRetrieval_PlayerHandSelection(void)
{
    return HandlePlayerSelection2HandCardsToDiscard();
}

bool32 Tcg_SuperEnergyRetrieval_PlayerDiscardPileSelection(void)
{
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ChooseUpTo4FromDiscardPile);
    CreateEnergyCardListFromDiscardPile(TRUE);
    for (;;)
    {
        u32 card;
        Tcg_InitAndDrawCardListScreenLayout(FALSE);
        Tcg_SetCardListHeaderText(TCG_TEXT_PleaseSelectCard, TCG_TEXT_PlayerDiscardPile);
        card = Tcg_DisplayCardList();
        if (card == TCG_SELECTION_CANCEL)
        {
            if (AskWhetherToQuitSelectingCards(6))
                continue;
            break;
        }
        *GetNextPositionInTempList() = card;
        if (Tcg_RemoveCardFromDuelTempList(card))
            break;
        if (gTcg->curSelectionItem >= 6)
            break;
    }
    *GetNextPositionInTempList() = 0xFF;
    return FALSE;
}

bool32 Tcg_SuperEnergyRetrieval_DiscardAndAddToHandEffect(void)
{
    return DiscardAndAddToHand(2);
}

bool32 Tcg_EnergyRemoval_EnergyCheck(void)
{
    bool32 none;
    Tcg_SwapTurn();
    none = CheckIfThereAreAnyEnergyCardsAttached();
    gTcg->hlText = TCG_TEXT_NoEnergyAttachedToOpponentsActive;
    Tcg_SwapTurn();
    return none;
}

bool32 Tcg_EnergyRemoval_PlayerSelection(void)
{
    bool32 cancelled;
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ChoosePokemonToRemoveEnergyFrom);
    Tcg_SwapTurn();
    cancelled = HandlePokemonAndEnergySelectionScreen();
    Tcg_SwapTurn();
    return cancelled;
}

// (the game returns its pick in a, which the AI's Trainer code stores itself)
bool32 Tcg_EnergyRemoval_AISelection(void)
{
    AIPickEnergyCardToDiscardFromDefendingPokemon();
    return FALSE;
}

bool32 Tcg_EnergyRemoval_DiscardEffect(void)
{
    Tcg_SwapTurn();
    Tcg_PutCardInDiscardPile(gTcg->tempPlayAreaLocationFfa1);
    Tcg_SwapTurn();
    if (IsPlayerTurn())
        return FALSE;
    Tcg_SwapTurn();
    DrawPlayAreaScreenToShowChanges(gTcg->temp);
    Tcg_SwapTurn();
    return FALSE;
}

bool32 Tcg_SuperEnergyRemoval_EnergyCheck(void)
{
    bool32 none;
    if (CheckIfThereAreAnyEnergyCardsAttached())
        return Fail(TCG_TEXT_NoEnergyCardsAttachedToPokemonInYourPlayArea);
    Tcg_SwapTurn();
    none = CheckIfThereAreAnyEnergyCardsAttached();
    gTcg->hlText = TCG_TEXT_NoEnergyCardsAttachedToPokemonInOppPlayArea;
    Tcg_SwapTurn();
    return none;
}

// one Energy off one of the player's Pokémon, then up to 2 off one of the opponent's
bool32 Tcg_SuperEnergyRemoval_PlayerSelection(void)
{
    u32 loc;
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ChoosePokemonInYourAreaThenPokemonInYourOpp);
    if (HandlePokemonAndEnergySelectionScreen())
        return TRUE;
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ChoosePokemonToRemoveEnergyFrom);
    Tcg_SwapTurn();
    gTcg->curSelectionItem = 3;
    for (;;)
    {
        loc = Tcg_OpenPlayAreaScreenForSelection(FALSE);
        if (loc == TCG_SELECTION_CANCEL)
        {
            Tcg_SwapTurn();
            return TRUE;
        }
        gTcg->tempPlayAreaLocation = loc;
        Tcg_GetPlayAreaCardAttachedEnergies(loc);
        if (gTcg->totalAttachedEnergies)
            break;
        Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_NoEnergyCards);
    }
    gTcg->playAreaEffectTarget = gTcg->tempPlayAreaLocation;
    Tcg_CreateArenaOrBenchEnergyCardList(loc);
    Tcg_DisplayEnergyDiscardScreen(gTcg->tempPlayAreaLocation);
    gTcg->energyDiscardMenuDenominator = 2;
    for (;;)
    {
        u32 card = Tcg_HandleEnergyDiscardMenuInput();
        if (card == TCG_SELECTION_CANCEL)
        {
            u32 numerator;
            if (!AskWhetherToQuitSelectingCards(5))
                break;
            numerator = gTcg->energyDiscardMenuNumerator;
            Tcg_DisplayEnergyDiscardScreen(gTcg->tempPlayAreaLocation);
            gTcg->energyDiscardMenuDenominator = 2;
            gTcg->energyDiscardMenuNumerator = numerator;
            continue;
        }
        *GetNextPositionInTempList() = card;
        Tcg_RemoveCardFromDuelTempList(card);
        gTcg->energyDiscardMenuNumerator++;
        if (gTcg->curSelectionItem >= 5 || gTcg->duelTempList[0] == 0xFF)
            break;
        Tcg_DisplayEnergyDiscardMenu();
    }
    *GetNextPositionInTempList() = 0xFF;
    Tcg_SwapTurn();
    return FALSE;
}

bool32 Tcg_SuperEnergyRemoval_DiscardEffect(void)
{
    const u8 *c = &gTcg->tempList[1];
    Tcg_PutCardInDiscardPile(*c++);
    c++;
    Tcg_SwapTurn();
    for (; *c != 0xFF; c++)
        Tcg_PutCardInDiscardPile(*c);
    Tcg_SwapTurn();
    if (IsPlayerTurn())
        return FALSE;
    DrawPlayAreaScreenToShowChanges(gTcg->temp);
    gTcg->duelDisplayedScreen = 0;
    Tcg_SwapTurn();
    DrawPlayAreaScreenToShowChanges(gTcg->playAreaEffectTarget);
    Tcg_SwapTurn();
    return FALSE;
}

bool32 Tcg_Switch_BenchCheck(void)
{
    gTcg->hlText = TCG_TEXT_EffectNoPokemonOnTheBench;
    return *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) < 2;
}

bool32 Tcg_Switch_PlayerSelection(void)
{
    u32 loc;
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_SelectPkmnOnBenchToSwitchWithActive);
    loc = Tcg_OpenPlayAreaScreenForSelection(TRUE);
    gTcg->temp = gTcg->tempPlayAreaLocation;
    if (loc != TCG_SELECTION_CANCEL)
        gTcg->temp = loc;
    return loc == TCG_SELECTION_CANCEL;
}

bool32 Tcg_Switch_SwitchEffect(void)
{
    Tcg_SwapArenaWithBenchPokemon(gTcg->temp);
    return FALSE;
}

bool32 Tcg_PokemonCenter_DamageCheck(void)
{
    gTcg->hlText = TCG_TEXT_NoPokemonWithDamageCounters;
    return CheckIfPlayAreaHasAnyDamage();
}

// all the turn holder's Pokémon healed, and the Energy on each healed one discarded
bool32 Tcg_PokemonCenter_HealDiscardEnergyEffect(void)
{
    u32 e, n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA), i;
    for (e = 0; e < n; e++)
    {
        u32 damage;
        gTcg->tempPlayAreaLocation = e;
        damage = Tcg_GetCardDamageAndMaxHP(e, NULL);
        if (damage == 0)
            continue;
        HealPlayAreaCardHP(damage);
        for (i = 0; i < TCG_DECK_SIZE; i++)
        {
            if (*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + i) != TCG_PLAY_AREA(e))
                continue;
            Tcg_LoadCard2(i);
            if (gTcg->loadedCard2->type & TCG_TYPE_ENERGY)
                Tcg_PutCardInDiscardPile(i);
        }
    }
    return FALSE;
}

bool32 Tcg_PokeBall_DeckCheck(void)
{
    gTcg->hlText = TCG_TEXT_NoCardsLeftInTheDeck;
    return *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_NOT_IN_DECK) >= TCG_DECK_SIZE;
}

// TRUE: a Pokémon card in wDuelTempList
static bool32 IsPokemonInDuelTempList(void)
{
    const u8 *c;
    for (c = gTcg->duelTempList; *c != 0xFF; c++)
    {
        Tcg_LoadCard2(*c);
        if (gTcg->loadedCard2->type < TCG_TYPE_ENERGY)
            return TRUE;
    }
    return FALSE;
}

// a coin: heads, a Pokémon from the deck (hTempList: the coin, the card)
bool32 Tcg_PokeBall_PlayerSelection(void)
{
    gTcg->tempList[0] = Tcg_TossCoin(TCG_TEXT_TrainerCardSuccessCheck);
    if (!gTcg->tempList[0])
        return FALSE;
    Tcg_CreateDeckCardList();
    if (LookForCardsInDeck(TCG_SEARCHEFFECT_POKEMON, 0, TCG_TEXT_ChooseBasicOrEvolutionPokemonCardFromDeck, TCG_TEXT_EvolutionCard))
    {
        gTcg->tempList[1] = 0xFF;
        return FALSE;
    }
    Tcg_InitAndDrawCardListScreenLayout(TRUE);
    Tcg_SetCardListHeaderText(TCG_TEXT_ChoosePokemonCard, TCG_TEXT_DuelistDeck);
    for (;;)
    {
        u32 card = Tcg_DisplayCardList();
        if (card == TCG_SELECTION_CANCEL)
        {
            if (!IsPokemonInDuelTempList())
            {
                gTcg->tempList[1] = 0xFF;
                return FALSE;
            }
            continue;
        }
        Tcg_LoadCard2(card);
        if (gTcg->loadedCard2->type < TCG_TYPE_ENERGY)
        {
            gTcg->tempList[1] = card;
            return FALSE;
        }
        Tcg_PlaySFX_InvalidChoice();
    }
}

bool32 Tcg_PokeBall_AddToHandEffect(void)
{
    if (!gTcg->tempList[0])
        return FALSE;
    if (gTcg->tempList[1] != 0xFF)
    {
        Tcg_SearchCardInDeckAndAddToHand(gTcg->tempList[1]);
        Tcg_AddCardToHand(gTcg->tempList[1]);
        if (!IsPlayerTurn())
            Tcg_DisplayCardDetailScreen(gTcg->tempList[1], TCG_TEXT_WasPlacedInTheHand);
    }
    ShuffleCardsInDeck();
    return FALSE;
}

bool32 Tcg_ScoopUp_BenchCheck(void)
{
    gTcg->hlText = TCG_TEXT_EffectNoPokemonOnTheBench;
    return *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) < 2;
}

bool32 Tcg_ScoopUp_PlayerSelection(void)
{
    u32 loc;
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ChoosePokemonToScoopUp);
    loc = Tcg_OpenPlayAreaScreenForSelection(FALSE);
    if (loc == TCG_SELECTION_CANCEL)
        return TRUE;
    gTcg->temp = loc;
    if (loc != TCG_PLAY_AREA_ARENA)
        return FALSE;
    Tcg_EmptyScreen();
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_SelectPokemonToPlaceInTheArena);
    gTcg->tempPlayAreaLocationFfa1 = Tcg_OpenPlayAreaScreenForSelection(TRUE);
    return gTcg->tempPlayAreaLocationFfa1 == TCG_SELECTION_CANCEL;
}

// the Basic Pokémon at hTemp_ffa0 back to the hand, the rest of its cards discarded
bool32 Tcg_ScoopUp_ReturnToHandEffect(void)
{
    u32 i, where = TCG_PLAY_AREA(gTcg->temp);
    for (i = 0; i < TCG_DECK_SIZE; i++)
    {
        if (*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + i) != where)
            continue;
        Tcg_LoadCard2(i);
        if (gTcg->loadedCard2->type >= TCG_TYPE_ENERGY || gTcg->loadedCard2->stage != TCG_BASIC)
            continue;
        gTcg->tempCardIndex = i;
        Tcg_AddCardToHand(i);
    }
    Tcg_MovePlayAreaCardToDiscardPile(gTcg->temp);
    if (gTcg->temp == TCG_PLAY_AREA_ARENA)
        Tcg_ClearAllStatusConditions();
    if (!IsPlayerTurn())
        Tcg_DisplayCardDetailScreen(gTcg->tempCardIndex, gTcg->temp == TCG_PLAY_AREA_ARENA
            ? TCG_TEXT_PokemonWasReturnedFromArenaToHand : TCG_TEXT_PokemonWasReturnedFromBenchToHand);
    if (gTcg->temp != TCG_PLAY_AREA_ARENA)
    {
        Tcg_ShiftAllPokemonToFirstPlayAreaSlots();
        return FALSE;
    }
    Tcg_SwapPlayAreaPokemon(gTcg->tempPlayAreaLocationFfa1, TCG_PLAY_AREA_ARENA);
    Tcg_ShiftAllPokemonToFirstPlayAreaSlots();
    return FALSE;
}

bool32 Tcg_ComputerSearch_HandDeckCheck(void)
{
    if (*Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND) < 3)
        return Fail(TCG_TEXT_NotEnoughCardsInHand);
    gTcg->hlText = TCG_TEXT_NoCardsLeftInTheDeck;
    return *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_NOT_IN_DECK) >= TCG_DECK_SIZE;
}

bool32 Tcg_ComputerSearch_PlayerDiscardHandSelection(void)
{
    return HandlePlayerSelection2HandCardsToDiscard();
}

bool32 Tcg_ComputerSearch_PlayerDeckSelection(void)
{
    u32 card;
    Tcg_CreateDeckCardList();
    Tcg_InitAndDrawCardListScreenLayout(TRUE);
    Tcg_SetCardListHeaderText(TCG_TEXT_ChooseCardToPlaceInHand, TCG_TEXT_DuelistDeck);
    do
        card = Tcg_DisplayCardList();
    while (card == TCG_SELECTION_CANCEL);
    gTcg->tempList[2] = card;
    return FALSE;
}

bool32 Tcg_ComputerSearch_DiscardAddToHandEffect(void)
{
    Tcg_RemoveCardFromHand(gTcg->tempList[0]);
    Tcg_PutCardInDiscardPile(gTcg->tempList[0]);
    Tcg_RemoveCardFromHand(gTcg->tempList[1]);
    Tcg_PutCardInDiscardPile(gTcg->tempList[1]);
    Tcg_SearchCardInDeckAndAddToHand(gTcg->tempList[2]);
    Tcg_AddCardToHand(gTcg->tempList[2]);
    ShuffleCardsInDeck();
    return FALSE;
}

bool32 Tcg_PlusPowerEffect(void)
{
    Tcg_PutHandCardInPlayArea(gTcg->tempCardIndexFf9f, TCG_PLAY_AREA_ARENA);
    (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_ATTACHED_PLUSPOWER))++;
    return FALSE;
}

bool32 Tcg_Defender_PlayerSelection(void)
{
    u32 loc;
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ChoosePokemonToAttachDefenderTo);
    loc = Tcg_OpenPlayAreaScreenForSelection(FALSE);
    if (loc != TCG_SELECTION_CANCEL)
        gTcg->tempPlayAreaLocation = loc;
    gTcg->temp = gTcg->tempPlayAreaLocation;
    return loc == TCG_SELECTION_CANCEL;
}

bool32 Tcg_Defender_AttachDefenderEffect(void)
{
    Tcg_PutHandCardInPlayArea(gTcg->tempCardIndexFf9f, gTcg->temp);
    (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_ATTACHED_DEFENDER + gTcg->temp))++;
    if (!IsPlayerTurn())
        DrawPlayAreaScreenToShowChanges(gTcg->temp);
    return FALSE;
}

bool32 Tcg_GustOfWind_BenchCheck(void)
{
    gTcg->hlText = TCG_TEXT_EffectNoPokemonOnTheBench;
    return *Tcg_NonTurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) < 2;
}

bool32 Tcg_GustOfWind_PlayerSelection(void)
{
    u32 loc;
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ChooseAPokemonToSwitchWithActivePokemon);
    Tcg_SwapTurn();
    loc = Tcg_OpenPlayAreaScreenForSelection(TRUE);
    if (loc != TCG_SELECTION_CANCEL)
        gTcg->tempPlayAreaLocation = loc;
    gTcg->temp = gTcg->tempPlayAreaLocation;
    Tcg_SwapTurn();
    return loc == TCG_SELECTION_CANCEL;
}

bool32 Tcg_GustOfWind_SwitchEffect(void)
{
    PlayTrainerEffectAnimation(TCG_ATK_ANIM_GUST_OF_WIND);
    Tcg_SwapTurn();
    Tcg_SwapArenaWithBenchPokemon(gTcg->temp);
    Tcg_SwapTurn();
    Tcg_ClearDamageReductionSubstatus2();
    gTcg->duelDisplayedScreen = 0;
    return FALSE;
}

bool32 Tcg_Potion_DamageCheck(void)
{
    gTcg->hlText = TCG_TEXT_NoPokemonWithDamageCounters;
    return CheckIfPlayAreaHasAnyDamage();
}

// a Pokémon with damage (hTemp_ffa0), and how much Potion heals (up to 20: hTempPlayAreaLocation_ffa1)
bool32 Tcg_Potion_PlayerSelection(void)
{
    for (;;)
    {
        u32 loc = Tcg_OpenPlayAreaScreenForSelection(FALSE), damage;
        if (loc == TCG_SELECTION_CANCEL)
            return TRUE;
        gTcg->temp = loc;
        damage = Tcg_GetCardDamageAndMaxHP(loc, NULL);
        if (damage == 0)
            continue;
        gTcg->tempPlayAreaLocationFfa1 = damage < 20 ? damage : 20;
        return FALSE;
    }
}

bool32 Tcg_Potion_HealEffect(void)
{
    gTcg->tempPlayAreaLocation = gTcg->temp;
    HealPlayAreaCardHP(gTcg->tempPlayAreaLocationFfa1);
    return FALSE;
}

bool32 Tcg_FullHeal_StatusCheck(void)
{
    if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS))
        return FALSE;
    return Fail(TCG_TEXT_NotAffectedByPoisonSleepParalysisOrConfusion);
}

bool32 Tcg_FullHeal_ClearStatusEffect(void)
{
    PlayTrainerEffectAnimation(TCG_ATK_ANIM_FULL_HEAL);
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS) = TCG_NO_STATUS;
    Tcg_DrawDuelHUDs();
    return FALSE;
}

bool32 Tcg_Recycle_DiscardPileCheck(void)
{
    gTcg->hlText = TCG_TEXT_ThereAreNoCardsInTheDiscardPile;
    return *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_DISCARD_PILE) < 1;
}

// a coin: heads, a card from the discard pile onto the top of the deck (hTempList[0])
bool32 Tcg_Recycle_PlayerSelection(void)
{
    u32 card;
    if (!Tcg_TossCoin(TCG_TEXT_TrainerCardSuccessCheck))
    {
        gTcg->tempList[0] = 0xFF;
        return FALSE;
    }
    Tcg_CreateDiscardPileCardList();
    Tcg_InitAndDrawCardListScreenLayout(TRUE);
    Tcg_SetCardListHeaderText(TCG_TEXT_PleaseSelectCard, TCG_TEXT_PlayerDiscardPile);
    do
        card = Tcg_DisplayCardList();
    while (card == TCG_SELECTION_CANCEL);
    gTcg->tempList[0] = card;
    return FALSE;
}

bool32 Tcg_Recycle_AddToHandEffect(void)
{
    if (gTcg->tempList[0] == 0xFF)
        return FALSE;
    Tcg_MoveDiscardPileCardToHand(gTcg->tempList[0]);
    Tcg_ReturnCardToDeck(gTcg->tempList[0]);
    if (!IsPlayerTurn())
        Tcg_DisplayCardDetailScreen(gTcg->tempList[0], TCG_TEXT_CardWasChosen);
    return FALSE;
}
