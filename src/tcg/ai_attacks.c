// Pokémon Canvas: the Trading Card Game AI's attacking, ported from the Game Boy Color game's
// engine/duel/ai/attacks.asm and special_attacks.asm (pret/poketcg) (TcgAi_ prefixed): each attack
// scored (from $50), the best used if it scores $50 or more.
#include "global.h"
#include "tcg.h"
#include "tcg_ai.h"

static inline u32 TurnCardId(u32 deckIndex)
{
    return Tcg_GetCardIdFromDeckIndex(deckIndex);
}

static void RetrievePlayAreaAIScoreFromBackup2(void)
{
    memcpy(gTcgAi->playAreaAIScore, gTcgAi->tempPlayAreaAIScore, TCG_MAX_PLAY_AREA_POKEMON);
    gTcgAi->score = gTcgAi->tempAIScore;
}

// ---- special_attacks.asm --------------------------------------------------------------------

static bool32 CheckIfAnyBasicPokemonInDeck(void)
{
    u32 e;
    for (e = 0; e < TCG_DECK_SIZE; e++)
    {
        if (*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + e) != TCG_CARD_LOCATION_DECK)
            continue;
        Tcg_LoadCard2(e);
        if (gTcg->loadedCard2->type < TCG_TYPE_ENERGY && gTcg->loadedCard2->stage == TCG_BASIC)
            return TRUE;
    }
    return FALSE;
}

// the score ($80: neutral) for the attacks with SPECIAL_AI_HANDLING. (Of the game's list, these are
// the ones in our decks: Oddish's Sprout, Exeggutor's Teleport, Kangaskhan's Fetch, Electrode's Energy
// Spike, Dragonair's Hyper Beam.)
u32 TcgAi_HandleSpecialAIAttacks(void)
{
    u32 id = TurnCardId(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcg->tempPlayAreaLocation)), n;
    switch (id)
    {
    case TCG_CARD_ODDISH:   // .CallForFamily
        if (TcgAi_LookForCardIDInLocation(TCG_CARD_LOCATION_DECK, id) == 0xFF)
            return 0;
        n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
        if (n >= TCG_MAX_PLAY_AREA_POKEMON - 1)
            return 0;
        return 0x80 + (TCG_MAX_PLAY_AREA_POKEMON - 1 - n);
    case TCG_CARD_EXEGGUTOR:    // .Teleport
        return TcgAi_DecideWhetherToRetreat() ? 0x8A : 0;
    case TCG_CARD_KANGASKHAN:   // .Fetch
        return *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_NOT_IN_DECK) >= 41 ? 0 : 0x80;
    case TCG_CARD_ELECTRODE_LV35:   // .EnergySpike
        if (TcgAi_LookForCardIDInLocation(TCG_CARD_LOCATION_DECK, TCG_CARD_LIGHTNING_ENERGY) == 0xFF)
            return 0;
        return TcgAi_ProcessButDontPlayEnergy_SkipEvolution() ? 0x83 : 0;
    case TCG_CARD_DRAGONAIR:    // .HyperBeam
        Tcg_SwapTurn();
        n = TcgAi_CountNumberOfEnergyCardsAttached(TCG_PLAY_AREA_ARENA);
        Tcg_SwapTurn();
        return n ? 0x83 : 0x80;
    }
    (void)CheckIfAnyBasicPokemonInDeck;     // (.FriendshipSong's: Jigglypuff Lv13 isn't in our decks)
    return 0;
}

// the first attack instead of the second, if it scored $50 or more and knocks out the defender too
// (unless the second heals, or weakens the defender's attacks)
static void CheckWhetherToSwitchToFirstAttack(void)
{
    if (gTcgAi->firstAttackAIScore < 0x50)
        goto keep_second;
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    TcgAi_EstimateDamage_VersusDefendingCard(0);
    if (*Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_HP) > (u8)gTcg->damage)
        goto keep_second;
    Tcg_CopyAttackDataAndDamage_FromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD), 1);
    if (Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG2_ADDRESS | TCG_HEAL_USER_F))
        goto keep_second;
    if (Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG2_ADDRESS | TCG_NULLIFY_OR_WEAKEN_ATTACK_F))
        goto keep_second;
    gTcg->selectedAttack = 0;
    return;
keep_second:
    gTcg->selectedAttack = 1;
}

// ---- attacks.asm ----------------------------------------------------------------------------

// how many of the turn holder's benched Pokémon `damage` would knock out, plus `start`; TRUE: that's
// as many as the other duelist's prizes (or more)
static bool32 CheckIfKOsBench(u32 start, u32 damage, u32 *count)
{
    u32 e, d = start;
    for (e = 1; e < TCG_MAX_PLAY_AREA_POKEMON && *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + e) != 0xFF; e++)
        if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + e) <= damage)
            d++;
    *count = (u8)d;
    Tcg_SwapTurn();
    e = Tcg_CountPrizes();
    Tcg_SwapTurn();
    return e <= (u8)d;
}

// the AI's score for the Active Pokémon's attack `attack` (wAIScore)
static void GetAIScoreOfAttack(u32 attack)
{
    u32 tempAI, counters;
    gTcg->selectedAttack = attack;
    gTcgAi->score = 0x50;
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    if (TcgAi_CheckIfSelectedAttackIsUnusable())
        goto unusable;
    gTcgAi->cannotDamage = 0;
    gTcg->tempTurnDuelistCardId = TurnCardId(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
    Tcg_SwapTurn();
    gTcg->tempNonTurnDuelistCardId = TurnCardId(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
    if (Tcg_HandleNoDamageOrEffectSubstatus())
    {
        Tcg_SwapTurn();
        gTcgAi->cannotDamage = 1;
        TcgAi_EstimateDamage_VersusDefendingCard(gTcg->selectedAttack);
        if (gTcg->loadedAttack.category == TCG_POKEMON_POWER)
            goto unusable;
        if (!(gTcg->loadedAttack.category & TCG_RESIDUAL)
         && !Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG1_ADDRESS | TCG_DAMAGE_TO_OPPONENT_BENCH_F))
            goto unusable;
    }
    else
    {
        Tcg_SwapTurn();
    }
    // .check_if_can_ko
    TcgAi_EstimateDamage_VersusDefendingCard(gTcg->selectedAttack);
    if (*Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_HP) <= (u8)gTcg->damage)
        TcgAi_Encourage(20);
    // .check_damage
    gTcgAi->attackIsNonDamaging = 0;
    gTcgAi->tempAI = gTcg->damage;
    if ((u8)gTcg->damage)
    {
        TcgAi_Encourage(TcgAi_ConvertHPToDamageCounters(gTcg->damage));
    }
    else
    {
        gTcgAi->attackIsNonDamaging = 1;
        TcgAi_Discourage(1);
        if (gTcg->aiMaxDamage)
        {
            TcgAi_Encourage(2);
            gTcgAi->attackIsNonDamaging = 0;
        }
        if (Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG1_ADDRESS | TCG_DAMAGE_TO_OPPONENT_BENCH_F))
            TcgAi_Encourage(2);
    }
    // .check_recoil
    if (Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG1_ADDRESS | TCG_LOW_RECOIL_F)
     || Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG1_ADDRESS | TCG_HIGH_RECOIL_F))
    {
        if (gTcg->loadedAttack.extra)
        {
            u16 recoil;
            gTcg->damage = gTcg->loadedAttack.extra;
            recoil = Tcg_ApplyDamageModifiers_DamageToSelf();
            TcgAi_Discourage(TcgAi_ConvertHPToDamageCounters(recoil));
            if (!Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG1_ADDRESS | TCG_HIGH_RECOIL_F))
            {
                if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP) > (u8)recoil)
                    goto check_defending_can_ko;
                TcgAi_Discourage(10);   // (it'd knock itself out)
            }
            // .high_recoil
            {
                u32 id, benchDamage, playerKOs, ownKOs;
                if (*Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) < 2)
                    goto dismiss;
                if (gTcgAi->deckId == TCG_DECKID_ZAPPING_SELFDESTRUCT)
                {
                    u32 maxHp, damage;
                    if (*Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_NOT_IN_DECK) < 31)
                    {
                        damage = Tcg_GetCardDamageAndMaxHP(TCG_PLAY_AREA_ARENA, &maxHp);
                        if ((u8)(damage << 1) >= maxHp)
                        {
                            benchDamage = TurnCardId(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD)) == TCG_CARD_MAGNEMITE_LV13 ? 10 : 20;
                            if (CheckIfKOsBench(1, benchDamage, &ownKOs))
                                goto dismiss;
                            TcgAi_Encourage(20);
                            return;
                        }
                    }
                }
                // (Rock Crusher's, Boom Boom Selfdestruct's and Power Generator's: not our decks)
                // .high_recoil_generic_checks
                id = TurnCardId(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
                if (id == TCG_CARD_CHANSEY)
                    benchDamage = 0;
                else if (id == TCG_CARD_MAGNEMITE_LV13)
                    benchDamage = 10;
                else
                    benchDamage = 20;
                Tcg_SwapTurn();
                if (CheckIfKOsBench(0, benchDamage, &playerKOs))
                {
                    Tcg_SwapTurn();
                    TcgAi_Encourage(20);    // (it wins the duel)
                    return;
                }
                Tcg_SwapTurn();
                if (CheckIfKOsBench(1, benchDamage, &ownKOs))
                    goto dismiss;
                if (ownKOs)
                    TcgAi_Discourage(ownKOs - 1);
                TcgAi_Encourage(playerKOs);
            }
        }
    }
check_defending_can_ko:
    attack = gTcg->selectedAttack;
    if (TcgAi_CheckIfDefendingPokemonCanKnockOut(NULL))
    {
        gTcg->selectedAttack = attack;
        TcgAi_Encourage(5);
        if (gTcgAi->attackIsNonDamaging)
            TcgAi_Discourage(5);
    }
    gTcg->selectedAttack = attack;
    // .check_discard
    Tcg_CopyAttackDataAndDamage_FromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD), gTcg->selectedAttack);
    if (Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG2_ADDRESS | TCG_DISCARD_ENERGY_F))
    {
        TcgAi_Discourage(1);
        TcgAi_Discourage(gTcg->loadedAttack.extra);
    }
    if (Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG2_ADDRESS | TCG_ENCOURAGE_THIS_ATTACK_F))
        TcgAi_Encourage(gTcg->loadedAttack.extra);
    if (Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG2_ADDRESS | TCG_NULLIFY_OR_WEAKEN_ATTACK_F))
        TcgAi_Encourage(1);
    if (Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG1_ADDRESS | TCG_DRAW_CARD_F))
        TcgAi_Encourage(1);
    if (Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG2_ADDRESS | TCG_HEAL_USER_F))
    {
        u32 heal = gTcg->loadedAttack.extra, damage;
        if (heal != TCG_HEALING_EQUALS_10_HP)
        {
            u32 b = TcgAi_ConvertHPToDamageCounters(gTcgAi->tempAI);
            if (heal != TCG_HEALING_EQUALS_DAMAGE_DEALT)
                b = (b >> 1) + (b & 1);
            heal = TcgAi_ConvertHPToDamageCounters(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP));
            if (heal >= b)
                heal = b;
        }
        damage = TcgAi_ConvertHPToDamageCounters(Tcg_GetCardDamageAndMaxHP(TCG_PLAY_AREA_ARENA, NULL));
        TcgAi_Encourage(damage < heal ? damage : heal);
    }
    // .check_status_effect (not against Snorlax: Thick Skinned)
    Tcg_SwapTurn();
    counters = TurnCardId(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
    Tcg_SwapTurn();
    if (counters != TCG_CARD_SNORLAX)
    {
        tempAI = gTcgAi->tempAI = *Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_STATUS);
        if (Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG1_ADDRESS | TCG_INFLICT_POISON_F))
        {
            if (!(tempAI & TCG_DOUBLE_POISONED))
                TcgAi_Encourage(2);
            else if ((tempAI & 0x40) && Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG2_ADDRESS | TCG_ENCOURAGE_THIS_ATTACK_F))
                TcgAi_Discourage(2);
        }
        if (Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG1_ADDRESS | TCG_INFLICT_SLEEP_F) && (tempAI & TCG_CNF_SLP_PRZ) != TCG_ASLEEP)
            TcgAi_Encourage(1);
        if (Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG1_ADDRESS | TCG_INFLICT_PARALYSIS_F))
        {
            if ((tempAI & TCG_CNF_SLP_PRZ) == TCG_ASLEEP)
                TcgAi_Discourage(1);
            else
                TcgAi_Encourage(1);
        }
        if (Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG1_ADDRESS | TCG_INFLICT_CONFUSION_F))
        {
            if ((tempAI & TCG_CNF_SLP_PRZ) == TCG_ASLEEP)
                TcgAi_Discourage(1);
            else if ((tempAI & TCG_CNF_SLP_PRZ) != TCG_CONFUSED)
                TcgAi_Encourage(1);
        }
        if ((*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS) & TCG_CNF_SLP_PRZ) == TCG_CONFUSED)
            TcgAi_Discourage(1);
    }
    // .handle_special_atks
    if (Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG3_ADDRESS | TCG_SPECIAL_AI_HANDLING_F))
    {
        u32 special = TcgAi_HandleSpecialAIAttacks();
        if (special >= 0x80)
            TcgAi_Encourage(special - 0x80);
        else
            TcgAi_Discourage(0x80 - special);
    }
    return;
dismiss:
unusable:
    gTcgAi->score = 0;
}

// the attacks scored, and the best used (or just chosen: wAIExecuteProcessedAttack). TRUE: an attack
// was used (or chosen).
bool32 TcgAi_ProcessAttacks(void)
{
    if (gTcgAi->previousAIFlags & TCG_AI_FLAG_USED_PLUSPOWER)
    {
        gTcg->selectedAttack = gTcgAi->plusPowerAttack;
    }
    else
    {
        u32 best, score;
        // (the game's Mewtwo-mill check: never set here)
        GetAIScoreOfAttack(0);
        gTcgAi->firstAttackAIScore = gTcgAi->score;
        GetAIScoreOfAttack(1);
        best = 1;
        score = gTcgAi->score;
        if (score < gTcgAi->firstAttackAIScore)
        {
            best = 0;
            score = gTcgAi->firstAttackAIScore;
        }
        if (score < 0x50)
        {
            if (gTcgAi->executeProcessedAttack)
            {
                RetrievePlayAreaAIScoreFromBackup2();
                return FALSE;
            }
            gTcgAi->retreatScore++;
            return FALSE;
        }
        gTcg->selectedAttack = best;
        if (best)
            CheckWhetherToSwitchToFirstAttack();
    }
    // .attack_chosen
    if (gTcgAi->executeProcessedAttack)
    {
        RetrievePlayAreaAIScoreFromBackup2();
        return TRUE;
    }
    TcgAi_ProcessHandTrainerCards(TCG_AI_TRAINER_CARD_PHASE_14);
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    TcgAi_EstimateDamage_VersusDefendingCard(gTcg->selectedAttack);
    if ((u8)gTcg->damage || Tcg_CheckLoadedAttackFlag(TCG_ATTACK_FLAG1_ADDRESS | TCG_DAMAGE_TO_OPPONENT_BENCH_F))
        gTcgAi->retreatScore = 0;
    else
        gTcgAi->retreatScore++;
    gTcgAi->triedAttack = TRUE;
    TcgAi_TryUseAttack();
    return TRUE;
}

// the best attack chosen, not used (the play area's AI scores kept): wSelectedAttack. TRUE: there's one.
bool32 TcgAi_ProcessButDontUseAttack(void)
{
    gTcgAi->executeProcessedAttack = 1;
    memcpy(gTcgAi->tempPlayAreaAIScore, gTcgAi->playAreaAIScore, TCG_MAX_PLAY_AREA_POKEMON);
    gTcgAi->tempAIScore = gTcgAi->score;
    return TcgAi_ProcessAttacks();
}

bool32 TcgAi_ProcessAndTryToUseAttack(void)
{
    gTcgAi->executeProcessedAttack = 0;
    return TcgAi_ProcessAttacks();
}
