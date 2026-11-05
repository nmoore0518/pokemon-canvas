// Pokémon Canvas: the Trading Card Game duel's attacks, damage and knock-outs, ported from the Game
// Boy Color game (pret/poketcg) function by function, under the same names (Tcg_ prefixed):
//   home/duel.asm (GetPlayAreaCardAttachedEnergies .. CheckLoadedAttackFlag), home/substatus.asm,
//   home/card_color.asm, home/effect_commands.asm, home/coin_toss.asm, and from engine/duel/core.asm
//   ApplyStatusConditionQueue, HandleDestinyBondAndBetweenTurnKnockOuts (and what it calls),
//   ClearNonTurnTemporaryDuelvars, UpdateArenaCardLastTurnDamage, _TossCoin's logic,
//   PrintThereWasNoEffectFromStatusText; and the AI's AIPickPrizeCards.
// Damage works in 16 bits as the game's de does (bit 15 set: below 0). Where the game returns carry,
// these return TRUE; where it returns a text id in hl for its caller to print, gTcg->hlText.
// The game's checks for cards that aren't in any of our decks (Muk's Toxic Gas, Kabuto Armor,
// Mew's Neutralizing Shield, Haunter's Transparency, Omanyte, Dodrio, Aerodactyl, Venomoth's Shift)
// can never be true here: they're left out, and noted where they were.
#include "global.h"
#include "tcg.h"
#include "tcg_ui.h"
#include "constants/tcg_text.h"

#define TCG_PLAY_AREA(loc)  (TCG_CARD_LOCATION_PLAY_AREA | (loc))

static inline u32 GetCardType(u32 cardId)
{
    return Tcg_Card(cardId)->type;
}

void Tcg_LoadTxRam2(const u8 *text)
{
    gTcg->txRam2[0] = text;
}

void Tcg_LoadTxRam3(u16 n)
{
    gTcg->txRam3[0] = n;
}

// _CopyCardNameAndLevel (engine/copy_card_name.asm): wLoadedCard1's name and level into
// wDefaultText, padded with spaces to `length` (half-width text: half tiles). Returns the game's hl:
// where the padding starts (half-width), or the level's end (full-width)
u8 *Tcg_CopyCardNameAndLevelLength(u32 length)
{
    const struct TcgCard *card = gTcg->loadedCard1;
    u8 *hl = gTcg->defaultText;
    const u8 *s = card->name;
    bool32 level = card->type < TCG_TYPE_ENERGY && card->level;
    s32 b;
    while ((*hl = *s++) != TCG_TX_END)
        hl++;
    if (gTcg->defaultText[0] == TCG_TX_HALFWIDTH)
    {
        b = (u8)((length + 1) * 2) - (hl - gTcg->defaultText) - 1;
        if (level)
        {
            u32 lv = card->level;
            *hl++ = ' ';
            *hl++ = 'L';
            *hl++ = 'v';
            b -= 3;
            if (lv >= 10)
            {
                *hl++ = '0' + lv / 10;
                b--;
            }
            *hl++ = '0' + lv % 10;
            b--;
        }
        {
            // (the game's loop: at least one space, and its count wraps if the name's longer)
            u8 *pad = hl;
            do
                *hl++ = ' ';
            while (--b > 0 && hl < gTcg->defaultText + sizeof(gTcg->defaultText) - 1);
            *hl = TCG_TX_END;
            return pad;
        }
    }
    // (full-width names: none of ours)
    if (level)
    {
        *hl++ = TCG_TX_SYMBOL;
        *hl++ = TCG_SYM_Lv;
        if (card->level >= 10)
        {
            *hl++ = TCG_TX_SYMBOL;
            *hl++ = TCG_SYM_0 + card->level / 10;
        }
        *hl++ = TCG_TX_SYMBOL;
        *hl++ = TCG_SYM_0 + card->level % 10;
    }
    *hl = TCG_TX_END;
    return hl;
}

// (CopyCardNameAndLevel, then "ld [hl], TX_END", as every caller in the duel's rules does: the name
// and level without the padding)
void Tcg_CopyCardNameAndLevel(void)
{
    *Tcg_CopyCardNameAndLevelLength(18) = TCG_TX_END;
}

// ---- home/duel.asm --------------------------------------------------------------------------

// the energy attached to the turn holder's Pokémon at `location` (PLAY_AREA_*): wAttachedEnergies
// (each type's count: Double Colorless counts 2) and wTotalAttachedEnergies
void Tcg_GetPlayAreaCardAttachedEnergies(u32 location)
{
    u32 i, total = 0, where = TCG_PLAY_AREA(location);
    for (i = 0; i < TCG_NUM_TYPES; i++)
        gTcg->attachedEnergies[i] = 0;
    for (i = 0; i < TCG_DECK_SIZE; i++)
    {
        u32 type;
        if (*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + i) != where)
            continue;
        Tcg_LoadCard2(i);
        type = gTcg->loadedCard2->type;
        if (!(type & (1 << TCG_TYPE_ENERGY_F)))
            continue;
        type &= TCG_TYPE_PKMN;
        gTcg->attachedEnergies[type]++;
        if (type == TCG_COLORLESS)
            gTcg->attachedEnergies[type]++;
    }
    for (i = 0; i < TCG_NUM_TYPES; i++)
        total += gTcg->attachedEnergies[i];
    gTcg->totalAttachedEnergies = total;
}

// how many of card `cardId` the turn holder has at `location` (CARD_LOCATION_*)
u32 Tcg_CountCardIDInLocation(u32 location, u32 cardId)
{
    u32 i, n = 0;
    for (i = 0; i < TCG_DECK_SIZE; i++)
        if (*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + i) == location && Tcg_GetCardIdFromDeckIndex(i) == cardId)
            n++;
    return n;
}

// Checks if the command type is one of the loaded attack's (or Trainer card's) effect commands, and
// if so runs its function (TRUE: the function's carry). CheckMatchingCommand: the function, or NULL.
bool32 (*Tcg_CheckMatchingCommand(const struct TcgEffectCommand *commands, u32 type))(void)
{
    if (commands == NULL)
        return NULL;
    for (; commands->type; commands++)
        if (commands->type == type)
            return commands->func;
    return NULL;
}

bool32 Tcg_TryExecuteEffectCommandFunction(u32 type)
{
    bool32 (*func)(void) = Tcg_CheckMatchingCommand(gTcg->loadedAttackEffectCommands, type);
    return func ? func() : FALSE;
}

// a played Pokémon card (hTempCardIndex_ff98): if it has a Pokémon Power that triggers when it's
// played, it's used
void Tcg_ProcessPlayedPokemonCard(void)
{
    u32 card = gTcg->tempCardIndex;
    // (ClearChangedTypesIfMuk: no Muk)
    Tcg_CopyAttackDataAndDamage_FromDeckIndex(card, 0);
    Tcg_UpdateArenaCardIDsAndClearTwoTurnDuelVars();
    gTcg->tempCardIndexFf9f = card;
    gTcg->tempTurnDuelistCardId = Tcg_GetCardIdFromDeckIndex(card);
    if (gTcg->loadedAttack.category != TCG_POKEMON_POWER)
        return;
    Tcg_DisplayUsePokemonPowerScreen();
    Tcg_LoadCard1(card);
    Tcg_LoadTxRam2(gTcg->loadedCard1->name);
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_HavePokemonPower);
    // (Muk's Toxic Gas: CheckIsIncapableOfUsingPkmnPower(PLAY_AREA_BENCH_1) checks only for it)
    if (Tcg_CheckMatchingCommand(gTcg->loadedAttackEffectCommands, TCG_EFFECTCMDTYPE_PKMN_POWER_TRIGGER) == NULL)
        return;
    Tcg_DrawDuelMainScene();
    Tcg_LoadCard1(gTcg->tempCardIndexFf9f);
    gTcg->txRam2[0] = gTcg->loadedCard1->name;
    gTcg->txRam2[1] = gTcg->loadedAttack.name;
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_WillUseThePokemonPower);
    Tcg_ResetAttackAnimationIsPlaying();
    Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_PKMN_POWER_TRIGGER);
}

// wLoadedCard1's attack `attack` (0, 1) into wLoadedAttack, its damage into wDamage
static void CopyAttackDataAndDamage(u32 attack)
{
    gTcg->tempCardId = gTcg->loadedCard1Id;
    gTcg->loadedAttack = gTcg->loadedCard1->attacks[attack ? 1 : 0];
    gTcg->loadedAttackEffectCommands = gTcgEffectCommands[gTcg->loadedAttack.effects];
    gTcg->damage = gTcg->loadedAttack.damage;
    gTcg->noDamageOrEffect = 0;
    gTcg->dealtDamage = 0;
}

void Tcg_CopyAttackDataAndDamage_FromCardID(u32 cardId, u32 deckIndex, u32 attack)
{
    gTcg->selectedAttack = attack;
    gTcg->tempCardIndexFf9f = deckIndex;
    gTcg->loadedCard1Id = cardId;
    gTcg->loadedCard1 = Tcg_Card(cardId);
    CopyAttackDataAndDamage(attack);
}

void Tcg_CopyAttackDataAndDamage_FromDeckIndex(u32 deckIndex, u32 attack)
{
    gTcg->selectedAttack = attack;
    gTcg->tempCardIndexFf9f = deckIndex;
    Tcg_LoadCard1(deckIndex);
    CopyAttackDataAndDamage(attack);
}

// the arena cards' ids, and the variables that last a two-player turn cleared (when a Pokémon card is
// played or an attack is used)
void Tcg_UpdateArenaCardIDsAndClearTwoTurnDuelVars(void)
{
    gTcg->tempCardIndexFf9f = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD);
    gTcg->tempTurnDuelistCardId = Tcg_GetCardIdFromDeckIndex(gTcg->tempCardIndexFf9f);
    Tcg_SwapTurn();
    gTcg->tempNonTurnDuelistCardId = Tcg_GetCardIdFromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
    Tcg_SwapTurn();
    gTcg->statusConditionQueueIndex = 0;
    gTcg->effectFailed = 0;
    gTcg->isDamageToSelf = 0;
    gTcg->defendingWasForcedToSwitch = 0;
    gTcg->metronomeEnergyCost = 0;
    gTcg->noEffectFromWhichStatus = 0;
    Tcg_ClearNonTurnTemporaryDuelvars();
}

static bool32 ReturnCarry_PrintHl(void)
{
    Tcg_DrawWideTextBox_WaitForInput(gTcg->hlText);
    return TRUE;
}

static bool32 UsePokemonPower(void);
static void Func_1bb4(void);

// Use an attack (from the attack menu) or a Pokémon Power. TRUE: it wasn't used (the game's carry).
bool32 Tcg_UseAttackOrPokemonPower(void)
{
    gTcg->playerAttackingAttackIndex = gTcg->selectedAttack;
    gTcg->playerAttackingCardIndex = gTcg->tempCardIndexFf9f;
    gTcg->playerAttackingCardId = gTcg->tempCardId;
    if (gTcg->loadedAttack.category == TCG_POKEMON_POWER)
        return UsePokemonPower();
    Tcg_UpdateArenaCardIDsAndClearTwoTurnDuelVars();
    if (Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_INITIAL_EFFECT_1))
        return ReturnCarry_PrintHl();
    if (Tcg_CheckSandAttackOrSmokescreenSubstatus())
    {
        if (Tcg_HandleSandAttackOrSmokescreenSubstatus())
        {
            Tcg_ClearNonTurnTemporaryDuelvars();
            return FALSE;
        }
    }
    if (Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_INITIAL_EFFECT_2))
        return TRUE;
    Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_DISCARD_ENERGY);
    if (Tcg_CheckSelfConfusionDamage())
        return Tcg_HandleConfusionDamageToSelf();
    Tcg_DrawDuelMainScene_PrintPokemonsAttackText();
    Tcg_WaitForWideTextBoxInput();
    Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_REQUIRE_SELECTION);
    return Tcg_PlayAttackAnimation_DealAttackDamage();
}

bool32 Tcg_PlayAttackAnimation_DealAttackDamage(void)
{
    u16 damage;
    u8 *hp;
    Tcg_ResetAttackAnimationIsPlaying();
    if (!(gTcg->loadedAttack.category & TCG_RESIDUAL))
    {
        Tcg_SwapTurn();
        Tcg_HandleNoDamageOrEffectSubstatus();
        Tcg_SwapTurn();
    }
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_BEFORE_DAMAGE);
    damage = Tcg_ApplyDamageModifiers_DamageToTarget();
    damage = Tcg_ApplyTransparencyIfApplicable(damage);
    gTcg->dealtDamage = damage;
    hp = Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_HP);
    Tcg_PlayAttackAnimation(gTcg->whoseTurn ^ 1, TCG_PLAY_AREA_ARENA, gTcg->damageEffectiveness, damage);
    Tcg_PlayStatusConditionQueueAnimations();
    Tcg_WaitAttackAnimation();
    Tcg_SubtractHP(hp, damage);
    Tcg_DrawDuelHUDsIfMainScene();
    Tcg_PrintKnockedOutIfHLZero(hp);
    return Tcg_HandleAfterDamageEffects();
}

bool32 Tcg_HandleAfterDamageEffects(void)
{
    u8 defending = gTcg->tempNonTurnDuelistCardId;
    Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_AFTER_DAMAGE);
    gTcg->tempNonTurnDuelistCardId = defending;
    Tcg_HandleStrikesBack_AgainstResidualAttack();
    Tcg_ApplyStatusConditionQueue();
    Func_1bb4();
    Tcg_UpdateArenaCardLastTurnDamage();
    Tcg_HandleDestinyBondAndBetweenTurnKnockOuts();
    return FALSE;
}

// the attacker hits itself in its confusion: 20 damage
bool32 Tcg_HandleConfusionDamageToSelf(void)
{
    Tcg_DrawDuelMainScene();
    gTcg->isDamageToSelf = TRUE;
    Tcg_DrawWideTextBox_PrintText(TCG_TEXT_DamageToSelfDueToConfusion);
    gTcg->loadedAttack.animation = TCG_ATK_ANIM_CONFUSION_HIT;
    Tcg_DealConfusionDamageToSelf(20);
    Func_1bb4();
    Tcg_HandleDestinyBondAndBetweenTurnKnockOuts();
    Tcg_ClearNonTurnTemporaryDuelvars();
    return FALSE;
}

static bool32 UsePokemonPower(void)
{
    Tcg_ResetAttackAnimationIsPlaying();
    if (Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_INITIAL_EFFECT_2))
    {
        Tcg_DisplayUsePokemonPowerScreen();
        return ReturnCarry_PrintHl();
    }
    if (Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_REQUIRE_SELECTION))
        return TRUE;
    Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_BEFORE_DAMAGE);
    return FALSE;
}

// (Haunter's Transparency: no Haunter Lv17 here, so the game's HandleTransparency never stops the
// damage, and this keeps it as it is)
u16 Tcg_ApplyTransparencyIfApplicable(u16 damage)
{
    return damage;
}

// TRUE: the confused attacker got tails and hits itself
bool32 Tcg_CheckSelfConfusionDamage(void)
{
    gTcg->confusionAttackCheckWasUnsuccessful = FALSE;
    if ((*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS) & TCG_CNF_SLP_PRZ) != TCG_CONFUSED)
        return FALSE;
    if (Tcg_TossCoin(TCG_TEXT_ConfusionCheckDamage))
        return FALSE;
    gTcg->confusionAttackCheckWasUnsuccessful = TRUE;
    return TRUE;
}

// play the Trainer card at hTempCardIndex_ff98 (its effect commands work like an attack's).
// TRUE: it wasn't played.
bool32 Tcg_PlayTrainerCard(void)
{
    if (!Tcg_CheckCantUseTrainerDueToEffect())
    {
        gTcg->tempCardIndexFf9f = gTcg->tempCardIndex;
        Tcg_LoadNonPokemonCardEffectCommands();
        if (!Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_INITIAL_EFFECT_1))
        {
            if (Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_INITIAL_EFFECT_2))
                return FALSE;
            Tcg_DisplayUsedTrainerCardDetailScreen();
            Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_DISCARD_ENERGY);
            Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_REQUIRE_SELECTION);
            Tcg_TryExecuteEffectCommandFunction(TCG_EFFECTCMDTYPE_BEFORE_DAMAGE);
            Tcg_MoveHandCardToDiscardPile(gTcg->tempCardIndexFf9f);
            return FALSE;
        }
    }
    Tcg_DrawWideTextBox_WaitForInput(gTcg->hlText);
    return TRUE;
}

void Tcg_LoadNonPokemonCardEffectCommands(void)
{
    Tcg_LoadCard1(gTcg->tempCardIndexFf9f);
    gTcg->loadedAttackEffectCommands = gTcgEffectCommands[gTcg->loadedCard1->effects];
}

// the turn holder's arena Pokémon hurts itself: recoil (Thrash, Selfdestruct...) or confusion
void Tcg_DealRecoilDamageToSelf(u32 damage)
{
    gTcg->loadedAttack.animation = TCG_ATK_ANIM_RECOIL_HIT;
    Tcg_DealConfusionDamageToSelf(damage);
}

void Tcg_DealConfusionDamageToSelf(u32 damage)
{
    u8 noDamageOrEffect = gTcg->noDamageOrEffect, defending = gTcg->tempNonTurnDuelistCardId;
    u8 *hp;
    u16 de;
    gTcg->damage = damage;
    gTcg->noDamageOrEffect = 0;
    Tcg_ResetAttackAnimationIsPlaying();
    gTcg->tempNonTurnDuelistCardId = gTcg->tempTurnDuelistCardId;
    de = Tcg_ApplyDamageModifiers_DamageToSelf();
    hp = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP);
    Tcg_PlayAttackAnimation_DealAttackDamageSimple(gTcg->whoseTurn, TCG_PLAY_AREA_ARENA, gTcg->damageEffectiveness, hp, de);
    Tcg_PrintKnockedOutIfHLZero(hp);
    gTcg->tempNonTurnDuelistCardId = defending;
    gTcg->noDamageOrEffect = noDamageOrEffect;
}

// (engine/duel/core.asm)
void Tcg_PlayAttackAnimation_DealAttackDamageSimple(u32 duelist, u32 location, u32 effectiveness, u8 *hp, u16 damage)
{
    Tcg_PlayAttackAnimation(duelist, location, effectiveness, damage);
    Tcg_WaitAttackAnimation();
    Tcg_SubtractHP(hp, damage);
    Tcg_DrawDuelHUDsIfMainScene();
}

// wDamage, with the weakness (x2) and resistance (-30) of the defending (non-turn holder's arena)
// Pokémon to the attacker's color, PlusPower (+10 each), Defender (-20 each), and the substatuses
// and Pokémon Powers that change it. Sets wDamageEffectiveness.
u16 Tcg_ApplyDamageModifiers_DamageToTarget(void)
{
    u16 de;
    u32 wr;
    gTcg->damageEffectiveness = 0;
    if (gTcg->damage == 0)
        return 0;
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    de = gTcg->damage;
    if (de & (1 << (8 + TCG_UNAFFECTED_BY_WEAKNESS_RESISTANCE_F)))
    {
        de &= ~(1 << (8 + TCG_UNAFFECTED_BY_WEAKNESS_RESISTANCE_F));
        gTcg->damageEffectiveness = 0;
        de = Tcg_HandleDoubleDamageSubstatus(de);
    }
    else
    {
        de = Tcg_HandleDoubleDamageSubstatus(de);
        if (de == 0)
            return 0;
        wr = Tcg_TranslateColorToWR(Tcg_GetPlayAreaCardColor(gTcg->tempPlayAreaLocation));
        Tcg_SwapTurn();
        if (Tcg_GetArenaCardWeakness() & wr)
        {
            de <<= 1;
            gTcg->damageEffectiveness |= 1 << TCG_WEAKNESS;
        }
        if (Tcg_GetArenaCardResistance() & wr)
        {
            de -= 30;
            gTcg->damageEffectiveness |= 1 << TCG_RESISTANCE;
        }
        Tcg_SwapTurn();
    }
    de = Tcg_ApplyAttachedPlusPower(TCG_CARD_LOCATION_ARENA, de);
    Tcg_SwapTurn();
    de = Tcg_ApplyAttachedDefender(TCG_CARD_LOCATION_ARENA, de);
    de = Tcg_HandleDamageReduction(de);
    if (de & 0x8000)
        de = 0;
    Tcg_SwapTurn();
    return de;
}

// convert a color to its WR_* (weakness/resistance) bit: InvertedPowersOf2
u32 Tcg_TranslateColorToWR(u32 color)
{
    return (0x80 >> color) & 0xFF;
}

// wDamage, with the turn holder's arena Pokémon's weakness and resistance to its own color
u16 Tcg_ApplyDamageModifiers_DamageToSelf(void)
{
    u16 de;
    u32 wr;
    gTcg->damageEffectiveness = 0;
    if (gTcg->damage == 0)
        return 0;
    de = gTcg->damage;
    wr = Tcg_TranslateColorToWR(Tcg_GetArenaCardColor());
    if (Tcg_GetArenaCardWeakness() & wr)
    {
        de <<= 1;
        gTcg->damageEffectiveness |= 1 << TCG_WEAKNESS;
    }
    if (Tcg_GetArenaCardResistance() & wr)
    {
        de -= 30;
        gTcg->damageEffectiveness |= 1 << TCG_RESISTANCE;
    }
    de = Tcg_ApplyAttachedPlusPower(TCG_CARD_LOCATION_ARENA, de);
    de = Tcg_ApplyAttachedDefender(TCG_CARD_LOCATION_ARENA, de);
    if (de & 0x8000)
        return 0;
    return de;
}

// +10 for each PlusPower the turn holder has at `location`
u16 Tcg_ApplyAttachedPlusPower(u32 location, u16 damage)
{
    return damage + 10 * Tcg_CountCardIDInLocation(location, TCG_CARD_PLUSPOWER);
}

// -20 for each Defender the turn holder has at `location`
u16 Tcg_ApplyAttachedDefender(u32 location, u16 damage)
{
    return damage - 20 * Tcg_CountCardIDInLocation(location, TCG_CARD_DEFENDER);
}

// HP at `hp` minus `damage` (not below 0). TRUE: the HP isn't 0.
bool32 Tcg_SubtractHP(u8 *hp, u16 damage)
{
    u32 result = (u32)*hp - damage;
    *hp = (result & 0x8000) ? 0 : result;   // (the game: the 16-bit result's sign)
    return *hp != 0;
}

// if the turn holder's Pokémon at `location` has no HP left, it's knocked out (the text). TRUE: it was.
bool32 Tcg_PrintPlayAreaCardKnockedOutIfNoHP(u32 location)
{
    u8 defending;
    if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + location))
        return FALSE;
    defending = gTcg->tempNonTurnDuelistCardId;
    Tcg_LoadCard1(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + location));
    gTcg->tempNonTurnDuelistCardId = gTcg->loadedCard1Id;
    Tcg_PrintKnockedOut();
    gTcg->tempNonTurnDuelistCardId = defending;
    return TRUE;
}

void Tcg_PrintKnockedOutIfHLZero(const u8 *hp)
{
    if (*hp == 0)
        Tcg_PrintKnockedOut();
}

// "<wTempNonTurnDuelistCardID> was Knocked Out!" (40 frames)
void Tcg_PrintKnockedOut(void)
{
    Tcg_LoadTxRam2(Tcg_Card(gTcg->tempNonTurnDuelistCardId)->name);
    Tcg_DrawWideTextBox_PrintText(TCG_TEXT_WasKnockedOut);
    Tcg_DoFrames(40);
}

// damage to the turn holder's Pokémon at `location` (PLAY_AREA_*), from an effect: shown on the
// defending player's play area screen with the bench-hit animation, or wLoadedAttackAnimation's
void Tcg_DealDamageToPlayAreaPokemon_RegularAnim(u32 location, u16 damage)
{
    gTcg->loadedAttack.animation = TCG_ATK_ANIM_BENCH_HIT;
    Tcg_DealDamageToPlayAreaPokemon(location, damage);
}

void Tcg_DealDamageToPlayAreaPokemon(u32 location, u16 de)
{
    u8 *hp, oldHp;
    gTcg->tempPlayAreaLocationCceb = location;
    if (location == TCG_PLAY_AREA_ARENA && gTcg->noDamageOrEffect)
        return;
    gTcg->noDamageOrEffect = 0;
    gTcg->tempNonTurnDuelistCardId = Tcg_GetCardIdFromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + location));
    if (location == TCG_PLAY_AREA_ARENA)
    {
        if (gTcg->isDamageToSelf)
        {
            de = Tcg_ApplyAttachedPlusPower(TCG_CARD_LOCATION_ARENA, de);
        }
        else
        {
            Tcg_SwapTurn();
            de = Tcg_ApplyAttachedPlusPower(TCG_CARD_LOCATION_ARENA, de);
            Tcg_SwapTurn();
        }
    }
    if (gTcg->loadedAttack.category != TCG_POKEMON_POWER)
        de = Tcg_ApplyAttachedDefender(TCG_PLAY_AREA(location), de);
    if (location == TCG_PLAY_AREA_ARENA)
    {
        Tcg_HandleNoDamageOrEffectSubstatus();
        de = Tcg_HandleDamageReduction(de);
    }
    if (de & 0x8000)
        de = 0;
    de = Tcg_HandleDamageReductionOrNoDamageFromPkmnPowerEffects(de);
    if (location == TCG_PLAY_AREA_ARENA)
        gTcg->dealtDamage += de;
    hp = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + location);
    oldHp = *hp;
    Tcg_PlayAttackAnimation_DealAttackDamageSimple(gTcg->whoseTurn, location, 0, hp, de);
    if (oldHp)
        Tcg_PrintKnockedOutIfHLZero(hp);
    Tcg_HandleStrikesBack_AgainstDamagingAttack(de);
}

// "<Pokémon Lvxx>'s <attack>!" (the turn holder's arena Pokémon, wLoadedAttack)
void Tcg_DrawDuelMainScene_PrintPokemonsAttackText(void)
{
    Tcg_DrawDuelMainScene();
    Tcg_PrintPokemonsAttackText();
}

void Tcg_PrintPokemonsAttackText(void)
{
    Tcg_LoadCard1(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
    Tcg_CopyCardNameAndLevel();
    gTcg->txRam2[0] = NULL;     // (wDefaultText: the name and level)
    gTcg->txRam2[1] = gTcg->loadedAttack.name;
    Tcg_DrawWideTextBox_PrintText(TCG_TEXT_PokemonsAttack);
}

static void Func_1bb4(void)
{
    Tcg_FinishQueuedAnimations();
    Tcg_DrawDuelMainScene();
    Tcg_DrawDuelHUDs();
    gTcg->tempPlayAreaLocation = TCG_PLAY_AREA_ARENA;
    Tcg_PrintFailedEffectText();
    Tcg_WaitForWideTextBoxInput();
}

// "There was no effect from ..." (wEffectFailed: EFFECT_FAILED_NO_EFFECT) or "... was unsuccessful"
// (EFFECT_FAILED_UNSUCCESSFUL). TRUE: printed.
bool32 Tcg_PrintFailedEffectText(void)
{
    if (gTcg->effectFailed == 0)
        return FALSE;
    if (gTcg->effectFailed == TCG_EFFECT_FAILED_NO_EFFECT)
    {
        Tcg_DrawWideTextBox_PrintText(Tcg_PrintThereWasNoEffectFromStatusText());
        return TRUE;
    }
    Tcg_LoadCard1(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcg->tempPlayAreaLocation));
    Tcg_CopyCardNameAndLevel();
    gTcg->txRam2[0] = NULL;
    gTcg->txRam2[1] = gTcg->loadedAttack.name;
    Tcg_DrawWideTextBox_PrintText(TCG_TEXT_WasUnsuccessful);
    return TRUE;
}

// the retreat cost of the turn holder's Pokémon at hTempPlayAreaLocation_ff9d
u32 Tcg_GetPlayAreaCardRetreatCost(void)
{
    Tcg_LoadCard1(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcg->tempPlayAreaLocation));
    return Tcg_GetLoadedCard1RetreatCost();
}

// all the turn holder's cards `cardId` in the play area to the discard pile (the attached PlusPowers
// and Defenders, at the end of a turn)
void Tcg_MoveCardToDiscardPileIfInPlayArea(u32 cardId)
{
    u32 i;
    for (i = 0; i < TCG_DECK_SIZE; i++)
        if ((*Tcg_TurnVar(TCG_DUELVARS_CARD_LOCATIONS + i) & TCG_CARD_LOCATION_PLAY_AREA)
         && Tcg_GetCardIdFromDeckIndex(i) == cardId)
            Tcg_PutCardInDiscardPile(i);
}

// the damage on the turn holder's Pokémon at `location`, and its max HP
u32 Tcg_GetCardDamageAndMaxHP(u32 location, u32 *maxHp)
{
    Tcg_LoadCard2(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + location));
    if (maxHp)
        *maxHp = gTcg->loadedCard2->hp;
    return (u8)(gTcg->loadedCard2->hp - *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + location));
}

// TRUE: wLoadedAttack has the flag (an ATTACK_FLAG*: %fffffbbb, f the flag byte, b its bit)
bool32 Tcg_CheckLoadedAttackFlag(u32 flag)
{
    return (gTcg->loadedAttack.flags[(flag >> 3) & 0x1F] >> (flag & 7)) & 1;
}

// ---- home/substatus.asm ---------------------------------------------------------------------

// doubled if Swords Dance or Focus Energy was used last turn by the turn holder's arena Pokémon
u16 Tcg_HandleDoubleDamageSubstatus(u16 de)
{
    if ((*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS3) & (1 << TCG_SUBSTATUS3_THIS_TURN_DOUBLE_DAMAGE_F)) && de)
        de <<= 1;
    return de;
}

// the attacker's (non-turn holder's arena card's) SUBSTATUS2 that lowers its damage this turn, and
// the defender's (turn holder's) SUBSTATUS1 and Pokémon Powers
u16 Tcg_HandleDamageReduction(u16 de)
{
    de = Tcg_HandleDamageReductionExceptSubstatus2(de);
    switch (*Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS2))
    {
    case TCG_SUBSTATUS2_REDUCE_BY_20:
        return de - 20;
    case TCG_SUBSTATUS2_POUNCE:
    case TCG_SUBSTATUS2_GROWL:
        return de - 10;
    }
    return de;
}

// the game's halving: "sla d" (a bug: it should be sra d), "rr e", then down to a multiple of 10
static u16 HalveDamage(u16 de)
{
    u32 d = de >> 8, e = de & 0xFF, carry = (d >> 7) & 1;
    d = (d << 1) & 0xFF;
    e = (e >> 1) | (carry << 7);
    de = (d << 8) | e;
    if (e & 1)
        de -= 5;
    return de;
}

static u16 PkmnPowerDamageReduction(u16 de)
{
    if (gTcg->loadedAttack.category == TCG_POKEMON_POWER)
        return de;
    if (gTcg->tempNonTurnDuelistCardId == TCG_CARD_MR_MIME)
    {
        // Invisible Wall: 30 or more is prevented
        if (de >= 30)
            de = 0;
    }
    // (Kabuto Armor: no Kabuto)
    return de;
}

u16 Tcg_HandleDamageReductionExceptSubstatus2(u16 de)
{
    if (gTcg->noDamageOrEffect)
        return 0;
    switch (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS1))
    {
    case 0:
        break;
    case TCG_SUBSTATUS1_NO_DAMAGE_STIFFEN:
    case TCG_SUBSTATUS1_NO_DAMAGE_WITHDRAW:
    case TCG_SUBSTATUS1_NO_DAMAGE_HIDE_IN_SHELL:
    case TCG_SUBSTATUS1_NO_DAMAGE_SCRUNCH:
        return 0;
    case TCG_SUBSTATUS1_REDUCE_BY_10:
        return de - 10;
    case TCG_SUBSTATUS1_REDUCE_BY_20:
        return de - 20;
    case TCG_SUBSTATUS1_PREVENT_LESS_THAN_40:
        return de < 40 ? 0 : de;
    case TCG_SUBSTATUS1_HALVE_DAMAGE:
        return HalveDamage(de);
    }
    if (Tcg_CheckIsIncapableOfUsingPkmnPower(TCG_PLAY_AREA_ARENA))
        return de;
    return PkmnPowerDamageReduction(de);
}

// Invisible Wall (and the game's Kabuto Armor, Neutralizing Shield and Transparency) for a benched
// Pokémon: (none of them can make it 0 here: no Mew Lv8 or Haunter Lv17)
u16 Tcg_HandleDamageReductionOrNoDamageFromPkmnPowerEffects(u16 de)
{
    if (gTcg->loadedAttack.category == TCG_POKEMON_POWER)
        return de;
    if (gTcg->tempPlayAreaLocationCceb != TCG_PLAY_AREA_ARENA)
        de = PkmnPowerDamageReduction(de);
    return de;
}

// When Machamp is damaged with its Strikes Back active, the attacking Pokémon (the turn holder's arena
// Pokémon) takes 10 damage (a damaging attack's effect: `de`, not 0)
void Tcg_HandleStrikesBack_AgainstDamagingAttack(u16 de)
{
    u8 *hp, oldHp;
    if (de == 0 || gTcg->isDamageToSelf || gTcg->tempNonTurnDuelistCardId != TCG_CARD_MACHAMP)
        return;
    if (gTcg->loadedAttack.category == TCG_POKEMON_POWER)
        return;
    if (gTcg->tempPlayAreaLocationCceb == TCG_PLAY_AREA_ARENA && Tcg_CheckIsIncapableOfUsingPkmnPower(TCG_PLAY_AREA_ARENA))
        return;
    Tcg_SwapTurn();
    Tcg_LoadCard2(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
    hp = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP);
    oldHp = *hp;
    Tcg_SubtractHP(hp, 10);
    gTcg->tempNonTurnDuelistCardId = gTcg->loadedCard2Id;
    Tcg_LoadTxRam3(10);
    Tcg_LoadTxRam2(gTcg->loadedCard2->name);
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ReceivesDamageDueToStrikesBack);
    if (oldHp)
        Tcg_PrintPlayAreaCardKnockedOutIfNoHP(TCG_PLAY_AREA_ARENA);
    Tcg_SwapTurn();
}

// TRUE: the turn holder's arena Pokémon can't attack (Tail Wag, Leer, Bone Attack): hlText says why
bool32 Tcg_HandleCantAttackSubstatus(void)
{
    switch (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS2))
    {
    case TCG_SUBSTATUS2_TAIL_WAG:
        gTcg->hlText = TCG_TEXT_UnableToAttackDueToTailWag;
        return TRUE;
    case TCG_SUBSTATUS2_LEER:
        gTcg->hlText = TCG_TEXT_UnableToAttackDueToLeer;
        return TRUE;
    case TCG_SUBSTATUS2_BONE_ATTACK:
        gTcg->hlText = TCG_TEXT_UnableToAttackDueToBoneAttack;
        return TRUE;
    }
    return FALSE;
}

// TRUE: Amnesia stops the turn holder's arena Pokémon using wSelectedAttack
bool32 Tcg_HandleAmnesiaSubstatus(void)
{
    if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS2) != TCG_SUBSTATUS2_AMNESIA)
        return FALSE;
    if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_DISABLED_ATTACK_INDEX) != gTcg->selectedAttack)
        return FALSE;
    gTcg->hlText = TCG_TEXT_UnableToUseAttackDueToAmnesia;
    return TRUE;
}

// TRUE: Sand Attack or Smokescreen made the turn holder's attack fail (a coin: tails)
bool32 Tcg_HandleSandAttackOrSmokescreenSubstatus(void)
{
    if (!Tcg_CheckSandAttackOrSmokescreenSubstatus())
        return FALSE;
    gTcg->gotHeadsFromSandAttackOrSmokescreenCheck = Tcg_TossCoin(gTcg->deText);
    if (gTcg->gotHeadsFromSandAttackOrSmokescreenCheck)
        return FALSE;
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_AttackUnsuccessful);
    return TRUE;
}

// TRUE: the turn holder's arena Pokémon is under Sand Attack or Smokescreen, and hasn't got heads
// for it yet (deText: the coin's text)
bool32 Tcg_CheckSandAttackOrSmokescreenSubstatus(void)
{
    switch (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS2))
    {
    case TCG_SUBSTATUS2_SAND_ATTACK:
        gTcg->deText = TCG_TEXT_SandAttackCheck;
        break;
    case TCG_SUBSTATUS2_SMOKESCREEN:
        gTcg->deText = TCG_TEXT_SmokescreenCheck;
        break;
    default:
        return FALSE;
    }
    return !gTcg->gotHeadsFromSandAttackOrSmokescreenCheck;
}

// TRUE: the defending (turn holder's arena) Pokémon is under Fly, Barrier or Agility (no damage or
// effect this turn): wNoDamageOrEffect says which, hlText the text
bool32 Tcg_HandleNoDamageOrEffectSubstatus(void)
{
    u32 which;
    gTcg->noDamageOrEffect = 0;
    if (gTcg->loadedAttack.category == TCG_POKEMON_POWER)
        return FALSE;
    switch (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS1))
    {
    case TCG_SUBSTATUS1_FLY:
        which = TCG_NO_DAMAGE_OR_EFFECT_FLY;
        gTcg->hlText = TCG_TEXT_NoDamageOrEffectDueToFly;
        break;
    case TCG_SUBSTATUS1_BARRIER:
        which = TCG_NO_DAMAGE_OR_EFFECT_BARRIER;
        gTcg->hlText = TCG_TEXT_NoDamageOrEffectDueToBarrier;
        break;
    case TCG_SUBSTATUS1_AGILITY:
        which = TCG_NO_DAMAGE_OR_EFFECT_AGILITY;
        gTcg->hlText = TCG_TEXT_NoDamageOrEffectDueToAgility;
        break;
    default:
        // (Mew Lv8's Neutralizing Shield: no Mew Lv8)
        return FALSE;
    }
    gTcg->noDamageOrEffect = which;
    return TRUE;
}

// TRUE: wNoDamageOrEffect is set; hlText: its text the first time, then TCG_TEXT_NONE
bool32 Tcg_CheckNoDamageOrEffect(void)
{
    static const u16 sTexts[] =
    {
        [TCG_NO_DAMAGE_OR_EFFECT_AGILITY - 1] = TCG_TEXT_NoDamageOrEffectDueToAgility,
        [TCG_NO_DAMAGE_OR_EFFECT_BARRIER - 1] = TCG_TEXT_NoDamageOrEffectDueToBarrier,
        [TCG_NO_DAMAGE_OR_EFFECT_FLY - 1] = TCG_TEXT_NoDamageOrEffectDueToFly,
        [TCG_NO_DAMAGE_OR_EFFECT_TRANSPARENCY - 1] = TCG_TEXT_NoDamageOrEffectDueToTransparency,
        [TCG_NO_DAMAGE_OR_EFFECT_NSHIELD - 1] = TCG_TEXT_NoDamageOrEffectDueToNShield,
    };
    u32 a = gTcg->noDamageOrEffect;
    if (a == 0)
        return FALSE;
    if (a & 0x80)
    {
        gTcg->hlText = TCG_TEXT_NONE;
        return TRUE;
    }
    gTcg->noDamageOrEffect |= 0x80;
    gTcg->hlText = sTexts[(a - 1) % ARRAY_COUNT(sTexts)];
    return TRUE;
}

// TRUE: the turn holder's Pokémon at `location` can't use its Pokémon Power (the arena Pokémon:
// asleep, confused or paralyzed; the game's Toxic Gas: no Muk). hlText: why.
bool32 Tcg_CheckIsIncapableOfUsingPkmnPower(u32 location)
{
    if (location == TCG_PLAY_AREA_ARENA && (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS) & TCG_CNF_SLP_PRZ))
    {
        gTcg->hlText = TCG_TEXT_CannotUseDueToStatus;
        return TRUE;
    }
    gTcg->hlText = TCG_TEXT_UnableDueToToxicGas;
    return FALSE;
}

// how many of Pokémon `cardId` the turn holder has in play that can use its Pokémon Power (an arena
// one asleep, confused or paralyzed doesn't count)
u32 Tcg_CountTurnDuelistPokemonWithActivePkmnPower(u32 cardId)
{
    u32 n = 0, i;
    u8 arena = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD);
    const u8 *bench = Tcg_TurnVar(TCG_DUELVARS_BENCH);
    if (arena != 0xFF && Tcg_GetCardIdFromDeckIndex(arena) == cardId
     && !(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS) & TCG_CNF_SLP_PRZ))
        n++;
    for (i = 0; i < TCG_MAX_PLAY_AREA_POKEMON - 1 && bench[i] != 0xFF; i++)
        if (Tcg_GetCardIdFromDeckIndex(bench[i]) == cardId)
            n++;
    return n;
}

u32 Tcg_CountPokemonWithActivePkmnPowerInBothPlayAreas(u32 cardId)
{
    u32 n = Tcg_CountTurnDuelistPokemonWithActivePkmnPower(cardId);
    Tcg_SwapTurn();
    n += Tcg_CountTurnDuelistPokemonWithActivePkmnPower(cardId);
    Tcg_SwapTurn();
    return n;
}

// wLoadedCard1's retreat cost (the game's Dodrio Retreat Aid: no Dodrio)
u32 Tcg_GetLoadedCard1RetreatCost(void)
{
    return gTcg->loadedCard1->retreatCost;
}

// TRUE: Acid stops the turn holder's arena Pokémon retreating (hlText)
bool32 Tcg_CheckUnableToRetreatDueToEffect(void)
{
    if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS2) != TCG_SUBSTATUS2_ACID)
        return FALSE;
    gTcg->hlText = TCG_TEXT_UnableToRetreatDueToAcid;
    return TRUE;
}

// TRUE: Headache stops the turn holder using Trainer cards (hlText)
bool32 Tcg_CheckCantUseTrainerDueToEffect(void)
{
    if (!(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS3) & (1 << TCG_SUBSTATUS3_HEADACHE_F)))
        return FALSE;
    gTcg->hlText = TCG_TEXT_UnableToUseTrainerDueToHeadache;
    return TRUE;
}

// (IsPrehistoricPowerActive: no Aerodactyl; IsClairvoyanceActive: no Omanyte)
bool32 Tcg_IsPrehistoricPowerActive(void)
{
    return FALSE;
}

bool32 Tcg_IsClairvoyanceActive(void)
{
    return FALSE;
}

// the SUBSTATUS2s that lower an attack's damage, or stop the defender attacking the Pokémon that
// caused them, cleared from the turn holder's arena Pokémon
void Tcg_ClearDamageReductionSubstatus2(void)
{
    u8 *s = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS2);
    switch (*s)
    {
    case TCG_SUBSTATUS2_REDUCE_BY_20:
    case TCG_SUBSTATUS2_POUNCE:
    case TCG_SUBSTATUS2_GROWL:
    case TCG_SUBSTATUS2_TAIL_WAG:
    case TCG_SUBSTATUS2_LEER:
        *s = 0;
    }
}

// the turn holder starting their turn: SUBSTATUS1 cleared, a double damage for next turn now this turn's
void Tcg_UpdateSubstatusConditions_StartOfTurn(void)
{
    u8 *s1 = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS1), old = *s1;
    *s1 = 0;
    if (old == TCG_SUBSTATUS1_NEXT_TURN_DOUBLE_DAMAGE)
        *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS3) |= 1 << TCG_SUBSTATUS3_THIS_TURN_DOUBLE_DAMAGE_F;
}

// the turn holder ending their turn: SUBSTATUS2 and Headache cleared, and this turn's double damage
// (unless it's for next turn)
void Tcg_UpdateSubstatusConditions_EndOfTurn(void)
{
    u8 *s3 = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS3);
    *s3 &= ~(1 << TCG_SUBSTATUS3_HEADACHE_F);
    *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS2) = 0;
    if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS1) != TCG_SUBSTATUS1_NEXT_TURN_DOUBLE_DAMAGE)
        *s3 &= ~(1 << TCG_SUBSTATUS3_THIS_TURN_DOUBLE_DAMAGE_F);
}

// TRUE: the turn holder has a Blastoise whose Rain Dance can be used
bool32 Tcg_IsRainDanceActive(void)
{
    return Tcg_CountTurnDuelistPokemonWithActivePkmnPower(TCG_CARD_BLASTOISE) != 0;
}

// TRUE: hTempCardIndex_ff98 is a Water Energy and hTempPlayAreaLocation_ff9d's Pokémon is Water
bool32 Tcg_CheckRainDanceScenario(void)
{
    if (GetCardType(Tcg_GetCardIdFromDeckIndex(gTcg->tempCardIndex)) != TCG_TYPE_ENERGY_WATER)
        return FALSE;
    return Tcg_GetPlayAreaCardColor(gTcg->tempPlayAreaLocation) == TCG_TYPE_PKMN_WATER;
}

// Destiny Bond: if the defending (non-turn) Pokémon's HP is 0 and the attacker's isn't, the attacker
// is knocked out too
void Tcg_HandleDestinyBondSubstatus(void)
{
    u8 *hp;
    if (*Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_SUBSTATUS1) != TCG_SUBSTATUS1_DESTINY_BOND)
        return;
    if (*Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD) == 0xFF || *Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_HP))
        return;
    hp = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP);
    if (*hp == 0)
        return;
    *hp = 0;
    Tcg_DrawDuelMainScene();
    Tcg_DrawDuelHUDs();
    Tcg_LoadCard2(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
    Tcg_LoadTxRam2(gTcg->loadedCard2->name);
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_KnockedOutDueToDestinyBond);
}

// Strikes Back against a RESIDUAL-category attack (one that does its damage after the others)
void Tcg_HandleStrikesBack_AgainstResidualAttack(void)
{
    u8 *hp, oldHp;
    if (gTcg->tempNonTurnDuelistCardId != TCG_CARD_MACHAMP)
        return;
    if (gTcg->loadedAttack.category & TCG_RESIDUAL)
        return;
    if ((gTcg->dealtDamage & 0xFF) == 0)    // (the game checks wDealtDamage's low byte)
        return;
    Tcg_SwapTurn();
    if (Tcg_CheckIsIncapableOfUsingPkmnPower(TCG_PLAY_AREA_ARENA))
    {
        Tcg_SwapTurn();
        return;
    }
    Tcg_SwapTurn();
    // ApplyStrikesBack_AgainstResidualAttack
    Tcg_LoadTxRam3(10);
    gTcg->loadedCard2Id = gTcg->tempTurnDuelistCardId;
    gTcg->loadedCard2 = Tcg_Card(gTcg->loadedCard2Id);
    Tcg_LoadTxRam2(gTcg->loadedCard2->name);
    hp = Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP);
    oldHp = *hp;
    Tcg_SubtractHP(hp, 10);
    Tcg_DrawWideTextBox_PrintText(TCG_TEXT_ReceivesDamageDueToStrikesBack);
    Tcg_WaitForWideTextBoxInput();
    if (oldHp == 0)
        return;
    Tcg_PrintPlayAreaCardKnockedOutIfNoHP(TCG_PLAY_AREA_ARENA);
    Tcg_DrawDuelHUDs();
}

// ---- home/card_color.asm --------------------------------------------------------------------

// the turn holder's Pokémon's color (the game's Venomoth Shift: no Venomoth). A Trainer: Colorless.
u32 Tcg_GetPlayAreaCardColor(u32 location)
{
    u32 type = GetCardType(Tcg_GetCardIdFromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + location)));
    return type == TCG_TYPE_TRAINER ? TCG_COLORLESS : type;
}

u32 Tcg_GetArenaCardColor(void)
{
    return Tcg_GetPlayAreaCardColor(TCG_PLAY_AREA_ARENA);
}

static u32 GetCardWeakness(u32 var)
{
    Tcg_LoadCard2(*Tcg_TurnVar(var));
    return gTcg->loadedCard2->weakness;
}

static u32 GetCardResistance(u32 var)
{
    Tcg_LoadCard2(*Tcg_TurnVar(var));
    return gTcg->loadedCard2->resistance;
}

u32 Tcg_GetArenaCardWeakness(void)
{
    u32 changed = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_CHANGED_WEAKNESS);
    return changed ? changed : GetCardWeakness(TCG_DUELVARS_ARENA_CARD);
}

u32 Tcg_GetPlayAreaCardWeakness(u32 location)
{
    return location == TCG_PLAY_AREA_ARENA ? Tcg_GetArenaCardWeakness() : GetCardWeakness(TCG_DUELVARS_ARENA_CARD + location);
}

u32 Tcg_GetArenaCardResistance(void)
{
    u32 changed = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_CHANGED_RESISTANCE);
    return changed ? changed : GetCardResistance(TCG_DUELVARS_ARENA_CARD);
}

u32 Tcg_GetPlayAreaCardResistance(u32 location)
{
    return location == TCG_PLAY_AREA_ARENA ? Tcg_GetArenaCardResistance() : GetCardResistance(TCG_DUELVARS_ARENA_CARD + location);
}

// Charizard's Energy Burn: all of wAttachedEnergies but Double Colorless counts as Fire
void Tcg_HandleEnergyBurn(void)
{
    u32 i;
    if (Tcg_GetCardIdFromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD)) != TCG_CARD_CHARIZARD)
        return;
    if (Tcg_CheckIsIncapableOfUsingPkmnPower(TCG_PLAY_AREA_ARENA))
        return;
    for (i = 0; i < TCG_NUM_COLORED_TYPES; i++)
        gTcg->attachedEnergies[i] = 0;
    gTcg->attachedEnergies[TCG_FIRE] = gTcg->totalAttachedEnergies;
}

// ---- home/coin_toss.asm, engine/duel/core.asm's _TossCoin -----------------------------------

// `count` coins (the turn holder's), `textId` shown with them: the number of heads
u32 Tcg_TossCoinATimes(u32 count, u16 textId)
{
    u32 i;
    gTcg->coinTossTotalNum = count;
    gTcg->coinTossNumHeads = 0;
    for (i = 0; i < count; i++)
    {
        bool32 heads = Tcg_Random(2);
        gTcg->coinTossNumHeads += heads;
        TcgUi_CoinToss(textId, i, count, heads);
    }
    gTcg->coinTossNumTossed = count;
    return gTcg->coinTossNumHeads;
}

// one coin: TRUE, heads
bool32 Tcg_TossCoin(u16 textId)
{
    bool32 heads = Tcg_TossCoinATimes(1, textId);
    TcgUi_CoinTossDone();
    return heads;
}

// ---- engine/duel/core.asm -------------------------------------------------------------------

// each status condition queued (wStatusConditionQueue), in order. TRUE: some were, and the defender
// didn't have "no damage or effect"
bool32 Tcg_ApplyStatusConditionQueue(void)
{
    u8 *q = gTcg->statusConditionQueue;
    u32 i;
    gTcg->arenaCardLastTurnStatus[0] = gTcg->arenaCardLastTurnStatus[1] = 0;
    if (gTcg->statusConditionQueueIndex == 0)
        return FALSE;
    q[gTcg->statusConditionQueueIndex] = 0;
    if (!Tcg_CheckNoDamageOrEffect())
    {
        for (i = 0; q[i]; i += 3)
            Tcg_ApplyStatusConditionToArenaPokemon(&q[i]);
        return TRUE;
    }
    if (gTcg->hlText)
        Tcg_DrawWideTextBox_PrintText(gTcg->hlText);
    // no damage or effect to the defender: only those on the attacker's own Pokémon
    for (i = 0; q[i]; i += 3)
        if (q[i] == TCG_QUEUE_DUELIST(gTcg->whoseTurnReal))
            Tcg_ApplyStatusConditionToArenaPokemon(&q[i]);
    return FALSE;
}

// a queue entry: (duelist, the statuses kept, the status added)
void Tcg_ApplyStatusConditionToArenaPokemon(const u8 *entry)
{
    u8 *vars = gTcg->vars[(entry[0] - TCG_QUEUE_DUELIST(0)) & 1];
    vars[TCG_DUELVARS_ARENA_CARD_STATUS] = (vars[TCG_DUELVARS_ARENA_CARD_STATUS] & entry[1]) | entry[2];
    vars[TCG_DUELVARS_ARENA_CARD_LAST_TURN_STATUS] = (vars[TCG_DUELVARS_ARENA_CARD_LAST_TURN_STATUS] & entry[1]) | entry[2];
}

// the number of the turn holder's Pokémon just knocked out (wNumberPrizeCardsToTake): in play with
// 0 HP. TRUE: some were.
static bool32 CountKnockedOutPokemon(void)
{
    u32 i, n = 0;
    for (i = 0; i < TCG_MAX_PLAY_AREA_POKEMON; i++)
    {
        u8 card = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + i);
        if (card == 0xFF || *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + i))
            continue;
        if (GetCardType(Tcg_GetCardIdFromDeckIndex(card)) != TCG_TYPE_TRAINER)
            n++;
    }
    gTcg->numberPrizeCardsToTake = n;
    return n != 0;
}

// TRUE: all the turn holder's Pokémon in play are knocked out
static bool32 CheckIfTurnDuelistPlayAreaPokemonAreAllKnockedOut(void)
{
    u32 i, n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    for (i = 0; i < n; i++)
        if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + i))
            return FALSE;
    return TRUE;
}

// the turn holder's prize at `prize` (0-5) taken into their hand
void Tcg_TakePrizeCard(u32 prize)
{
    *Tcg_TurnVar(TCG_DUELVARS_PRIZES) &= ~(1 << prize);
    Tcg_AddCardToHand(*Tcg_TurnVar(TCG_DUELVARS_PRIZE_CARDS + prize));
}

// the AI's prizes: random ones (engine/duel/ai/core.asm's AIPickPrizeCards)
void Tcg_AIPickPrizeCards(void)
{
    u32 n = gTcg->numberPrizeCardsToTake;
    while (n--)
    {
        u32 prize, prizes = *Tcg_TurnVar(TCG_DUELVARS_PRIZES);
        if (!prizes)
            break;
        do
            prize = Tcg_Random(6);
        while (!(prizes & (1 << prize)));
        Tcg_TakePrizeCard(prize);
        if (*Tcg_TurnVar(TCG_DUELVARS_PRIZES) == 0)
            break;
    }
}

// the turn holder takes wNumberPrizeCardsToTake prizes. TRUE: none are left.
static bool32 TurnDuelistTakePrizes(void)
{
    Tcg_FinishQueuedAnimations();
    Tcg_LoadTxRam3(gTcg->numberPrizeCardsToTake);
    if (*Tcg_TurnVar(TCG_DUELVARS_DUELIST_TYPE) == TCG_DUELIST_TYPE_PLAYER)
    {
        Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_WillDrawNPrizes);
        Tcg_SelectPrizeCards(gTcg->numberPrizeCardsToTake);
    }
    else
    {
        u32 remaining;
        Tcg_DrawYourOrOppPlayAreaScreen(gTcg->whoseTurn);
        Tcg_DrawWideTextBox_PrintText(TCG_TEXT_WillDrawNPrizes);
        remaining = Tcg_CountPrizes();
        Tcg_AIPickPrizeCards();
        Tcg_DoFrames(60);
        if (remaining < gTcg->numberPrizeCardsToTake)
            Tcg_LoadTxRam3(remaining);
        Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_DrewNPrizes);
    }
    return *Tcg_TurnVar(TCG_DUELVARS_PRIZES) == 0;
}

// TakeAPrizes: the turn holder's prizes, `count` fewer (for a win by knock-outs: they're all taken)
static void TakeAPrizes(u32 count)
{
    static const u8 sPrizeBitmasks[] = {0, 1, 3, 7, 15, 31, 63};
    s32 left;
    if (count == 0)
        return;
    left = (s32)Tcg_CountPrizes() - (s32)count;
    if (left < 0)
        left = 0;
    *Tcg_TurnVar(TCG_DUELVARS_PRIZES) = sPrizeBitmasks[left];
}

// TRUE: the turn holder has no Pokémon with HP on their bench (HasAlivePokemonInBench's carry)
static bool32 HasNoAlivePokemonInBench(void)
{
    u32 i, n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    for (i = 1; i < n; i++)
        if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + i))
            return FALSE;
    return TRUE;
}

// the turn holder replaces their knocked-out arena Pokémon. TRUE: there's none to replace it.
static bool32 ReplaceKnockedOutPokemon(void)
{
    if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP))
        return FALSE;
    Tcg_ClearAllStatusConditions();
    if (HasNoAlivePokemonInBench())
    {
        Tcg_DrawDuelMainScene();
        Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_ThereAreNoPokemonInPlayArea);
        return TRUE;
    }
    if (*Tcg_TurnVar(TCG_DUELVARS_DUELIST_TYPE) == TCG_DUELIST_TYPE_PLAYER)
    {
        Tcg_DrawDuelMainScene();
        Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_SelectPokemonToPlaceInTheArena);
        do
            gTcg->tempPlayAreaLocation = Tcg_OpenPlayAreaScreenForSelection(TRUE);
        while (gTcg->tempPlayAreaLocation == TCG_SELECTION_CANCEL);
    }
    else
    {
        gTcg->tempPlayAreaLocation = TcgAi_KOSwitch();
    }
    Tcg_FinishQueuedAnimations();
    Tcg_SwapPlayAreaPokemon(gTcg->tempPlayAreaLocation, TCG_PLAY_AREA_ARENA);
    Tcg_DisplayCardDetailScreen(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD), TCG_TEXT_DuelistPlacedACard);
    return FALSE;
}

// all the turn holder's Pokémon in play with 0 HP to the discard pile
static void MoveAllTurnHolderKnockedOutPokemonToDiscardPile(void)
{
    u32 i, n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    for (i = 0; i < n; i++)
        if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + i) == 0)
            Tcg_MovePlayAreaCardToDiscardPile(i);
}

// Func_6fa5: if the turn holder's Pokémon were knocked out, the other duelist takes prizes.
// TRUE: they took the last.
static bool32 Func_6fa5(void)
{
    bool32 tookAll;
    if (!CountKnockedOutPokemon())
        return FALSE;
    Tcg_SwapTurn();
    tookAll = TurnDuelistTakePrizes();
    Tcg_SwapTurn();
    if (!tookAll)
        return FALSE;
    Tcg_SwapTurn();
    Tcg_DrawDuelMainScene();
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_TookAllThePrizes);
    Tcg_SwapTurn();
    return TRUE;
}

static void ClearKnockedOutDamageReductionSubstatus2(void)
{
    Tcg_SwapTurn();
    if (*Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_HP) == 0)
        Tcg_ClearDamageReductionSubstatus2();
    Tcg_SwapTurn();
    if (*Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_HP) == 0)
        Tcg_ClearDamageReductionSubstatus2();
}

// Destiny Bond, then the knock-outs: prizes taken, knocked-out Pokémon replaced and discarded, and
// whether the duel's over (wDuelFinished). TRUE: it is.
bool32 Tcg_HandleDestinyBondAndBetweenTurnKnockOuts(void)
{
    Tcg_HandleDestinyBondSubstatus();
    return Tcg_HandleBetweenTurnKnockOuts();
}

// the knock-outs (between turns, or after an attack): prizes taken, knocked-out Pokémon replaced
// and discarded, and whether the duel's over (wDuelFinished). TRUE: it is.
bool32 Tcg_HandleBetweenTurnKnockOuts(void)
{
    static const u8 sResults[] =
    {
        TCG_DUEL_NOT_FINISHED, TCG_TURN_PLAYER_LOST, TCG_TURN_PLAYER_WON,  TCG_TURN_PLAYER_TIED,
        TCG_TURN_PLAYER_LOST,  TCG_TURN_PLAYER_LOST, TCG_TURN_PLAYER_TIED, TCG_TURN_PLAYER_LOST,
        TCG_TURN_PLAYER_WON,   TCG_TURN_PLAYER_TIED, TCG_TURN_PLAYER_WON,  TCG_TURN_PLAYER_WON,
        TCG_TURN_PLAYER_TIED,  TCG_TURN_PLAYER_LOST, TCG_TURN_PLAYER_WON,  TCG_TURN_PLAYER_TIED,
    };
    bool32 finished = FALSE;
    // wDuelFinishParam gathers four results, a bit each (the game's rl [hl])
    ClearKnockedOutDamageReductionSubstatus2();
    gTcg->duelFinishParam = 0;
    Tcg_SwapTurn();
    gTcg->duelFinishParam = (gTcg->duelFinishParam << 1) | Func_6fa5();
    Tcg_SwapTurn();
    if (gTcg->duelFinishParam && !CheckIfTurnDuelistPlayAreaPokemonAreAllKnockedOut())
    {
        u32 knockedOut, prizes;
        CountKnockedOutPokemon();
        knockedOut = gTcg->numberPrizeCardsToTake;
        Tcg_SwapTurn();
        prizes = Tcg_CountPrizes();
        Tcg_SwapTurn();
        if ((u8)(prizes - 1) >= knockedOut)     // (the game: "dec a, cp c, jr c")
        {
            Tcg_SwapTurn();
            TakeAPrizes(knockedOut);
            Tcg_SwapTurn();
            gTcg->duelFinished = TCG_TURN_PLAYER_WON;
            finished = TRUE;
            goto discard;
        }
    }
    gTcg->duelFinishParam = (gTcg->duelFinishParam << 1) | Func_6fa5();
    if (gTcg->duelFinishParam == TRUE)
    {
        bool32 allOut;
        Tcg_SwapTurn();
        allOut = CheckIfTurnDuelistPlayAreaPokemonAreAllKnockedOut();
        Tcg_SwapTurn();
        if (!allOut)
        {
            gTcg->duelFinished = TCG_TURN_PLAYER_LOST;
            finished = TRUE;
            goto discard;
        }
    }
    Tcg_SwapTurn();
    gTcg->duelFinishParam = (gTcg->duelFinishParam << 1) | ReplaceKnockedOutPokemon();
    Tcg_SwapTurn();
    gTcg->duelFinishParam = (gTcg->duelFinishParam << 1) | ReplaceKnockedOutPokemon();
    if (gTcg->duelFinishParam)
    {
        gTcg->duelFinished = sResults[gTcg->duelFinishParam & 0xF];
        finished = TRUE;
    }
discard:
    MoveAllTurnHolderKnockedOutPokemonToDiscardPile();
    Tcg_SwapTurn();
    MoveAllTurnHolderKnockedOutPokemonToDiscardPile();
    Tcg_SwapTurn();
    Tcg_ShiftAllPokemonToFirstPlayAreaSlots();
    return finished;
}

// the non-turn holder's duel variables that last a two-player turn at most cleared
void Tcg_ClearNonTurnTemporaryDuelvars(void)
{
    u32 i;
    for (i = 0; i < 8; i++)
        *Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_DISABLED_ATTACK_INDEX + i) = 0;
}

// the non-turn holder's DUELVARS_ARENA_CARD_LAST_TURN_DAMAGE: wDealtDamage (0 if they were forced to
// switch)
void Tcg_UpdateArenaCardLastTurnDamage(void)
{
    u16 d = gTcg->defendingWasForcedToSwitch ? 0 : gTcg->dealtDamage;
    *Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_DAMAGE) = d & 0xFF;
    *Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD_LAST_TURN_DAMAGE + 1) = d >> 8;
}

// the "There was no effect from ..." text for wNoEffectFromWhichStatus
u16 Tcg_PrintThereWasNoEffectFromStatusText(void)
{
    u32 status = gTcg->noEffectFromWhichStatus;
    if (status == 0)
    {
        Tcg_LoadTxRam2(gTcg->loadedAttack.name);
        return TCG_TEXT_ThereWasNoEffectFromTxRam2;
    }
    if (status == (TCG_POISONED | TCG_CONFUSED))
        return TCG_TEXT_ThereWasNoEffectFromPoisonConfusion;
    if (status & TCG_PSN_DBLPSN)
        return TCG_TEXT_ThereWasNoEffectFromPoison;
    switch (status & TCG_CNF_SLP_PRZ)
    {
    case TCG_PARALYZED:
        return TCG_TEXT_ThereWasNoEffectFromParalysis;
    case TCG_ASLEEP:
        return TCG_TEXT_ThereWasNoEffectFromSleep;
    }
    return TCG_TEXT_ThereWasNoEffectFromConfusion;
}
