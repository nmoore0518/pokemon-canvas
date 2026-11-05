// Pokémon Canvas: the Trading Card Game AI's Pokémon playing and retreating, ported from the Game Boy
// Color game's engine/duel/ai/hand_pokemon.asm and retreat.asm (pret/poketcg) (TcgAi_ prefixed).
#include "global.h"
#include "tcg.h"
#include "tcg_ai.h"

static inline u32 TurnCardId(u32 deckIndex)
{
    return Tcg_GetCardIdFromDeckIndex(deckIndex);
}

static void AddScore(u32 a)
{
    if (a >= 0x80)
        TcgAi_Encourage(a - 0x80);
    else
        TcgAi_Discourage(0x80 - a);
}

// ---- hand_pokemon.asm -----------------------------------------------------------------------

// Rod's evolutions he's choosy about (the Pokémon about to evolve: wLoadedCard2): Charmeleon to
// Charizard with the energy for Fire Spin, a benched Magikarp with energy, Dragonair to Dragonite when
// Healing Wind would heal a lot
static void AIDecideSpecialEvolutions(void)
{
    u32 loc = gTcg->tempPlayAreaLocation, e, total;
    if (gTcgAi->deckId != TCG_DECKID_LEGENDARY_DRAGONITE)
        return;     // (Invincible Ronald's and Legendary Ronald's aren't here)
    switch (gTcg->loadedCard2Id)
    {
    case TCG_CARD_CHARMELEON:
    {
        u32 n = TcgAi_CountNumberOfEnergyCardsAttached(loc);
        if (n >= 3 && (u8)(n + TcgAi_CountOppEnergyCardsInHand()) >= 6)
            TcgAi_Encourage(3);
        else
            TcgAi_Discourage(10);
        return;
    }
    case TCG_CARD_MAGIKARP:
        if (loc == TCG_PLAY_AREA_ARENA)
            return;
        if (TcgAi_CountNumberOfEnergyCardsAttached(loc) >= 2)
            TcgAi_Encourage(3);
        return;
    case TCG_CARD_DRAGONAIR:
        if (loc != TCG_PLAY_AREA_ARENA)
        {
            // (all the damage in play: over 70, Healing Wind's worth it)
            for (total = 0, e = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA); e > 0; e--)
                total = (u8)(total + Tcg_GetCardDamageAndMaxHP(e - 1, NULL));
            if (total > 70)
                TcgAi_Encourage(10);    // (Muk's Toxic Gas: no Muk)
            else
                TcgAi_Discourage(10);
            return;
        }
        if (Tcg_GetCardDamageAndMaxHP(TCG_PLAY_AREA_ARENA, NULL) < 50)
        {
            TcgAi_Discourage(10);
            return;
        }
        Tcg_GetPlayAreaCardAttachedEnergies(TCG_PLAY_AREA_ARENA);
        if (gTcg->totalAttachedEnergies < 3)
            TcgAi_Discourage(10);
        else
            TcgAi_Encourage(10);
        return;
    }
}

// each Evolution card in the hand scored for each Pokémon it can evolve, and played if 133 or more
static bool32 AIDecideEvolution(void)
{
    u8 hand[TCG_DUEL_TEMP_LIST_SIZE];
    const u8 *h;
    Tcg_CreateHandCardList();
    memcpy(hand, gTcg->duelTempList, sizeof(hand));
    for (h = hand; *h != 0xFF; h++)
    {
        u32 b, n;
        gTcgAi->tempAIPokemonCard = *h;
        if (Tcg_IsPrehistoricPowerActive())
            continue;
        Tcg_LoadCard1(*h);
        if (gTcg->loadedCard1->type >= TCG_TYPE_ENERGY || gTcg->loadedCard1->stage == TCG_BASIC)
            continue;
        n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
        for (b = 0; b < n; b++)
        {
            u8 *slot, saved;
            if (Tcg_CheckIfCanEvolveInto(b, gTcgAi->tempAIPokemonCard, NULL))
                continue;
            gTcgAi->tempAI = b;
            gTcg->tempPlayAreaLocation = b;
            gTcgAi->score = 0x80;
            AIDecideSpecialEvolutions();
            // the Pokémon as it is: can it attack, knock out?
            gTcg->selectedAttack = 0;
            if (TcgAi_CheckIfSelectedAttackIsUnusable())
            {
                gTcg->selectedAttack = 1;
                if (TcgAi_CheckIfSelectedAttackIsUnusable())
                {
                    gTcgAi->curCardCanAttack = 0;
                    gTcgAi->curCardCanKO = 0;
                    goto check_evolution_attacks;
                }
            }
            gTcgAi->curCardCanAttack = 1;
            if (TcgAi_CheckIfAnyAttackKnocksOutDefendingCard() && !TcgAi_CheckIfSelectedAttackIsUnusable())
                gTcgAi->curCardCanKO = 1;
        check_evolution_attacks:
            // the evolution in its place, for a moment
            slot = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcg->tempPlayAreaLocation);
            saved = *slot;
            *slot = gTcgAi->tempAIPokemonCard;
            gTcg->selectedAttack = 0;
            if (!TcgAi_CheckIfSelectedAttackIsUnusable() || (gTcg->selectedAttack = 1, !TcgAi_CheckIfSelectedAttackIsUnusable()))
            {
                TcgAi_Encourage(5);
            }
            else if (gTcgAi->curCardCanAttack)
            {
                TcgAi_Discourage(2);
                if (!gTcg->alreadyPlayedEnergy && TcgAi_LookForEnergyNeededInHand())
                    TcgAi_Encourage(7);
            }
            // .check_evolution_ko
            if (gTcgAi->curCardCanAttack && gTcgAi->tempAI == TCG_PLAY_AREA_ARENA)
            {
                if (TcgAi_CheckIfAnyAttackKnocksOutDefendingCard() && !TcgAi_CheckIfSelectedAttackIsUnusable())
                    TcgAi_Encourage(5);
                else if (gTcgAi->curCardCanKO)
                    TcgAi_Discourage(20);
            }
            if (gTcgAi->tempAI == TCG_PLAY_AREA_ARENA)
            {
                gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
                if (TcgAi_CheckIfDefendingPokemonCanKnockOut(NULL))
                    TcgAi_Discourage(5);
            }
            if (!TcgAi_CheckDamageToMrMime(gTcgAi->tempAI))
                TcgAi_Discourage(20);
            *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcgAi->tempAI) = saved;
            // the Pokémon as it is again: in danger, or statused?
            if (gTcgAi->tempAI == TCG_PLAY_AREA_ARENA)
            {
                gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
                if (TcgAi_CheckIfDefendingPokemonCanKnockOut(NULL))
                    TcgAi_Encourage(5);
                if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS))
                    TcgAi_Encourage(4);
            }
            // a Stage 2 to come
            if (TcgAi_CheckForEvolutionInList(gTcgAi->tempAIPokemonCard) != 0xFF)
                TcgAi_Encourage(2);
            else if (TcgAi_CheckForEvolutionInDeck(gTcgAi->tempAIPokemonCard) != 0xFF)
                TcgAi_Encourage(1);
            // damage on it (a quarter's worth of counters)
            {
                u32 damage = Tcg_GetCardDamageAndMaxHP(gTcgAi->tempAI, NULL);
                if (damage)
                    TcgAi_Discourage(TcgAi_ConvertHPToDamageCounters(damage >> 2));
            }
            Tcg_LoadCard1(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcgAi->tempAI));
            // (Mysterious Fossil and the Pikachu deck's aren't here)
            if (gTcg->loadedCard1->aiInfo == TCG_AI_INFO_ENCOURAGE_EVO)
                TcgAi_Encourage(2);
            if (gTcgAi->score >= 133)
            {
                gTcg->tempPlayAreaLocationFfa1 = gTcgAi->tempAI;
                gTcg->temp = gTcgAi->tempAIPokemonCard;
                Tcg_AIMakeDecision(TCG_OPPACTION_EVOLVE_PKMN);
                break;
            }
        }
    }
    return FALSE;
}

// each Basic in the hand scored, played if 180 or more; then the evolutions. TRUE: the turn's over.
bool32 TcgAi_DecidePlayPokemonCard(void)
{
    u8 hand[TCG_DUEL_TEMP_LIST_SIZE];
    const u8 *h;
    Tcg_CreateHandCardList();
    TcgAi_SortTempHandByIDList();
    memcpy(hand, gTcg->duelTempList, sizeof(hand));
    for (h = hand; *h != 0xFF; h++)
    {
        gTcgAi->tempAIPokemonCard = *h;
        Tcg_LoadCard1(*h);
        if (gTcg->loadedCard1->type >= TCG_TYPE_ENERGY || gTcg->loadedCard1->stage != TCG_BASIC)
            continue;
        gTcgAi->score = 130;
        // (AIDecidePlayLegendaryBirds: the Legendary bird decks')
        if (*Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) >= 4)
            TcgAi_Discourage(20);
        else
            TcgAi_Encourage(50);
        gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
        if (TcgAi_CheckIfDefendingPokemonCanKnockOut(NULL))
            TcgAi_Encourage(20);
        if (TcgAi_CheckEnergyFlagsNeededInList(TcgAi_GetAttacksEnergyCostBits(gTcgAi->tempAIPokemonCard)))
            TcgAi_Encourage(20);
        if (TcgAi_CheckForEvolutionInList(gTcgAi->tempAIPokemonCard) != 0xFF)
            TcgAi_Encourage(20);
        if (TcgAi_CheckForEvolutionInDeck(gTcgAi->tempAIPokemonCard) != 0xFF)
            TcgAi_Encourage(10);
        if (gTcgAi->score < 180)
            continue;
        gTcg->temp = gTcgAi->tempAIPokemonCard;
        if (TcgAi_CheckIfCardCanBePlayed(gTcgAi->tempAIPokemonCard))
            continue;
        if (Tcg_AIMakeDecision(TCG_OPPACTION_PLAY_BASIC_PKMN))
            return TRUE;
    }
    return AIDecideEvolution();
}

// ---- retreat.asm ----------------------------------------------------------------------------

// TRUE: the AI wants its Active Pokémon to retreat (its score 131 or more)
bool32 TcgAi_DecideWhetherToRetreat(void)
{
    const u8 *b;
    u32 e, cost, id;
    if (gTcg->confusionRetreatCheckWasUnsuccessful)
        return FALSE;
    gTcgAi->playEnergyCardForRetreat = FALSE;
    TcgAi_LoadDefendingPokemonColorWRAndPrizeCards();
    gTcgAi->score = 0x80;
    if (gTcgAi->retreatScore)
        TcgAi_Encourage((gTcgAi->retreatScore >> 2) << 1);
    {
        u8 status = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS);
        if (status)
        {
            if (status & TCG_DOUBLE_POISONED)
                TcgAi_Encourage(2);
            if ((status & TCG_CNF_SLP_PRZ) == TCG_CONFUSED)
                TcgAi_Encourage(1);
        }
    }
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    if (TcgAi_CheckIfAnyAttackKnocksOutDefendingCard()
     && (!TcgAi_CheckIfSelectedAttackIsUnusable() || TcgAi_LookForEnergyNeededForAttackInHand()))
    {
        // (it can knock out the defender: stay)
        TcgAi_Discourage(5);
        if (gTcgAi->opponentPrizeCount < 2)
            TcgAi_Discourage(35);
    }
    // .active_cant_ko_1
    if (TcgAi_CheckIfDefendingPokemonCanKnockOut(NULL))
    {
        TcgAi_Encourage(2);
        if (TcgAi_CheckIfNotABossDeckID())
            goto check_resistance_1;
        if (gTcgAi->playerPrizeCount >= 2)
            goto check_prize_count;
        gTcgAi->playEnergyCardForRetreat = TRUE;
    }
    // .defending_cant_ko
    if (TcgAi_CheckIfNotABossDeckID())
        goto check_resistance_1;
    if (gTcgAi->playerPrizeCount < 2)
        TcgAi_Encourage(2);
    else
        goto check_prize_count;
check_prize_count:
    if (gTcgAi->opponentPrizeCount < 2)
        TcgAi_Discourage(2);
check_resistance_1:
    // the defender resists it: better if a benched one isn't resisted
    if (gTcgAi->playerResistance & Tcg_TranslateColorToWR(Tcg_GetArenaCardColor()))
    {
        TcgAi_Encourage(1);
        for (b = Tcg_TurnVar(TCG_DUELVARS_BENCH);; b++)
        {
            if (*b == 0xFF)
            {
                TcgAi_Discourage(2);
                break;
            }
            Tcg_LoadCard1(*b);
            if (!(Tcg_TranslateColorToWR(gTcg->loadedCard1->type) & gTcgAi->playerResistance))
                break;
        }
    }
    // it's weak to the defender: better if a benched one isn't
    if (Tcg_GetArenaCardWeakness() & gTcgAi->playerColor)
    {
        TcgAi_Encourage(2);
        for (b = Tcg_TurnVar(TCG_DUELVARS_BENCH);; b++)
        {
            if (*b == 0xFF)
            {
                TcgAi_Discourage(3);
                break;
            }
            Tcg_LoadCard1(*b);
            if (!(gTcg->loadedCard1->weakness & gTcgAi->playerColor))
                break;
        }
    }
    // it resists the defender
    if (Tcg_GetArenaCardResistance() & gTcgAi->playerColor)
        TcgAi_Discourage(3);
    // .check_weakness_2: a benched one the defender's weak to
    for (e = 1, b = Tcg_TurnVar(TCG_DUELVARS_BENCH); *b != 0xFF; b++, e++)
    {
        Tcg_LoadCard1(*b);
        if (!(Tcg_TranslateColorToWR(gTcg->loadedCard1->type) & gTcgAi->playerWeakness))
            continue;
        TcgAi_Encourage(2);
        // (Porygon's: not in our decks)
        if (Tcg_TranslateColorToWR(Tcg_GetArenaCardColor()) & gTcgAi->playerWeakness)
            TcgAi_Discourage(3);
        break;
    }
    // .check_resistance_3: a benched one that resists the defender
    for (b = Tcg_TurnVar(TCG_DUELVARS_BENCH); *b != 0xFF; b++)
    {
        Tcg_LoadCard1(*b);
        if (gTcg->loadedCard1->resistance & gTcgAi->playerColor)
        {
            TcgAi_Encourage(1);
            break;
        }
    }
    // .check_ko_2: a benched one that can knock out the defender
    for (e = 1; e < TCG_MAX_PLAY_AREA_POKEMON && *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + e) != 0xFF; e++)
    {
        gTcg->tempPlayAreaLocation = e;
        if (!TcgAi_CheckIfAnyAttackKnocksOutDefendingCard())
            continue;
        if (TcgAi_CheckIfSelectedAttackIsUnusable() && !TcgAi_LookForEnergyNeededForAttackInHand())
            continue;
        TcgAi_Encourage(2);
        if (gTcgAi->opponentPrizeCount >= 2 || TcgAi_CheckIfNotABossDeckID())
            break;
        gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
        if (TcgAi_CheckIfAnyAttackKnocksOutDefendingCard() && !TcgAi_CheckIfSelectedAttackIsUnusable())
            break;
        TcgAi_Encourage(40);
        gTcgAi->playEnergyCardForRetreat = TRUE;
        break;
    }
    // .check_defending_id: Mr. Mime (or Hitmonlee) it can't hurt, but a benched one can
    Tcg_SwapTurn();
    id = TurnCardId(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
    Tcg_SwapTurn();
    if ((id == TCG_CARD_MR_MIME || id == TCG_CARD_HITMONLEE) && !TcgAi_CheckIfCanDamageDefendingPokemon(TCG_PLAY_AREA_ARENA))
    {
        for (e = 1; e < TCG_MAX_PLAY_AREA_POKEMON && *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + e) != 0xFF; e++)
        {
            if (TcgAi_CheckIfCanDamageDefendingPokemon(e))
            {
                TcgAi_Encourage(5);
                gTcgAi->playEnergyCardForRetreat = TRUE;
                break;
            }
        }
    }
    // .check_retreat_cost
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    cost = Tcg_GetPlayAreaCardRetreatCost();
    if (cost >= 3)
        TcgAi_Discourage(2);
    else if (cost == 2)
        TcgAi_Discourage(1);
    if (!TcgAi_CheckIfArenaCardIsFullyPowered())
    {
        u32 setUp = TcgAi_CountNumberOfSetUpBenchPokemon();
        if (setUp >= 2)
            TcgAi_Encourage(setUp);
    }
    // .check_defending_can_ko: every benched Pokémon would be knocked out too: stay
    for (e = 1; e < TCG_MAX_PLAY_AREA_POKEMON && *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + e) != 0xFF; e++)
    {
        gTcg->tempPlayAreaLocation = e;
        if (!TcgAi_CheckIfDefendingPokemonCanKnockOut(NULL))
            break;
    }
    if (e >= TCG_MAX_PLAY_AREA_POKEMON || *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + e) == 0xFF)
        TcgAi_Discourage(20);
    // (Mysterious Fossil, Clefairy Doll: not here)
    return gTcgAi->score >= 131;
}

// wAIRetreatFlags: bit 7, a switch from an effect (not a Pokémon Power) or before the AI attacked
static void SetAIRetreatFlags(void)
{
    gTcgAi->retreatFlags = 0;
    if (gTcg->whoseTurnReal == gTcg->whoseTurn)
    {
        // (the AI's own turn)
        if (gTcgAi->triedAttack)
            return;
    }
    else if (gTcg->loadedAttack.category == TCG_POKEMON_POWER)
    {
        return;
    }
    gTcgAi->retreatFlags = 0x80;
}

static void HandleAttackDamageScore(void)
{
    TcgAi_EstimateDamage_VersusDefendingCard(gTcg->selectedAttack);
    TcgAi_Encourage(TcgAi_ConvertHPToDamageCounters(gTcg->damage) + 1);
}

// the benched Pokémon the AI would switch in (each scored from 50): a PLAY_AREA_* (0xFF: none)
u32 TcgAi_DecideBenchPokemonToSwitchTo(void)
{
    u32 n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA), c;
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    if (n < 2)
        return 0xFF;
    SetAIRetreatFlags();
    TcgAi_LoadDefendingPokemonColorWRAndPrizeCards();
    gTcgAi->score = 50;
    gTcgAi->playAreaAIScore[TCG_PLAY_AREA_ARENA] = gTcgAi->score;
    for (c = 1; c < n; c++)
    {
        u32 id, hp, defending;
        const u8 *l;
        gTcg->tempPlayAreaLocation = c;
        gTcgAi->score = 50;
        if (TcgAi_CheckIfAnyAttackKnocksOutDefendingCard() && !TcgAi_CheckIfSelectedAttackIsUnusable())
        {
            TcgAi_Encourage(10);
            gTcgAi->retreatFlags |= 1;
            if (Tcg_CountPrizes() >= 2)
                goto check_defending_weak;
            TcgAi_Encourage(10);
        }
        // .check_can_use_atks
        gTcg->selectedAttack = 0;
        if (!TcgAi_CheckIfSelectedAttackIsUnusable())
            HandleAttackDamageScore();
        gTcg->selectedAttack = 1;
        if (!TcgAi_CheckIfSelectedAttackIsUnusable())
            HandleAttackDamageScore();
        if (TcgAi_LookForEnergyNeededInHand())
        {
            TcgAi_EstimateDamage_VersusDefendingCard(gTcg->selectedAttack);
            TcgAi_Encourage(TcgAi_ConvertHPToDamageCounters(gTcg->damage) >> 1);
        }
        Tcg_GetPlayAreaCardAttachedEnergies(gTcg->tempPlayAreaLocation);
        if (gTcg->totalAttachedEnergies == 0)
            TcgAi_Discourage(1);
        // a Mr. Mime it can hurt
        Tcg_SwapTurn();
        Tcg_LoadCard2(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
        defending = gTcg->loadedCard2Id;
        Tcg_SwapTurn();
        if (defending == TCG_CARD_MR_MIME)
        {
            TcgAi_EstimateDamage_VersusDefendingCard(0);
            if (!(u8)gTcg->damage)
                TcgAi_EstimateDamage_VersusDefendingCard(1);
            if ((u8)gTcg->damage)
                TcgAi_Encourage(5);
        }
    check_defending_weak:
        Tcg_LoadCard1(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcg->tempPlayAreaLocation));
        {
            u32 wr = Tcg_TranslateColorToWR(gTcg->loadedCard1->type);
            if (wr & gTcgAi->playerWeakness)
                TcgAi_Encourage(3);
            if (wr & gTcgAi->playerResistance)
                TcgAi_Discourage(2);
        }
        if (gTcgAi->playerColor & gTcg->loadedCard1->resistance)
            TcgAi_Encourage(2);
        if (gTcgAi->playerColor & gTcg->loadedCard1->weakness)
            TcgAi_Discourage(3);
        // .check_retreat_cost
        {
            u32 cost = Tcg_GetPlayAreaCardRetreatCost();
            if (cost < 2)
                TcgAi_Encourage(1);
            else if (cost > 2)
                TcgAi_Discourage(1);
        }
        if (gTcgAi->retreatFlags != (0x80 | 1) && TcgAi_CheckIfDefendingPokemonCanKnockOut(NULL))
            TcgAi_Discourage(gTcgAi->playerPrizeCount == 1 ? 10 : 3);
        // .check_hp
        hp = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + gTcg->tempPlayAreaLocation);
        if (hp == 0)
        {
            gTcgAi->score = 0;
            goto store_score;
        }
        TcgAi_Encourage(TcgAi_ConvertHPToDamageCounters(TcgAi_CalculateBDividedByA(hp, 4)));
        Tcg_LoadCard1(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcg->tempPlayAreaLocation));
        id = gTcg->loadedCard1Id;
        if (id == TCG_CARD_MR_MIME)     // (Mew Lv8's: not here)
            TcgAi_Encourage(5);
        if (gTcg->loadedCard1->aiInfo == TCG_AI_INFO_BENCH_UTILITY)
            TcgAi_Discourage(2);
        // (Mysterious Fossil, Clefairy Doll: not here)
        for (l = gTcgAi->cardListRetreatBonus; l && *l; l += 2)
            if (l[0] == id)
                AddScore(l[1]);
    store_score:
        gTcgAi->playAreaAIScore[gTcg->tempPlayAreaLocation] = gTcgAi->score;
    }
    gTcgAi->retreatScore = 0;
    return TcgAi_FindHighestBenchScore();
}

// the Active Pokémon retreats for the benched one `location` (an Energy attached first, if one more's
// all it needs and the AI may): the retreat cost's Energy cards picked. TRUE: it can't retreat.
static bool32 TryToRetreat(u32 location)
{
    u32 cost, i, c;
    u8 *de, status;
    const u8 *hl;
    if (gTcgAi->playEnergyCardForRetreat)
    {
        status = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS) & TCG_CNF_SLP_PRZ;
        if (status != TCG_ASLEEP && status != TCG_PARALYZED && !gTcg->alreadyPlayedEnergy)
        {
            u32 attached = TcgAi_CountNumberOfEnergyCardsAttached(TCG_PLAY_AREA_ARENA);
            gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
            cost = Tcg_GetPlayAreaCardRetreatCost();
            if (cost > attached && cost - attached == 1 && !TcgAi_CreateEnergyCardListFromHand())
            {
                gTcg->temp = gTcg->duelTempList[0];
                gTcg->tempPlayAreaLocationFfa1 = TCG_PLAY_AREA_ARENA;
                Tcg_AIMakeDecision(TCG_OPPACTION_PLAY_ENERGY);
            }
        }
    }
    // (Mysterious Fossil, Clefairy Doll: not here)
    gTcg->tempPlayAreaLocationFfa1 = location;
    status = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS);
    if ((status & TCG_CNF_SLP_PRZ) == TCG_ASLEEP || (status & TCG_CNF_SLP_PRZ) == TCG_PARALYZED)
        return TRUE;
    gTcg->temp = status;
    gTcg->tempRetreatCostCards[0] = 0xFF;
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    gTcgAi->tempCardRetreatCost = Tcg_GetPlayAreaCardRetreatCost();
    if (gTcgAi->tempCardRetreatCost == 0)
        goto retreat;
    Tcg_CreateArenaOrBenchEnergyCardList(TCG_PLAY_AREA_ARENA);
    Tcg_GetPlayAreaCardAttachedEnergies(TCG_PLAY_AREA_ARENA);
    if (gTcgAi->tempCardRetreatCost == gTcg->totalAttachedEnergies)
    {
        // all of them
        for (i = 0; i < ARRAY_COUNT(gTcg->tempRetreatCostCards); i++)
        {
            gTcg->tempRetreatCostCards[i] = gTcg->duelTempList[i];
            if (gTcg->duelTempList[i] == 0xFF)
                break;
        }
        goto retreat;
    }
    gTcgAi->tempCardId = TurnCardId(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
    gTcgAi->tempCardType = Tcg_Card(gTcgAi->tempCardId)->type | TCG_TYPE_ENERGY;
    c = gTcgAi->tempCardRetreatCost;
    de = gTcg->tempRetreatCostCards;
    // Double Colorless for 2 of it at a time
    for (hl = gTcg->duelTempList; c >= 2 && *hl != 0xFF;)
    {
        *de = *hl++;
        if (TurnCardId(*de) != TCG_CARD_DOUBLE_COLORLESS_ENERGY)
            continue;
        Tcg_RemoveCardFromDuelTempList(*de);
        hl--;
        de++;
        c -= 2;
        if (c == 0)
            goto end_retreat_list;
    }
    // then the ones no use to it, at random
    Tcg_ShuffleCards(gTcg->duelTempList, Tcg_CountCardsInDuelTempList());
    for (hl = gTcg->duelTempList; *hl != 0xFF;)
    {
        *de = *hl++;
        if (TcgAi_CheckIfEnergyIsUseful(*de))
            continue;
        Tcg_RemoveCardFromDuelTempList(*de);
        hl--;
        de++;
        if (--c == 0)
            goto end_retreat_list;
    }
    // then any
    for (hl = gTcg->duelTempList;;)
    {
        if (*hl == 0xFF)
            return TRUE;
        *de++ = *hl;
        if (TurnCardId(*hl++) == TCG_CARD_DOUBLE_COLORLESS_ENERGY && --c == 0)
            break;
        if (--c == 0)
            break;
    }
end_retreat_list:
    *de = 0xFF;
retreat:
    Tcg_AIMakeDecision(TCG_OPPACTION_ATTEMPT_RETREAT);
    return FALSE;
}

bool32 TcgAi_TryToRetreat(void)
{
    return TryToRetreat(gTcgAi->playAreaCardToSwitch);
}

// AIProcessRetreat (decks/general.asm): retreat, if the AI wants to and hasn't (with Switch, if it can)
void TcgAi_ProcessRetreat(void)
{
    u32 loc;
    if (gTcgAi->retreatedThisTurn)
        return;
    if (!TcgAi_DecideWhetherToRetreat())
        return;
    loc = TcgAi_DecideBenchPokemonToSwitchTo();
    if (loc == 0xFF)
        return;
    gTcgAi->playAreaCardToSwitch = loc;
    gTcgAi->retreatedThisTurn = TRUE;
    TcgAi_ProcessHandTrainerCards(TCG_AI_TRAINER_CARD_PHASE_09);
    if (gTcgAi->previousAIFlags & TCG_AI_FLAG_USED_SWITCH)
    {
        gTcgAi->previousAIFlags &= ~TCG_AI_FLAG_USED_SWITCH;
        TcgAi_HandleAIEnergyTrans(TCG_AI_ENERGY_TRANS_RETREAT);
        return;
    }
    TryToRetreat(gTcgAi->playAreaCardToSwitch);
}
