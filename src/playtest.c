// Pokémon Canvas: hooks for automated playtests (see include/playtest.h). Built only with
// `make firered PLAYTEST=1`.
#include "global.h"
#include "gym_tera.h"
#include "playtest.h"

#if PLAYTEST
#include "battle.h"
#include "event_data.h"
#include "script.h"
#include "item.h"
#include "pokemon.h"
#include "script_pokemon_util.h"
#include "battle_setup.h"
#include "pokemon_storage_system.h"
#include "move.h"
#include "battle_util.h"
#include "constants/moves.h"
#include "constants/battle.h"
#include "constants/trainers.h"
#include "gym_decor.h"

EWRAM_DATA struct Playtest gPlaytest = {0};
// (the harness reads these by offset: tools/gym_hack/playtest/)
STATIC_ASSERT(sizeof(struct PlaytestBattle) == 80, PlaytestBattleSize);
STATIC_ASSERT(sizeof(struct PlaytestPartyMon) == 32, PlaytestPartyMonSize);
STATIC_ASSERT(offsetof(struct Playtest, log) == 24, PlaytestLogOffset);
STATIC_ASSERT(offsetof(struct Playtest, party) == 24 + 80 * PLAYTEST_LOG_SIZE, PlaytestPartyOffset);
STATIC_ASSERT(offsetof(struct Playtest, asserts) == 24 + 80 * PLAYTEST_LOG_SIZE + 32 * PARTY_SIZE + 4, PlaytestAssertsOffset);
static EWRAM_DATA u16 sHpBefore[PARTY_SIZE] = {0};
static EWRAM_DATA u16 sOpponentSpecies[PARTY_SIZE] = {0};
static EWRAM_DATA u8 sOpponentLevel[PARTY_SIZE] = {0};

static void Init(void)
{
    if (gPlaytest.magic != 0x59414C50)
    {
        memset(&gPlaytest, 0, sizeof(gPlaytest));
        gPlaytest.magic = 0x59414C50;
    }
}

// The bot is playing (moves learned and the like are decided for it)
bool32 Playtest_Active(void)
{
    return gPlaytest.magic == 0x59414C50 && gPlaytest.autoBattle;
}

// The AI battles for the player: trainer battles (in wild ones the bot presses A, and the move
// cursor starts on the best move: Playtest_BestMove)
bool32 Playtest_AutoBattle(void)
{
    return FALSE;   // (the battle engine's AI-vs-AI mode: not used, see Playtest_ChooseMove)
}

// How good a move is against one foe: power, same-type bonus, type effectiveness, accuracy
static u32 MoveScoreAgainst(u32 battler, u32 slot, u32 target)
{
    u32 move = gBattleMons[battler].moves[slot], score, type, acc;
    if (move == MOVE_NONE || gBattleMons[battler].pp[slot] == 0)
        return 0;
    score = GetMovePower(move);
    if (score <= 1)
        return 1;
    if (GetMoveStrikeCount(move) > 1)
        score *= GetMoveStrikeCount(move);
    type = GetMoveType(move);
    if (type == GetSpeciesType(gBattleMons[battler].species, 0) || type == GetSpeciesType(gBattleMons[battler].species, 1))
        score = score * 3 / 2;
    score = score * GetTypeModifier(type, GetSpeciesType(gBattleMons[target].species, 0)) / 4096;
    if (GetSpeciesType(gBattleMons[target].species, 1) != GetSpeciesType(gBattleMons[target].species, 0))
        score = score * GetTypeModifier(type, GetSpeciesType(gBattleMons[target].species, 1)) / 4096;
    acc = GetMoveAccuracy(move);
    if (acc > 0 && acc <= 100)
        score = score * acc / 100;
    // physical or special: by the attacker's stronger attacking stat
    if (GetMoveCategory(move) == DAMAGE_CATEGORY_PHYSICAL)
        score = score * gBattleMons[battler].attack / max(1, gBattleMons[target].defense);
    else
        score = score * gBattleMons[battler].spAttack / max(1, gBattleMons[target].spDefense);
    return score + 2;
}

// The autopilot's move: the most damaging one, at the foe it hurts most
u32 Playtest_ChooseMove(u32 battler, u32 *target)
{
    u32 i, k, best = 0, bestScore = 0, unusable = CheckMoveLimitations(battler, 0, MOVE_LIMITATIONS_ALL);
    *target = GetOppositeBattler(battler);
    for (i = 0; i < MAX_MON_MOVES; i++)
        if (!(unusable & (1u << i)))
        {
            best = i;   // (something usable, whatever the scores)
            break;
        }
    // (the painter's Sketch quests: a Pokémon with Sketch waits for a foe's move, then sketches it)
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (gBattleMons[battler].moves[i] == MOVE_SKETCH && !(unusable & (1u << i)))
        {
            for (k = 0; k < gBattlersCount; k++)
            {
                if (!IsOnPlayerSide(k) && IsBattlerAlive(k) && gLastPrintedMoves[k] != MOVE_NONE && gLastPrintedMoves[k] != MOVE_UNAVAILABLE)
                {
                    *target = k;
                    return i;
                }
            }
            unusable |= 1u << i;    // (nothing to sketch yet: Sketch has 1 PP, so not now)
        }
    }
    for (k = 0; k < gBattlersCount; k++)
    {
        if (IsOnPlayerSide(k) || !IsBattlerAlive(k))
            continue;
        for (i = 0; i < MAX_MON_MOVES; i++)
        {
            u32 score = (unusable & (1u << i)) ? 0 : MoveScoreAgainst(battler, i, k);
            if (score > bestScore)
            {
                bestScore = score;
                best = i;
                *target = k;
            }
        }
    }
    return best;
}

u32 Playtest_BestMove(u32 battler)
{
    u32 target;
    return Playtest_ChooseMove(battler, &target);
}

// The autopilot's next Pokémon (after one faints): the one whose best move hurts the foe most,
// weighted by its HP left
u32 Playtest_ChooseSwitchIn(u32 battler)
{
    u32 i, k, best = PARTY_SIZE, bestScore = 0, foe = GetOppositeBattler(battler);
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gPlayerParty[i];
        u32 score = 0, hp = GetMonData(mon, MON_DATA_HP), maxHp = GetMonData(mon, MON_DATA_MAX_HP);
        bool32 active = FALSE;
        if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG) || hp == 0)
            continue;
        for (k = 0; k < gBattlersCount; k++)
            if (IsOnPlayerSide(k) && gBattlerPartyIndexes[k] == i && IsBattlerAlive(k))
                active = TRUE;
        if (active)
            continue;
        for (k = 0; k < MAX_MON_MOVES; k++)
        {
            u32 move = GetMonData(mon, MON_DATA_MOVE1 + k), s;
            if (move == MOVE_NONE || GetMovePower(move) <= 1)
                continue;
            s = GetMovePower(move);
            if (GetMoveType(move) == GetSpeciesType(GetMonData(mon, MON_DATA_SPECIES), 0))
                s = s * 3 / 2;
            s = s * GetTypeModifier(GetMoveType(move), GetSpeciesType(gBattleMons[foe].species, 0)) / 4096;
            if (GetSpeciesType(gBattleMons[foe].species, 1) != GetSpeciesType(gBattleMons[foe].species, 0))
                s = s * GetTypeModifier(GetMoveType(move), GetSpeciesType(gBattleMons[foe].species, 1)) / 4096;
            score = max(score, s);
        }
        score = (score + 10) * GetMonData(mon, MON_DATA_LEVEL) * (hp * 100 / max(1, maxHp) + 20) / 100;
        if (best == PARTY_SIZE || score > bestScore)
        {
            best = i;
            bestScore = score;
        }
    }
    return best;
}

static EWRAM_DATA u8 sOpponentTera[PARTY_SIZE] = {0};

void Playtest_BattleStart(void)
{
    u32 i;
    Init();
    for (i = 0; i < PARTY_SIZE; i++)
    {
        sHpBefore[i] = GetMonData(&gPlayerParty[i], MON_DATA_HP);
        sOpponentSpecies[i] = SPECIES_NONE;
    }
}

// (the foe's party, before anything can clear it)
void Playtest_FirstTurn(void)
{
    u32 i;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        sOpponentSpecies[i] = GetMonData(&gEnemyParty[i], MON_DATA_SPECIES);
        sOpponentLevel[i] = GetMonData(&gEnemyParty[i], MON_DATA_LEVEL);
        sOpponentTera[i] = GetMonData(&gEnemyParty[i], MON_DATA_TERA_TYPE);
    }
}

void Playtest_BattleEnd(void)
{
    struct PlaytestBattle *b;
    u32 i;
    Init();
    b = &gPlaytest.log[gPlaytest.numBattles % PLAYTEST_LOG_SIZE];
    memset(b, 0, sizeof(*b));
    b->trainer = (gBattleTypeFlags & BATTLE_TYPE_TRAINER) ? TRAINER_BATTLE_PARAM.opponentA : 0;
    b->outcome = gBattleOutcome;
    b->turns = gBattleResults.battleTurnCounter;
    b->playerFaints = gBattleResults.playerFaintCounter;
    b->opponentFaints = gBattleResults.opponentFaintCounter;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        u32 species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES);
        if (species != SPECIES_NONE && !GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG))
        {
            b->playerSpecies[b->numPlayer] = species;
            b->playerLevel[b->numPlayer] = GetMonData(&gPlayerParty[i], MON_DATA_LEVEL);
            b->playerHpBefore[b->numPlayer] = sHpBefore[i];
            b->playerHp[b->numPlayer] = GetMonData(&gPlayerParty[i], MON_DATA_HP);
            b->playerMaxHp[b->numPlayer] = GetMonData(&gPlayerParty[i], MON_DATA_MAX_HP);
            b->numPlayer++;
        }
        if (sOpponentSpecies[i] != SPECIES_NONE)
        {
            b->opponentSpecies[b->numOpponent] = sOpponentSpecies[i];
            b->opponentLevel[b->numOpponent] = sOpponentLevel[i];
            b->numOpponent++;
        }
    }
    gPlaytest.numBattles++;
}

static void DumpParty(void)
{
    u32 i, k;
    gPlaytest.partyCount = CalculatePlayerPartyCount();
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct PlaytestPartyMon *p = &gPlaytest.party[i];
        struct Pokemon *mon = &gPlayerParty[i];
        p->species = GetMonData(mon, MON_DATA_SPECIES);
        p->level = GetMonData(mon, MON_DATA_LEVEL);
        p->status = GetMonData(mon, MON_DATA_STATUS) != 0;
        p->hp = GetMonData(mon, MON_DATA_HP);
        p->maxHp = GetMonData(mon, MON_DATA_MAX_HP);
        for (k = 0; k < MAX_MON_MOVES; k++)
            p->moves[k] = GetMonData(mon, MON_DATA_MOVE1 + k);
        p->heldItem = GetMonData(mon, MON_DATA_HELD_ITEM);
        p->attack = GetMonData(mon, MON_DATA_ATK);
        p->defense = GetMonData(mon, MON_DATA_DEF);
        p->speed = GetMonData(mon, MON_DATA_SPEED);
        p->spAttack = GetMonData(mon, MON_DATA_SPATK);
        p->spDefense = GetMonData(mon, MON_DATA_SPDEF);
        p->ability = GetMonAbility(mon);
        p->ball = GetMonData(mon, MON_DATA_POKEBALL);
        p->friendship = GetMonData(mon, MON_DATA_FRIENDSHIP);
    }
}

// How much a move is worth to the autopilot: damage moves by power (same-type ones more), one
// status move kept at a middling value
static u32 MoveScore(struct Pokemon *mon, u32 move, bool32 haveStatus)
{
    u32 species = GetMonData(mon, MON_DATA_SPECIES), power, acc;
    if (move == MOVE_NONE)
        return 0;
    power = GetMovePower(move);
    if (power <= 1)
        return haveStatus ? 15 : 45;
    power *= GetMoveStrikeCount(move) > 1 ? GetMoveStrikeCount(move) : 1;
    acc = GetMoveAccuracy(move);
    if (acc == 0 || acc > 100)
        acc = 100;
    power = power * acc / 100;
    if (GetMoveType(move) == GetSpeciesType(species, 0) || GetMoveType(move) == GetSpeciesType(species, 1))
        power = power * 3 / 2;
    return power;
}

// Learning a move with four already: which to forget (the weakest, if the new one's better), or
// MAX_MON_MOVES to keep them all
u32 Playtest_MoveToForget(struct Pokemon *mon, u32 newMove)
{
    u32 i, worst = MAX_MON_MOVES, worstScore = 0xFFFF, newScore, statusCount = 0;
    for (i = 0; i < MAX_MON_MOVES; i++)
        if (GetMovePower(GetMonData(mon, MON_DATA_MOVE1 + i)) <= 1)
            statusCount++;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        u32 move = GetMonData(mon, MON_DATA_MOVE1 + i), score;
        if (CannotForgetMove(move))
            continue;
        score = MoveScore(mon, move, statusCount > 1);
        if (score < worstScore)
        {
            worstScore = score;
            worst = i;
        }
    }
    newScore = MoveScore(mon, newMove, statusCount > 0);
    return (worst < MAX_MON_MOVES && newScore > worstScore) ? worst : MAX_MON_MOVES;
}

// An assertf failed: recorded (and the game carries on, as a release build would), instead of the
// crash screen that waits for START
bool32 Playtest_Assert(const void *caller, const char *file, int line)
{
    u32 i;
    if (!Playtest_Active())
        return FALSE;
    i = gPlaytest.numAsserts++ % ARRAY_COUNT(gPlaytest.asserts);
    gPlaytest.asserts[i].file = file;
    gPlaytest.asserts[i].line = line;
    gPlaytest.asserts[i].battle = gPlaytest.numBattles;
    gPlaytest.asserts[i].caller = caller;
    return TRUE;
}

// Every overworld frame: runs the harness's command, if there is one
void Playtest_Update(void)
{
    u16 *a = gPlaytest.args;
    if (gPlaytest.magic != 0x59414C50)
        return;
    gPlaytest.frames++;
    gSaveBlock2Ptr->optionsBattleStyle = OPTIONS_BATTLE_STYLE_SET;
    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_FAST;
    gPlaytest.locked = ArePlayerFieldControlsLocked();
    gPlaytest.scriptRunning = ScriptContext_IsEnabled();
    if (gPlaytest.command == PLAYTEST_NONE)
        return;
    gPlaytest.result = 0;
    switch (gPlaytest.command)
    {
    case PLAYTEST_GIVE_MON:
        gPlaytest.result = ScriptGiveMon(a[0], a[1], ITEM_NONE);
        break;
    case PLAYTEST_HEAL:
        HealPlayerParty();
        break;
    case PLAYTEST_SWAP:
        if (a[0] < PARTY_SIZE && a[1] < PARTY_SIZE && a[0] != a[1])
        {
            struct Pokemon tmp = gPlayerParty[a[0]];
            gPlayerParty[a[0]] = gPlayerParty[a[1]];
            gPlayerParty[a[1]] = tmp;
        }
        break;
    case PLAYTEST_GIVE_ITEM:
        gPlaytest.result = AddBagItem(a[0], a[1]);
        break;
    case PLAYTEST_SET_LEVEL:
        if (a[0] < PARTY_SIZE && a[1] >= 1 && a[1] <= MAX_LEVEL)
        {
            struct Pokemon *mon = &gPlayerParty[a[0]];
            u32 exp = gExperienceTables[gSpeciesInfo[GetMonData(mon, MON_DATA_SPECIES)].growthRate][a[1]];
            SetMonData(mon, MON_DATA_EXP, &exp);
            CalculateMonStats(mon);
            GiveMonInitialMoveset(mon);     // (the moves it would know by now)
        }
        break;
    case PLAYTEST_TRAINER_BEATEN:
        gPlaytest.result = HasTrainerBeenFought(a[0]);
        break;
    case PLAYTEST_FLAG:
        gPlaytest.result = FlagGet(a[0]);
        break;
    case PLAYTEST_SET_FLAG:
        if (a[1])
            FlagSet(a[0]);
        else
            FlagClear(a[0]);
        break;
    case PLAYTEST_VAR:
        gPlaytest.result = VarGet(a[0]);
        break;
    case PLAYTEST_SET_VAR:
        VarSet(a[0], a[1]);
        break;
    case PLAYTEST_TRAIN_MON:
        if (a[0] < PARTY_SIZE)
        {
            static const u8 sSpreads[][6] =   // HP, Atk, Def, Spe, SpA, SpD (MON_DATA_*_EV order)
            {
                [PLAYTEST_EVS_ATK_SPE] = {4, 252, 0, 252, 0, 0},
                [PLAYTEST_EVS_SPA_SPE] = {4, 0, 0, 252, 252, 0},
                [PLAYTEST_EVS_HP_ATK]  = {252, 252, 4, 0, 0, 0},
                [PLAYTEST_EVS_HP_SPA]  = {252, 0, 4, 0, 252, 0},
                [PLAYTEST_EVS_HP_DEF]  = {252, 0, 252, 0, 0, 4},
                [PLAYTEST_EVS_HP_SPD]  = {252, 0, 4, 0, 0, 252},
            };
            struct Pokemon *mon = &gPlayerParty[a[0]];
            u32 i, iv = MAX_PER_STAT_IVS, nature = a[1], item = a[2];
            const u8 *evs = sSpreads[min(a[3], ARRAY_COUNT(sSpreads) - 1)];
            for (i = 0; i < NUM_STATS; i++)
            {
                u8 ev = evs[i];
                SetMonData(mon, MON_DATA_HP_IV + i, &iv);
                SetMonData(mon, MON_DATA_HP_EV + i, &ev);
            }
            if (nature < NUM_NATURES)
                SetMonData(mon, MON_DATA_HIDDEN_NATURE, &nature);
            SetMonData(mon, MON_DATA_HELD_ITEM, &item);
            CalculateMonStats(mon);
        }
        break;
    case PLAYTEST_SET_MOVE:
        if (a[0] < PARTY_SIZE && a[1] < MAX_MON_MOVES && a[2] < MOVES_COUNT)
            SetMonMoveSlot(&gPlayerParty[a[0]], a[2], a[1]);
        break;
    case PLAYTEST_TERA:
        if (a[1] < PARTY_SIZE)
        {
            if (a[0] == 0)
                gPlaytest.result = sOpponentTera[a[1]];   // (as the battle started)
            else if (a[0] == 1)
                gPlaytest.result = GymTera_PlayerBattleType(&gPlayerParty[a[1]]);
            else
                gPlaytest.result = GymTera_FreeType(&gPlayerParty[a[1]]);
        }
        break;
    case PLAYTEST_PLACE_DECOR:
        GymDecor_PutAllAway();   // (room for it)
        if (a[0] != 0)
        {
            s32 x, y;
            if (GymDecor_CountOwned(a[0]) == 0)
                GymDecor_SetOwned(a[0], 1);
            for (y = 0; y < 32 && !gPlaytest.result; y++)
                for (x = 0; x < 32 && !gPlaytest.result; x++)
                    gPlaytest.result = GymDecor_Place(a[0], 0, x, y);
        }
        break;
    case PLAYTEST_RELEASE:
        if (a[0] < PARTY_SIZE)
        {
            ZeroMonData(&gPlayerParty[a[0]]);
            CompactPartySlots();
            CalculatePlayerPartyCount();
        }
        break;
    }
    DumpParty();
    gPlaytest.command = PLAYTEST_NONE;
}

#else

void Playtest_BattleStart(void) {}
void Playtest_BattleEnd(void) {}
void Playtest_FirstTurn(void) {}
void Playtest_Update(void) {}
bool32 Playtest_AutoBattle(void) { return FALSE; }
bool32 Playtest_Active(void) { return FALSE; }
u32 Playtest_BestMove(u32 battler) { return 0; }
u32 Playtest_ChooseMove(u32 battler, u32 *target) { return 0; }
u32 Playtest_ChooseSwitchIn(u32 battler) { return PARTY_SIZE; }
bool32 Playtest_Assert(const void *caller, const char *file, int line) { return FALSE; }
u32 Playtest_MoveToForget(struct Pokemon *mon, u32 newMove) { return MAX_MON_MOVES; }

#endif
