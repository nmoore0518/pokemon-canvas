// Pokémon Canvas: the Trading Card Game duel's loop, its setup, between-turn events, and the duel
// menu's rules, ported from the Game Boy Color game's engine/duel/core.asm (pret/poketcg) under the
// same names (Tcg_ prefixed): StartDuel, MainDuelLoop, HandleTurn, HandleDuelSetup,
// ChooseInitialArenaAndBenchPokemon, ShuffleDeckAndDrawSevenCards, InitializeDuelVariables,
// InitTurnDuelistPrizes, HandleBetweenTurnsEvents (poison, sleep, paralysis; PlusPower, Defender),
// the duel menu (DuelMenu_Hand, _Attack, _PkmnPower, _Retreat, _Done and what they call:
// PlayEnergyCard, PlayPokemonCard, CheckAbleToRetreat, DisplayRetreatScreen's choosing,
// AttemptRetreat, CheckIfEnoughEnergiesToAttack...), and the AI's actions (AIMakeDecision and the
// OppAction_* routines it runs).
// Like the game, this code waits for the screens: it runs as a loop of its own (src/tcg/ui.c does
// the frames), so the rules read as they do in the game. Link-duel code is left out.
#include "global.h"
#include "tcg.h"
#include "tcg_sound.h"
#include "tcg_ui.h"
#include "tcg_gbc.h"
#include "constants/tcg_text.h"

// what a duel menu routine does next (the game jumps to one of these)
enum
{
    MENU_MAIN_INTERFACE,    // DuelMainInterface (redraw, then the menu)
    MENU_AGAIN,             // PrintDuelMenuAndHandleInput (the menu again)
    MENU_TURN_OVER,         // (ret: the turn's over)
};

// ---- the duel's variables -------------------------------------------------------------------

// InitializeDuelVariables: the turn holder's page cleared (but who they are), the deck in order,
// nothing in play
void Tcg_InitializeDuelVariables(void)
{
    u8 *v = gTcg->vars[gTcg->whoseTurn];
    u8 type = v[TCG_DUELVARS_DUELIST_TYPE];
    u32 i;
    memset(v, 0, 256);
    v[TCG_DUELVARS_DUELIST_TYPE] = type;
    for (i = 0; i < TCG_DECK_SIZE; i++)
        v[TCG_DUELVARS_DECK_CARDS + i] = i;
    for (i = 0; i < TCG_MAX_PLAY_AREA_POKEMON + 1; i++)
        v[TCG_DUELVARS_ARENA_CARD + i] = 0xFF;
}

// the turn holder's prizes: wDuelInitialPrizes cards from the top of their deck
void Tcg_InitTurnDuelistPrizes(void)
{
    static const u8 sPrizeBitmasks[] = {0, 1, 3, 7, 15, 31, 63};
    u32 i, n = gTcg->duelInitialPrizes;
    for (i = 0; i < n; i++)
    {
        u8 card;
        Tcg_DrawCardFromDeck(&card);
        *Tcg_TurnVar(TCG_DUELVARS_PRIZE_CARDS + i) = card;
        *Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + card) = TCG_CARD_LOCATION_PRIZE;
    }
    *Tcg_TurnVar(TCG_DUELVARS_PRIZES) = sPrizeBitmasks[n];
}

void Tcg_InitVariablesToBeginDuel(void)
{
    gTcg->duelFinished = 0;
    gTcg->duelTurns = 0;
    gTcg->playerAttackingCardIndex = 0xFF;
    gTcg->playerAttackingAttackIndex = 0xFF;
}

// the variables that last a single player's turn
void Tcg_InitVariablesToBeginTurn(void)
{
    gTcg->alreadyPlayedEnergy = 0;
    gTcg->confusionRetreatCheckWasUnsuccessful = 0;
    gTcg->gotHeadsFromSandAttackOrSmokescreenCheck = 0;
    gTcg->whoseTurnReal = gTcg->whoseTurn;
}

// from each duelist's second turn: everything in play can evolve, and use its Pokémon Power again
void Tcg_SetAllPlayAreaPokemonCanEvolve(void)
{
    u32 i, n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    for (i = 0; i < n; i++)
    {
        u8 *flags = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_FLAGS + i);
        *flags = (*flags & ~TCG_USED_PKMN_POWER_THIS_TURN) | TCG_CAN_EVOLVE_THIS_TURN;
    }
}

// ---- the setup ------------------------------------------------------------------------------

// TRUE: wLoadedCard1 is a Basic Pokémon (IsLoadedCard1BasicPokemon.skip_mysterious_fossil_clefairy_doll:
// there's no Clefairy Doll or Mysterious Fossil here)
static bool32 IsLoadedCard1BasicPokemon(void)
{
    return gTcg->loadedCard1->type < TCG_TYPE_ENERGY && gTcg->loadedCard1->stage == TCG_BASIC;
}

// the turn holder shuffles and draws 7. TRUE: no Basic Pokémon among them.
bool32 Tcg_ShuffleDeckAndDrawSevenCards(void)
{
    u32 i;
    bool32 basic = FALSE;
    Tcg_InitializeDuelVariables();
    Tcg_ShuffleDeck();
    Tcg_ShuffleDeck();
    for (i = 0; i < 7; i++)
    {
        u8 card;
        Tcg_DrawCardFromDeck(&card);
        Tcg_AddCardToHand(card);
    }
    for (i = 0; i < 7; i++)
    {
        Tcg_LoadCard1(*Tcg_TurnVar(TCG_DUELVARS_HAND + i));
        basic |= IsLoadedCard1BasicPokemon();
    }
    return !basic;
}

// the turn holder places their Active Pokémon and any Benched ones (the player, through the screens;
// the AI through AIDoAction_StartDuel)
static void ChooseInitialArenaAndBenchPokemon(void)
{
    if (*Tcg_TurnVar(TCG_DUELVARS_DUELIST_TYPE) != TCG_DUELIST_TYPE_PLAYER)
    {
        u8 type = *Tcg_TurnVar(TCG_DUELVARS_DUELIST_TYPE);
        TcgAi_StartDuel();
        *Tcg_TurnVar(TCG_DUELVARS_DUELIST_TYPE) = type;
        return;
    }
    // .choose_arena
    Tcg_DisplayDuelBoxMessage(TCG_BOXMSG_ARENA_POKEMON);
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ChooseBasicPkmnToPlaceInArena);
    for (;;)
    {
        u32 card = Tcg_DisplayPlaceInitialPokemonCardsScreen(FALSE, TCG_TEXT_PleaseChooseAnActivePokemon);
        if (card == TCG_SELECTION_CANCEL)
            continue;
        gTcg->tempCardIndex = card;
        Tcg_LoadCard1(card);
        Tcg_PutHandPokemonCardInPlayArea(card);
        Tcg_DisplayCardDetailScreen(card, TCG_TEXT_PlacedInTheArena);
        break;
    }
    // .choose_bench
    Tcg_DisplayDuelBoxMessage(TCG_BOXMSG_BENCH_POKEMON);
    Tcg_PrintScrollableText_NoTextBoxLabel(TCG_TEXT_ChooseUpTo5BasicPkmnToPlaceOnBench);
    for (;;)
    {
        u32 card = Tcg_DisplayPlaceInitialPokemonCardsScreen(TRUE, TCG_TEXT_ChooseYourBenchPokemon);
        if (card == TCG_SELECTION_CANCEL)
            break;
        if (*Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) >= TCG_MAX_PLAY_AREA_POKEMON)
        {
            Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_NoSpaceOnTheBench);
            continue;
        }
        gTcg->tempCardIndex = card;
        Tcg_PutHandPokemonCardInPlayArea(card);
        Tcg_DisplayCardDetailScreen(card, TCG_TEXT_PlacedOnTheBench);
    }
}

// both duelists shuffle, draw 7 (again, until they have a Basic Pokémon), place their Pokémon and
// prizes, and a coin decides who goes first
void Tcg_HandleDuelSetup(void)
{
    u32 prevTurn;
    bool32 playerNoBasic, oppNoBasic;
    for (;;)
    {
        Tcg_InitializeDuelVariables();
        Tcg_SwapTurn();
        Tcg_InitializeDuelVariables();
        Tcg_SwapTurn();
        Tcg_PlayShuffleAndDrawCardsAnimation(TRUE);
        playerNoBasic = Tcg_ShuffleDeckAndDrawSevenCards();     // (the turn holder: "player" in the game's names)
        Tcg_SwapTurn();
        oppNoBasic = Tcg_ShuffleDeckAndDrawSevenCards();
        Tcg_SwapTurn();
        if (!playerNoBasic || !oppNoBasic)
            break;
        // .neither_drew_basic_pkmn
        Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_NeitherPlayerHasBasicPkmn);
        Tcg_DisplayNoBasicPokemonInHandScreen();
        Tcg_InitializeDuelVariables();
        Tcg_SwapTurn();
        Tcg_DisplayNoBasicPokemonInHandScreen();
        Tcg_InitializeDuelVariables();
        Tcg_SwapTurn();
        Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ReturnCardsToDeckAndDrawAgain);
    }
    if (playerNoBasic || oppNoBasic)
    {
        if (oppNoBasic)
            Tcg_SwapTurn();
        do
        {
            Tcg_DisplayNoBasicPokemonInHandScreen();
            Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ReturnCardsToDeckAndDrawAgain);
            Tcg_InitializeDuelVariables();
            Tcg_PlayShuffleAndDrawCardsAnimation(FALSE);
        } while (Tcg_ShuffleDeckAndDrawSevenCards());
        if (oppNoBasic)
            Tcg_SwapTurn();
    }
    // .hand_cards_ok
    prevTurn = gTcg->whoseTurn;
    gTcg->whoseTurn = TCG_PLAYER;
    ChooseInitialArenaAndBenchPokemon();
    Tcg_SwapTurn();
    ChooseInitialArenaAndBenchPokemon();
    Tcg_SwapTurn();
    Tcg_DrawPlayAreaToPlacePrizeCards();
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_PlacingThePrizes);
    Tcg_LoadTxRam3(gTcg->duelInitialPrizes);
    Tcg_DrawWideTextBox_PrintText(TCG_TEXT_PleasePlacePrizes);
    Tcg_PlacePrizes();
    Tcg_WaitForWideTextBoxInput();
    gTcg->whoseTurn = prevTurn;
    Tcg_InitTurnDuelistPrizes();
    Tcg_SwapTurn();
    Tcg_InitTurnDuelistPrizes();
    Tcg_SwapTurn();
    Tcg_DisplayDuelBoxMessage(TCG_BOXMSG_COIN_TOSS);
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_CoinTossToDecideWhoPlaysFirst);
    if (gTcg->whoseTurn == TCG_PLAYER)
    {
        u16 text = TCG_TEXT_YouPlayFirst;
        Tcg_CopyDuelistName(TCG_PLAYER, gTcg->defaultText);
        gTcg->txRam2[0] = NULL;
        if (!Tcg_TossCoin(TCG_TEXT_IfHeadsDuelistPlaysFirst))
        {
            Tcg_SwapTurn();
            text = TCG_TEXT_YouPlaySecond;
        }
        Tcg_DrawWideTextBox_WaitForInput(text);
    }
    else
    {
        u16 text = TCG_TEXT_YouPlaySecond;
        Tcg_CopyDuelistName(TCG_OPPONENT, gTcg->defaultText);
        gTcg->txRam2[0] = NULL;
        if (!Tcg_TossCoin(TCG_TEXT_IfHeadsDuelistPlaysFirst))
        {
            Tcg_SwapTurn();
            text = TCG_TEXT_YouPlayFirst;
        }
        Tcg_DrawWideTextBox_WaitForInput(text);
    }
}

// ---- between turns --------------------------------------------------------------------------

// any PlusPower attached to the turn holder's Pokémon, to the discard pile
static void DiscardAttachedPlusPowers(void)
{
    u32 i;
    for (i = 0; i < TCG_MAX_PLAY_AREA_POKEMON; i++)
        *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_ATTACHED_PLUSPOWER + i) = 0;
    Tcg_MoveCardToDiscardPileIfInPlayArea(TCG_CARD_PLUSPOWER);
}

static void DiscardAttachedDefenders(void)
{
    u32 i;
    for (i = 0; i < TCG_MAX_PLAY_AREA_POKEMON; i++)
        *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_ATTACHED_DEFENDER + i) = 0;
    Tcg_MoveCardToDiscardPileIfInPlayArea(TCG_CARD_DEFENDER);
}

// TRUE: the turn holder's Active Pokémon is asleep or poisoned. *status: its confusion, sleep or
// paralysis (when it isn't poisoned).
static bool32 IsArenaPokemonAsleepOrPoisoned(u32 *status)
{
    u8 s = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS);
    *status = 0;
    if (s == 0)
        return FALSE;
    if (s & (TCG_POISONED | TCG_DOUBLE_POISONED))
        return TRUE;
    *status = s & TCG_CNF_SLP_PRZ;
    return *status == TCG_ASLEEP;
}

// wTempNonTurnDuelistCardID's name, and a text
static void PrintCardNameFromCardIDInTextBox(u16 textId)
{
    Tcg_LoadTxRam2(Tcg_Card(gTcg->tempNonTurnDuelistCardId)->name);
    Tcg_DrawWideTextBox_PrintText(textId);
}

// poison's damage to the turn holder's Active Pokémon (10, double poison 20). TRUE: it's knocked out.
static bool32 HandlePoisonDamage(u8 *status)
{
    u32 damage;
    u16 text;
    u8 *hp;
    if (!(*status & (1 << TCG_POISONED_F)))
        return FALSE;
    if (*status & (1 << TCG_DOUBLE_POISONED_F))
    {
        damage = TCG_DBLPSN_DAMAGE;
        text = TCG_TEXT_Received20DamageDueToPoison;
    }
    else
    {
        damage = TCG_PSN_DAMAGE;
        text = TCG_TEXT_Received10DamageDueToPoison;
    }
    Tcg_RedrawTurnDuelistsMainSceneOrDuelHUD();
    PrintCardNameFromCardIDInTextBox(text);
    Tcg_PlayBetweenTurnsAnimation(TCG_DUEL_ANIM_POISON, damage);
    hp = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP);
    Tcg_SubtractHP(hp, damage);
    Tcg_PlayBetweenTurnsAnimation(TCG_DUEL_ANIM_DAMAGE_HUD, damage);
    Tcg_PrintKnockedOutIfHLZero(hp);
    Tcg_WaitForWideTextBoxInput();
    return *hp == 0;
}

// a sleeping Pokémon's coin: heads, it wakes up
static void HandleSleepCheck(u8 *status)
{
    u32 anim;
    u16 text;
    if ((*status & TCG_CNF_SLP_PRZ) != TCG_ASLEEP)
        return;
    gTcg->loadedCard1Id = gTcg->tempNonTurnDuelistCardId;
    gTcg->loadedCard1 = Tcg_Card(gTcg->loadedCard1Id);
    Tcg_CopyCardNameAndLevel();
    gTcg->txRam2[0] = NULL;
    if (Tcg_TossCoin(TCG_TEXT_PokemonsSleepCheck))
    {
        *status &= TCG_DOUBLE_POISONED;
        anim = TCG_DUEL_ANIM_HEAL;
        text = TCG_TEXT_IsCuredOfSleep;
    }
    else
    {
        anim = TCG_DUEL_ANIM_SLEEP;
        text = TCG_TEXT_IsStillAsleep;
    }
    Tcg_RedrawTurnDuelistsMainSceneOrDuelHUD();
    PrintCardNameFromCardIDInTextBox(text);
    Tcg_PlayBetweenTurnsAnimation(anim, 0);
    Tcg_WaitForWideTextBoxInput();
}

// poison, sleep and paralysis between turns (the turn holder's paralysis wears off), and the
// PlusPowers (the turn holder's) and Defenders (the other's) discarded; then the knock-outs
void Tcg_HandleBetweenTurnsEvents(void)
{
    u32 status;
    u8 *s;
    bool32 handle = IsArenaPokemonAsleepOrPoisoned(&status) || status == TCG_PARALYZED;
    if (!handle)
    {
        Tcg_SwapTurn();
        handle = IsArenaPokemonAsleepOrPoisoned(&status);
        Tcg_SwapTurn();
    }
    if (!handle)
    {
        DiscardAttachedPlusPowers();
        Tcg_SwapTurn();
        DiscardAttachedDefenders();
        Tcg_SwapTurn();
        return;
    }
    Tcg_ResetAnimationQueue();
    Tcg_DisplayDuelBoxMessage(TCG_BOXMSG_BETWEEN_TURNS);
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_BetweenTurns);
    gTcg->tempNonTurnDuelistCardId = Tcg_GetCardIdFromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
    s = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS);
    if (*s && !HandlePoisonDamage(s))
    {
        HandleSleepCheck(s);
        if ((*s & TCG_CNF_SLP_PRZ) == TCG_PARALYZED)
        {
            *s &= TCG_DOUBLE_POISONED;
            Tcg_RedrawTurnDuelistsMainSceneOrDuelHUD();
            PrintCardNameFromCardIDInTextBox(TCG_TEXT_IsCuredOfParalysis);
            Tcg_PlayBetweenTurnsAnimation(TCG_DUEL_ANIM_HEAL, 0);
            Tcg_WaitForWideTextBoxInput();
        }
    }
    DiscardAttachedPlusPowers();
    Tcg_SwapTurn();
    gTcg->tempNonTurnDuelistCardId = Tcg_GetCardIdFromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
    s = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS);
    if (*s && !HandlePoisonDamage(s))
        HandleSleepCheck(s);
    DiscardAttachedDefenders();
    Tcg_SwapTurn();
    Tcg_HandleBetweenTurnKnockOuts();
}

// ---- the duel menu's rules ------------------------------------------------------------------

// TRUE: the turn holder's Active Pokémon is paralyzed or asleep (hlText: which)
bool32 Tcg_CheckIfActiveCardParalyzedOrAsleep(void)
{
    switch (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS) & TCG_CNF_SLP_PRZ)
    {
    case TCG_PARALYZED:
        gTcg->hlText = TCG_TEXT_UnableDueToParalysis;
        return TRUE;
    case TCG_ASLEEP:
        gTcg->hlText = TCG_TEXT_UnableDueToSleep;
        return TRUE;
    }
    return FALSE;
}

// TRUE: the turn holder has no Pokémon with HP on their bench (HasAlivePokemonInBench's carry)
bool32 Tcg_HasNoAlivePokemonInBench(void)
{
    u32 i, n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    for (i = 1; i < n; i++)
        if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + i))
            return FALSE;
    return TRUE;
}

// TRUE: not enough energy attached to the turn holder's Active Pokémon to retreat
// (wEnergyCardsRequiredToRetreat: the cost)
bool32 Tcg_CheckIfEnoughEnergiesToRetreat(void)
{
    u32 cost;
    Tcg_GetPlayAreaCardAttachedEnergies(TCG_PLAY_AREA_ARENA);
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    cost = Tcg_GetPlayAreaCardRetreatCost();
    gTcg->energyCardsRequiredToRetreat = cost;
    if (gTcg->totalAttachedEnergies < cost)
        return TRUE;
    gTcg->numRetreatEnergiesSelected = gTcg->totalAttachedEnergies;
    return FALSE;
}

// TRUE: the turn holder's Active Pokémon can't retreat (hlText: why)
bool32 Tcg_CheckAbleToRetreat(void)
{
    if (Tcg_CheckUnableToRetreatDueToEffect())
        return TRUE;
    if (Tcg_CheckIfActiveCardParalyzedOrAsleep())
        return TRUE;
    if (!Tcg_HasNoAlivePokemonInBench()
     && Tcg_Card(Tcg_GetCardIdFromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD)))->type != TCG_TYPE_TRAINER)
    {
        if (!Tcg_CheckIfEnoughEnergiesToRetreat())
            return FALSE;
        Tcg_LoadTxRam3(gTcg->energyCardsRequiredToRetreat);
        gTcg->hlText = TCG_TEXT_EnergyCardsRequiredToRetreat;
        return TRUE;
    }
    gTcg->hlText = TCG_TEXT_UnableToRetreat;
    return TRUE;
}

// the player picks the Energy cards to discard to retreat (hTempRetreatCostCards, $ff-terminated).
// TRUE: they decided not to.
static bool32 DisplayRetreatScreen(void)
{
    u32 pos = 0;
    gTcg->tempRetreatCostCards[0] = 0xFF;
    if (gTcg->energyCardsRequiredToRetreat == 0)
        return FALSE;
    gTcg->numRetreatEnergiesSelected = 0;
    Tcg_CreateArenaOrBenchEnergyCardList(TCG_PLAY_AREA_ARENA);
    Tcg_SortCardsInDuelTempListByID();
    Tcg_DisplayEnergyDiscardScreen(TCG_PLAY_AREA_ARENA);
    gTcg->energyDiscardMenuDenominator = gTcg->energyCardsRequiredToRetreat;
    for (;;)
    {
        u32 card;
        gTcg->energyDiscardMenuNumerator = gTcg->numRetreatEnergiesSelected;
        card = Tcg_HandleEnergyDiscardMenuInput();
        if (card == TCG_SELECTION_CANCEL)
            return TRUE;
        gTcg->tempCardIndex = card;
        Tcg_LoadCard2(card);
        gTcg->tempRetreatCostCards[pos++] = card;
        gTcg->numRetreatEnergiesSelected += gTcg->loadedCard2->type == TCG_TYPE_ENERGY_DOUBLE_COLORLESS ? 2 : 1;
        if (gTcg->numRetreatEnergiesSelected >= gTcg->energyCardsRequiredToRetreat)
        {
            gTcg->tempRetreatCostCards[pos] = 0xFF;
            return FALSE;
        }
        Tcg_RemoveCardFromDuelTempList(card);
        Tcg_DisplayEnergyDiscardMenu();
    }
}

void Tcg_DiscardRetreatCostCards(void)
{
    const u8 *c;
    for (c = gTcg->tempRetreatCostCards; *c != 0xFF; c++)
        Tcg_PutCardInDiscardPile(*c);
}

// (they're discarded for good by Tcg_AttemptRetreat)
static void ReturnRetreatCostCardsToArena(void)
{
    const u8 *c;
    for (c = gTcg->tempRetreatCostCards; *c != 0xFF; c++)
    {
        Tcg_MoveDiscardPileCardToHand(*c);
        Tcg_AddCardToHand(*c);
        Tcg_PutHandCardInPlayArea(*c, TCG_PLAY_AREA_ARENA);
    }
}

// the retreat cost discarded, and the Active Pokémon (status in hTemp_ffa0) swapped with the bench
// Pokémon at hTempPlayAreaLocation_ffa1. TRUE: a confused one failed its coin and stays.
bool32 Tcg_AttemptRetreat(void)
{
    Tcg_DiscardRetreatCostCards();
    if ((gTcg->temp & TCG_CNF_SLP_PRZ) == TCG_CONFUSED && !Tcg_TossCoin(TCG_TEXT_ConfusionCheckRetreat))
    {
        gTcg->confusionRetreatCheckWasUnsuccessful = TRUE;
        return TRUE;
    }
    Tcg_SwapArenaWithBenchPokemon(gTcg->tempPlayAreaLocationFfa1);
    gTcg->confusionRetreatCheckWasUnsuccessful = FALSE;
    return FALSE;
}

// "Attached <Energy> to <Pokémon>"
static void PrintAttachedEnergyToPokemon(void)
{
    Tcg_LoadCard1(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcg->tempPlayAreaLocation));
    gTcg->txRam2[1] = gTcg->loadedCard1->name;
    Tcg_LoadCard1(gTcg->tempCardIndex);
    gTcg->txRam2[0] = gTcg->loadedCard1->name;
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_AttachedEnergyToPokemon);
}

// "<Pokémon> evolved into <Pokémon>"
static void PrintPokemonEvolvedIntoPokemon(void)
{
    Tcg_PlaySFX(TCG_SFX_POKEMON_EVOLUTION);
    Tcg_LoadCard1(gTcg->preEvolutionPokemonCard);
    gTcg->txRam2[0] = gTcg->loadedCard1->name;
    Tcg_LoadCard1(gTcg->tempCardIndex);
    gTcg->txRam2[1] = gTcg->loadedCard1->name;
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_PokemonEvolvedIntoPokemon);
}

// TRUE: the player can't select the attack hCurMenuItem (wDuelTempList: deck index, attack index)
// for want of energy
static bool32 CheckIfEnoughEnergiesToAttack(u32 item)
{
    Tcg_GetPlayAreaCardAttachedEnergies(TCG_PLAY_AREA_ARENA);
    Tcg_HandleEnergyBurn();
    return Tcg_CheckIfEnoughEnergiesForGivenAttack(gTcg->duelTempList[item * 2], gTcg->duelTempList[item * 2 + 1]);
}

// TRUE: card `deckIndex`'s attack `attack` needs more energy than wAttachedEnergies (or isn't an attack)
bool32 Tcg_CheckIfEnoughEnergiesForGivenAttack(u32 deckIndex, u32 attack)
{
    const struct TcgAttack *a;
    u32 i, accum = 0;
    Tcg_LoadCard1(deckIndex);
    a = &gTcg->loadedCard1->attacks[attack ? 1 : 0];
    if (a->name == NULL || a->category == TCG_POKEMON_POWER)
        return TRUE;
    for (i = 0; i < TCG_NUM_COLORED_TYPES; i++)
    {
        accum += a->cost[i];
        if (a->cost[i] > gTcg->attachedEnergies[i])
            return TRUE;
    }
    return (u8)(gTcg->totalAttachedEnergies - accum) < a->cost[TCG_COLORLESS];
}

// the turn holder's Active Pokémon's attacks (not its Pokémon Power) into wDuelTempList: (deck index,
// attack index) each. The number of them.
u32 Tcg_PrintAndLoadAttacksToDuelTempList(void)
{
    u32 i, n = 0;
    if (!gTcg->headless)
        Tcg_DrawWideTextBox();
    gTcg->tempCardIndex = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD);
    Tcg_LoadCard1(gTcg->tempCardIndex);
    if (!gTcg->headless)
        gTcgGbc->cardPageNumber = 0;
    for (i = 0; i < 2; i++)
    {
        const struct TcgAttack *a = &gTcg->loadedCard1->attacks[i];
        if (a->name == NULL || (a->category & ~TCG_RESIDUAL) == TCG_POKEMON_POWER)
            continue;
        gTcg->duelTempList[n * 2] = gTcg->tempCardIndex;
        gTcg->duelTempList[n * 2 + 1] = i;
        if (!gTcg->headless)
            Tcg_PrintAttackOrPkmnPowerInformation(a, 13 + n * 2);
        n++;
    }
    return n;
}

// a Basic Pokémon from the hand to the bench, or an Evolution onto one in play (the card:
// hTempCardIndex_ff98, in wLoadedCard1). TRUE: it wasn't played.
static bool32 PlayPokemonCard(void)
{
    u32 card = gTcg->tempCardIndex, n, e;
    if (gTcg->loadedCard1->stage == TCG_BASIC)
    {
        if (*Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) >= TCG_MAX_PLAY_AREA_POKEMON)
        {
            Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_NoSpaceOnTheBench);
            return TRUE;
        }
        gTcg->temp = card;
        Tcg_PutHandPokemonCardInPlayArea(card);
        *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STAGE + gTcg->tempPlayAreaLocation) = TCG_BASIC;
        Tcg_LoadCard1(card);
        Tcg_CopyCardNameAndLevel();
        gTcg->txRam2[0] = NULL;
        Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_PlacedOnTheBench);
        Tcg_ProcessPlayedPokemonCard();
        return FALSE;
    }
    // .try_evolve
    n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    for (e = 0; e < n; e++)
        if (!Tcg_CheckIfCanEvolveInto(e, card, NULL))
            break;
    if (e == n)
    {
        u16 text = TCG_TEXT_NoPokemonCapableOfEvolving;
        for (e = 0; e < n; e++)
        {
            bool32 playedThisTurn;
            Tcg_CheckIfCanEvolveInto(e, card, &playedThisTurn);
            if (playedThisTurn)
            {
                text = TCG_TEXT_CantEvolvePokemonInSameTurnItsPlaced;
                break;
            }
        }
        Tcg_DrawWideTextBox_WaitForInput(text);
        return TRUE;
    }
    // .can_evolve (IsPrehistoricPowerActive: no Aerodactyl)
    for (;;)
    {
        u32 loc = Tcg_OpenPlayAreaScreenForSelection(FALSE);
        if (loc == TCG_SELECTION_CANCEL)
            return FALSE;   // (the game's .done: "or a")
        gTcg->tempPlayAreaLocation = loc;
        gTcg->temp = card;
        gTcg->tempPlayAreaLocationFfa1 = loc;
        gTcg->tempCardIndex = card;
        if (!Tcg_EvolvePokemonCardIfPossible())
            break;
    }
    Tcg_PrintPlayAreaCardList();
    PrintPokemonEvolvedIntoPokemon();
    Tcg_ProcessPlayedPokemonCard();
    return FALSE;
}

// an Energy card (type `type`) from the hand to a Pokémon in play: once a turn, but for Water onto
// Water Pokémon with Blastoise's Rain Dance. TRUE: back to the hand's list (it couldn't be played).
static bool32 PlayEnergyCard(u32 type)
{
    u32 loc;
    if (type == TCG_TYPE_ENERGY_WATER && Tcg_IsRainDanceActive())
    {
        loc = Tcg_OpenPlayAreaScreenForSelection(FALSE);
        if (loc == TCG_SELECTION_CANCEL)
            return FALSE;
        gTcg->tempPlayAreaLocation = loc;
        if (Tcg_CheckRainDanceScenario())
            goto play_energy;
        if (!gTcg->alreadyPlayedEnergy)
            goto play_energy_set_played;
        Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_MayOnlyAttachOneEnergyCard);
        return TRUE;
    }
    if (gTcg->alreadyPlayedEnergy)
    {
        Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_MayOnlyAttachOneEnergyCard);
        return TRUE;
    }
    loc = Tcg_OpenPlayAreaScreenForSelection(FALSE);
    if (loc == TCG_SELECTION_CANCEL)
        return FALSE;
    gTcg->tempPlayAreaLocation = loc;
play_energy_set_played:
    gTcg->alreadyPlayedEnergy = TRUE;
play_energy:
    gTcg->tempPlayAreaLocationFfa1 = gTcg->tempPlayAreaLocation;
    gTcg->temp = gTcg->tempCardIndex;
    Tcg_PutHandCardInPlayArea(gTcg->tempCardIndex, gTcg->tempPlayAreaLocation);
    Tcg_PrintPlayAreaCardList();
    PrintAttachedEnergyToPokemon();
    return FALSE;
}

// OpenPlayerHandScreen: the player's hand; a card picked is played if it can be
static u32 OpenPlayerHandScreen(void)
{
    bool32 reload = FALSE;
    Tcg_CreateHandCardList();
    for (;;)
    {
        u32 card = reload ? Tcg_ReloadHandCardList() : Tcg_DisplayHandCardList(TCG_TEXT_PleaseSelectHand);
        u32 type;
        bool32 notPlayed;
        if (card == TCG_SELECTION_CANCEL)
            return MENU_MAIN_INTERFACE;
        gTcg->tempCardIndex = card;
        Tcg_LoadCard1(card);
        type = gTcg->loadedCard1->type;
        if (type == TCG_TYPE_TRAINER)
            notPlayed = Tcg_PlayTrainerCard();
        else if (type & (1 << TCG_TYPE_ENERGY_F))
            notPlayed = PlayEnergyCard(type);
        else
            notPlayed = PlayPokemonCard();
        if (!notPlayed)
            return MENU_MAIN_INTERFACE;
        Tcg_CreateHandCardList();     // (ReloadCardListScreen)
        reload = TRUE;
    }
}

static u32 DuelMenu_Hand(void)
{
    if (*Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND) == 0)
    {
        Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_NoCardsInHand);
        return MENU_AGAIN;
    }
    return OpenPlayerHandScreen();
}

static u32 DuelMenu_Attack(void)
{
    if (Tcg_HandleCantAttackSubstatus() || Tcg_CheckIfActiveCardParalyzedOrAsleep())
    {
        Tcg_DrawWideTextBox_WaitForInput(gTcg->hlText);
        return MENU_AGAIN;
    }
    gTcg->selectedDuelSubMenuItem = 0;
    for (;;)
    {
        u32 n = Tcg_PrintAndLoadAttacksToDuelTempList(), item;
        if (n == 0)
        {
            Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_NoSelectableAttack);
            return MENU_AGAIN;
        }
        item = Tcg_HandleAttackMenuInput(n, gTcg->selectedDuelSubMenuItem);
        if (item == TCG_SELECTION_CANCEL)
            return MENU_AGAIN;
        gTcg->selectedDuelSubMenuItem = item;
        if (CheckIfEnoughEnergiesToAttack(item))
        {
            Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_NotEnoughEnergyCards);
            continue;
        }
        Tcg_CopyAttackDataAndDamage_FromDeckIndex(gTcg->duelTempList[item * 2], gTcg->duelTempList[item * 2 + 1]);
        if (Tcg_HandleAmnesiaSubstatus())
        {
            Tcg_DrawWideTextBox_WaitForInput(gTcg->hlText);
            continue;
        }
        if (Tcg_UseAttackOrPokemonPower())
            return MENU_MAIN_INTERFACE;
        return MENU_TURN_OVER;
    }
}

// the player picks a Pokémon in play whose Pokémon Power to use. TRUE: they backed out.
static bool32 DisplayPlayAreaScreenToUsePkmnPower(void)
{
    gTcg->selectedDuelSubMenuItem = 0;
    for (;;)
    {
        u32 loc = Tcg_SelectPlayAreaPokemonPower(gTcg->selectedDuelSubMenuItem), card;
        if (loc == TCG_SELECTION_CANCEL)
            return TRUE;
        gTcg->tempPlayAreaLocation = loc;
        gTcg->selectedDuelSubMenuItem = loc;
        card = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + loc);
        if (Tcg_Card(Tcg_GetCardIdFromDeckIndex(card))->attacks[0].category != TCG_POKEMON_POWER)
            continue;
        gTcg->tempCardIndex = card;
        Tcg_CopyAttackDataAndDamage_FromDeckIndex(card, 0);
        Tcg_DisplayUsePokemonPowerScreen();
        if (Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_INITIAL_EFFECT_1))
        {
            Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_PokemonPowerSelectNotRequired);
            continue;
        }
        if (Tcg_YesOrNoMenuWithText(TCG_TEXT_UseThisPokemonPower))
            continue;
        gTcg->temp = gTcg->tempCardIndex;
        return FALSE;
    }
}

static u32 DuelMenu_PkmnPower(void)
{
    if (!DisplayPlayAreaScreenToUsePkmnPower())
        Tcg_UseAttackOrPokemonPower();
    return MENU_MAIN_INTERFACE;
}

static u32 DuelMenu_Retreat(void)
{
    u32 status = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS) & TCG_CNF_SLP_PRZ, loc;
    gTcg->temp = status;
    if (status == TCG_CONFUSED)
    {
        if (gTcg->confusionRetreatCheckWasUnsuccessful)
            goto unable_due_to_confusion;
        if (Tcg_CheckAbleToRetreat())
            goto unable_to_retreat;
        if (DisplayRetreatScreen())
            return MENU_MAIN_INTERFACE;
        Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_SelectPkmnOnBenchToSwitchWithActive);
        loc = Tcg_OpenPlayAreaScreenForSelection(TRUE);
        if (loc == TCG_SELECTION_CANCEL)
            return MENU_MAIN_INTERFACE;
        gTcg->tempPlayAreaLocationFfa1 = loc;
        if (!Tcg_AttemptRetreat())
            return MENU_MAIN_INTERFACE;
        Tcg_DrawDuelMainScene();
    unable_due_to_confusion:
        Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_UnableToRetreat);
        return MENU_AGAIN;
    }
    // (the energy is discarded, then put back, then discarded for good by AttemptRetreat: so the
    // play area screen shows the Pokémon without it)
    if (Tcg_CheckAbleToRetreat())
        goto unable_to_retreat;
    if (DisplayRetreatScreen())
        return MENU_MAIN_INTERFACE;
    Tcg_DiscardRetreatCostCards();
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_SelectPkmnOnBenchToSwitchWithActive);
    loc = Tcg_OpenPlayAreaScreenForSelection(TRUE);
    gTcg->tempPlayAreaLocationFfa1 = loc;
    ReturnRetreatCostCardsToArena();
    if (loc == TCG_SELECTION_CANCEL)
        return MENU_MAIN_INTERFACE;
    Tcg_AttemptRetreat();
    return MENU_MAIN_INTERFACE;

unable_to_retreat:
    Tcg_DrawWideTextBox_WaitForInput(gTcg->hlText);
    return MENU_AGAIN;
}

// the player's turn: the duel menu until they attack or are done (DuelMainInterface,
// PrintDuelMenuAndHandleInput)
static void PlayerTurn(void)
{
    u32 next = MENU_MAIN_INTERFACE;
    for (;;)
    {
        if (next == MENU_MAIN_INTERFACE)
            Tcg_DrawDuelMainScene();
        if (gTcg->duelFinished)
            return;
        switch (Tcg_HandleDuelMenuInput())
        {
        case TCG_DUELMENU_HAND:
            next = DuelMenu_Hand();
            break;
        case TCG_DUELMENU_ATTACK:
            next = DuelMenu_Attack();
            break;
        case TCG_DUELMENU_CHECK:
            Tcg_FinishQueuedAnimations();
            Tcg_OpenDuelCheckMenu();
            next = MENU_MAIN_INTERFACE;
            break;
        case TCG_DUELMENU_PKMN_POWER:
            next = DuelMenu_PkmnPower();
            break;
        case TCG_DUELMENU_RETREAT:
            next = DuelMenu_Retreat();
            break;
        case TCG_DUELMENU_DONE:
        default:
            Tcg_ClearNonTurnTemporaryDuelvars();
            next = MENU_TURN_OVER;
            break;
        }
        if (next == MENU_TURN_OVER)
            return;
    }
}

// ---- the turns ------------------------------------------------------------------------------

// the turn holder's turn: they draw (an empty deck loses), then the player's menu or the AI
static void HandleTurn(void)
{
    u8 card;
    gTcg->duelistType = *Tcg_TurnVar(TCG_DUELVARS_DUELIST_TYPE);
    if (gTcg->duelTurns >= 2)
        Tcg_SetAllPlayAreaPokemonCanEvolve();
    Tcg_InitVariablesToBeginTurn();
    Tcg_DisplayDrawOneCardScreen();
    if (Tcg_DrawCardFromDeck(&card))
    {
        gTcg->duelFinished = TCG_TURN_PLAYER_LOST;
        return;
    }
    gTcg->tempCardIndex = card;
    Tcg_AddCardToHand(card);
    if (gTcg->duelistType == TCG_DUELIST_TYPE_PLAYER)
    {
        Tcg_DisplayPlayerDrawCardScreen();
        PlayerTurn();
        return;
    }
    // (IsClairvoyanceActive: no Omanyte)
    Tcg_DrawDuelMainScene();
    gTcg->skipDuelistIsThinkingDelay = FALSE;
    Tcg_DuelistIsThinking();
    TcgAi_DoTurn();
    gTcg->playerAttackingCardIndex = 0xFF;
    gTcg->playerAttackingAttackIndex = 0xFF;
}

// StartDuel: a duel between the decks in gTcg->deck (the duelists' types set), to its end, with
// sudden death on a tie. TCG_DUEL_WIN or TCG_DUEL_LOSS: the player's result.
u32 Tcg_StartDuel(void)
{
    gTcg->whoseTurn = TCG_PLAYER;
    Tcg_SetupDuel();
    Tcg_InitVariablesToBeginDuel();
    Tcg_HandleDuelSetup();
    for (;;)
    {
        u16 text;
        // MainDuelLoop
        Tcg_UpdateSubstatusConditions_StartOfTurn();
        Tcg_DisplayDuelistTurnScreen();
        HandleTurn();
        if (!gTcg->duelFinished)
        {
            Tcg_UpdateSubstatusConditions_EndOfTurn();
            Tcg_HandleBetweenTurnsEvents();
            Tcg_FinishQueuedAnimations();
            if (!gTcg->duelFinished)
            {
                gTcg->duelTurns++;
                Tcg_SwapTurn();
                continue;
            }
        }
        // .duel_finished
        if (gTcg->duelFinished == TCG_TURN_PLAYER_TIED)
        {
            text = TCG_TEXT_DuelWasADraw;
        }
        else if ((gTcg->duelFinished == TCG_TURN_PLAYER_WON) == (gTcg->whoseTurn == TCG_PLAYER))
        {
            gTcg->duelResult = TCG_DUEL_WIN;
            text = TCG_TEXT_WonDuel;
        }
        else
        {
            gTcg->duelResult = TCG_DUEL_LOSS;
            text = TCG_TEXT_LostDuel;
        }
        Tcg_DisplayDuelResult(gTcg->duelFinished, text);
        if (gTcg->duelFinished != TCG_TURN_PLAYER_TIED)
        {
            gTcg->whoseTurn = TCG_PLAYER;
            return gTcg->duelResult;
        }
        // .tied_duel: sudden death, one prize each (to the duel's music again: wDuelTheme, Rod's)
        Tcg_PlaySong(TCG_MUSIC_DUEL_THEME_3);
        Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_StartSuddenDeathMatch);
        gTcg->duelInitialPrizes = 1;
        Tcg_InitVariablesToBeginDuel();
        gTcg->whoseTurn = TCG_PLAYER;
        Tcg_HandleDuelSetup();
    }
}

// a duel's decks: each duelist's 60 cards (deck lists of TCG_CARD_*)
void Tcg_SetDecks(const u8 *playerDeck, const u8 *opponentDeck)
{
    memcpy(gTcg->deck[TCG_PLAYER], playerDeck, TCG_DECK_SIZE);
    memcpy(gTcg->deck[TCG_OPPONENT], opponentDeck, TCG_DECK_SIZE);
}

// ---- the AI's actions -----------------------------------------------------------------------

static void OppAction_PlayEnergyCard(void)
{
    gTcg->tempPlayAreaLocation = gTcg->tempPlayAreaLocationFfa1;
    gTcg->tempCardIndex = gTcg->temp;
    Tcg_PutHandCardInPlayArea(gTcg->temp, gTcg->tempPlayAreaLocationFfa1);
    Tcg_LoadCard1(gTcg->temp);
    Tcg_DrawLargePictureOfCard();
    PrintAttachedEnergyToPokemon();
    gTcg->alreadyPlayedEnergy = TRUE;
    Tcg_DrawDuelMainScene();
}

static void OppAction_EvolvePokemonCard(void)
{
    gTcg->tempPlayAreaLocation = gTcg->tempPlayAreaLocationFfa1;
    gTcg->tempCardIndex = gTcg->temp;
    Tcg_LoadCard1(gTcg->temp);
    Tcg_DrawLargePictureOfCard();
    Tcg_EvolvePokemonCardIfPossible();
    PrintPokemonEvolvedIntoPokemon();
    Tcg_ProcessPlayedPokemonCard();
    Tcg_DrawDuelMainScene();
}

static void OppAction_PlayBasicPokemonCard(void)
{
    gTcg->tempCardIndex = gTcg->temp;
    Tcg_PutHandPokemonCardInPlayArea(gTcg->temp);
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STAGE + gTcg->tempPlayAreaLocation) = TCG_BASIC;
    Tcg_DisplayCardDetailScreen(gTcg->temp, TCG_TEXT_PlacedOnTheBench);
    Tcg_ProcessPlayedPokemonCard();
    Tcg_DrawDuelMainScene();
}

static void OppAction_AttemptRetreat(void)
{
    u8 retreating = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD);
    u16 text = Tcg_AttemptRetreat() ? TCG_TEXT_RetreatWasUnsuccessful : TCG_TEXT_RetreatedToTheBench;
    Tcg_DrawDuelMainScene();
    Tcg_LoadCard1(retreating);
    Tcg_LoadTxRam2(gTcg->loadedCard1->name);
    Tcg_DrawWideTextBox_WaitForInput(text);
}

static void OppAction_FinishTurnWithoutAttacking(void)
{
    Tcg_DrawDuelMainScene();
    Tcg_ClearNonTurnTemporaryDuelvars();
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_FinishedTurnWithoutAttacking);
    gTcg->opponentTurnEnded = TRUE;
}

static void OppAction_PlayTrainerCard(void)
{
    Tcg_LoadNonPokemonCardEffectCommands();
    Tcg_DisplayUsedTrainerCardDetailScreen();
    Tcg_PrintUsedTrainerCardDescription();
    gTcg->skipDuelistIsThinkingDelay = TRUE;
}

static void OppAction_ExecuteTrainerCardEffectCommands(void)
{
    Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_DISCARD_ENERGY);
    Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_BEFORE_DAMAGE);
    Tcg_DrawDuelMainScene();
    Tcg_MoveHandCardToDiscardPile(gTcg->tempCardIndexFf9f);
    Tcg_DrawDuelMainScene();
}

static void OppAction_BeginUseAttack(void)
{
    Tcg_CopyAttackDataAndDamage_FromDeckIndex(gTcg->tempCardIndexFf9f, gTcg->temp);
    Tcg_UpdateArenaCardIDsAndClearTwoTurnDuelVars();
    gTcg->skipDuelistIsThinkingDelay = TRUE;
    if (!Tcg_CheckSandAttackOrSmokescreenSubstatus()
     && (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS) & TCG_CNF_SLP_PRZ) != TCG_CONFUSED)
        return;
    Tcg_DrawDuelMainScene();
    Tcg_PrintPokemonsAttackText();
    Tcg_WaitForWideTextBoxInput();
    if (!Tcg_HandleSandAttackOrSmokescreenSubstatus())
        return;
    Tcg_ClearNonTurnTemporaryDuelvars();
    gTcg->opponentTurnEnded = TRUE;
}

static void OppAction_UseAttack(void)
{
    Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_DISCARD_ENERGY);
    if (Tcg_CheckSelfConfusionDamage())
    {
        Tcg_HandleConfusionDamageToSelf();
        gTcg->opponentTurnEnded = TRUE;
        return;
    }
    Tcg_DisplayOpponentUsedAttackScreen();
    Tcg_PrintPokemonsAttackText();
    Tcg_WaitForWideTextBoxInput();
    gTcg->skipDuelistIsThinkingDelay = TRUE;
}

static void OppAction_UsePokemonPower(void)
{
    Tcg_CopyAttackDataAndDamage_FromDeckIndex(gTcg->tempCardIndexFf9f, 0);
    gTcg->tempPlayAreaLocation = gTcg->temp;
    Tcg_DisplayUsePokemonPowerScreen();
    Tcg_LoadCard1(gTcg->tempCardIndexFf9f);
    gTcg->txRam2[0] = gTcg->loadedCard1->name;
    gTcg->txRam2[1] = gTcg->loadedAttack.name;
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_WillUseThePokemonPower);
    gTcg->skipDuelistIsThinkingDelay = TRUE;
}

// AIMakeDecision: the AI does action OPPACTION_* (with hTemp_ffa0, hTempPlayAreaLocation_ffa1,
// hTempCardIndex_ff9f as the game's AI sets them). TRUE: its turn's over (or the duel is).
bool32 Tcg_AIMakeDecision(u32 action)
{
    if (!gTcg->skipDuelistIsThinkingDelay)
        Tcg_AIThinkingDelay();
    gTcg->skipDuelistIsThinkingDelay = FALSE;
    gTcg->opponentTurnEnded = FALSE;
    switch (action)
    {
    case TCG_OPPACTION_PLAY_BASIC_PKMN:
        OppAction_PlayBasicPokemonCard();
        break;
    case TCG_OPPACTION_EVOLVE_PKMN:
        OppAction_EvolvePokemonCard();
        break;
    case TCG_OPPACTION_PLAY_ENERGY:
        OppAction_PlayEnergyCard();
        break;
    case TCG_OPPACTION_ATTEMPT_RETREAT:
        OppAction_AttemptRetreat();
        break;
    case TCG_OPPACTION_FINISH_NO_ATTACK:
        OppAction_FinishTurnWithoutAttacking();
        break;
    case TCG_OPPACTION_PLAY_TRAINER:
        OppAction_PlayTrainerCard();
        break;
    case TCG_OPPACTION_EXECUTE_TRAINER_EFFECTS:
        OppAction_ExecuteTrainerCardEffectCommands();
        break;
    case TCG_OPPACTION_BEGIN_ATTACK:
        OppAction_BeginUseAttack();
        break;
    case TCG_OPPACTION_USE_ATTACK:
        OppAction_UseAttack();
        break;
    case TCG_OPPACTION_ATTACK_ANIM_AND_DAMAGE:
        Tcg_PlayAttackAnimation_DealAttackDamage();
        gTcg->opponentTurnEnded = TRUE;
        break;
    case TCG_OPPACTION_DRAW_CARD:
    {
        u8 card;
        if (!Tcg_DrawCardFromDeck(&card))
            Tcg_AddCardToHand(card);
        break;
    }
    case TCG_OPPACTION_USE_PKMN_POWER:
        OppAction_UsePokemonPower();
        break;
    case TCG_OPPACTION_EXECUTE_PKMN_POWER_EFFECT:
        Tcg_ResetAttackAnimationIsPlaying();
        Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_BEFORE_DAMAGE);
        gTcg->skipDuelistIsThinkingDelay = TRUE;
        break;
    case TCG_OPPACTION_6B15:
        Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_AFTER_DAMAGE);
        gTcg->skipDuelistIsThinkingDelay = TRUE;
        break;
    case TCG_OPPACTION_DUEL_MAIN_SCENE:
        Tcg_DrawDuelMainScene();
        break;
    }
    if (gTcg->duelFinished || gTcg->opponentTurnEnded)
        return TRUE;
    if (!gTcg->skipDuelistIsThinkingDelay)
        Tcg_DuelistIsThinking();
    return FALSE;
}
