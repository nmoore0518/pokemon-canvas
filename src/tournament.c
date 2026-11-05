// Pokémon Canvas: the tournament (scripts: data/scripts/gauntlet.inc; its board:
// src/tournament_board.c; its story and Bill: src/gauntlet.c).
//
// 128 entrants, single elimination, seven wins for the title. Each tournament draws 127 of the
// Gauntlet's challengers and the convention's trainers (no one twice: the convention has a few
// people in two halls), and the player. The seeding is rough: the entrants are sorted by tier
// (the legends and champions, then the strong, then everyone else, then the sidekicks), at
// random within a tier, and seeded the standard way (1 meets 128 first, and the top seeds can't
// meet until late); the player is unseeded, somewhere among the Gym Leaders.
//
// Only the seed is saved: the draw, the seeding and every match the player isn't in follow from
// it (a stronger tier wins more often, not always). Matches are decided a round at a time: the
// rest of a round is played out once the player has played theirs. Everyone's at level 100; the
// player's team is locked in when they enter (healed between rounds); a loss ends it. Prize money
// is the usual trainer battle's, by the opponent's class.
#include "global.h"
#include "drink_machine.h"
#include "tournament.h"
#include "data.h"
#include "event_data.h"
#include "gauntlet.h"
#include "gym_tera.h"
#include "gym_challenge.h"
#include "main.h"
#include "malloc.h"
#include "overworld.h"
#include "pokemon.h"
#include "random.h"
#include "berry_field.h"
#include "constants/items.h"
#include "script.h"
#include "string_util.h"
#include "title_card.h"
#include "constants/flags.h"
#include "constants/gym_hack.h"
#include "constants/rgb.h"
#include "constants/species.h"
#include "constants/vars.h"

#define TOURNEY_MAGIC 0x314E5254    // "TRN1"

EWRAM_DATA struct TourneySave gTourneySave = {0};

// How likely each tier is to win a match it isn't in (against another: its weight over both)
static const u8 sTierWeights[] = {9, 6, 4, 3};
#define PLAYER_TIER 2

static const u8 *const sRoundNames[TOURNEY_ROUNDS] =
{
    COMPOUND_STRING("ROUND OF 128"),
    COMPOUND_STRING("ROUND OF 64"),
    COMPOUND_STRING("ROUND OF 32"),
    COMPOUND_STRING("ROUND OF 16"),
    COMPOUND_STRING("QUARTERFINAL"),
    COMPOUND_STRING("SEMIFINAL"),
    COMPOUND_STRING("FINAL"),
};

// ---- The save ----
static u16 Checksum(void)
{
    const u8 *p = (const u8 *)&gTourneySave;
    u32 i, sum = 0;
    for (i = offsetof(struct TourneySave, titles); i < sizeof(gTourneySave); i++)
        sum = (sum * 31 + p[i]) & 0xFFFF;
    return sum;
}

void Tourney_ClearSave(void)
{
    memset(&gTourneySave, 0, sizeof(gTourneySave));
    gTourneySave.magic = TOURNEY_MAGIC;
}

// After loading: anything that isn't ours (or a save from before the tournament) is cleared
void Tourney_ValidateSave(void)
{
    if (gTourneySave.magic != TOURNEY_MAGIC || gTourneySave.checksum != Checksum())
        Tourney_ClearSave();
}

// (while playing: the checksum's only checked on loading)
static void EnsureSave(void)
{
    if (gTourneySave.magic != TOURNEY_MAGIC)
        Tourney_ClearSave();
}

// Before saving
void Tourney_SealSave(void)
{
    if (gTourneySave.magic != TOURNEY_MAGIC)
        Tourney_ClearSave();
    gTourneySave.checksum = Checksum();
}

// ---- The pool: the Gauntlet's challengers, then the convention's trainers ----
u32 Tourney_PoolSize(void)
{
    return Gauntlet_EntrantCount() + Convention_EntrantCount();
}

void Tourney_GetEntrant(u32 id, struct TourneyEntrant *out)
{
    if (id < Gauntlet_EntrantCount())
        Gauntlet_GetEntrant(id, out);
    else
        Convention_GetEntrant(id - Gauntlet_EntrantCount(), out);
}

// (the board's)
const u8 *Tourney_ShortName(u32 id)
{
    struct TourneyEntrant e;
    if (id == TOURNEY_PLAYER)
        return gSaveBlock2Ptr->playerName;
    if (id >= Tourney_PoolSize())
        return COMPOUND_STRING("???");
    Tourney_GetEntrant(id, &e);
    return e.battleName;
}

static u32 Tier(u32 id)
{
    struct TourneyEntrant e;
    if (id == TOURNEY_PLAYER)
        return PLAYER_TIER;
    Tourney_GetEntrant(id, &e);
    return e.tier < ARRAY_COUNT(sTierWeights) ? e.tier : ARRAY_COUNT(sTierWeights) - 1;
}

// ---- The draw ----
static u32 sRng;

static u32 NextRandom(void)
{
    sRng = sRng * 1103515245 + 24691;
    return sRng >> 16;
}

static u32 Mix(u32 x)
{
    x ^= x >> 16;
    x *= 0x7FEB352D;
    x ^= x >> 15;
    x *= 0x846CA68B;
    x ^= x >> 16;
    return x;
}

// The standard seeding: the bracket's slot -> its seed (0 the top), so that seed 1 meets 128 first
// and the top two can only meet in the final
static void SeedOrder(u8 *order)
{
    u32 size, i, n = 1;
    order[0] = 0;
    for (size = 2; size <= TOURNEY_SIZE; size *= 2)
    {
        for (i = n; i-- > 0;)
        {
            order[2 * i] = order[i];
            order[2 * i + 1] = size - 1 - order[i];
        }
        n = size;
    }
}

static bool32 NameTaken(const u8 *name, const u8 *const *names, u32 count)
{
    u32 i;
    for (i = 0; i < count; i++)
    {
        if (StringCompare(name, names[i]) == 0)
            return TRUE;
    }
    return FALSE;
}

// A tournament's first round: who's in which slot (TOURNEY_PLAYER: the player)
static void Draw(u32 seed, u8 *slots)
{
    u32 pool = Tourney_PoolSize(), i, j, n = 0, tier, k, start, count;
    u8 *order = Alloc(pool), entrants[TOURNEY_SIZE], picked[TOURNEY_SIZE], seeds[TOURNEY_SIZE];
    const u8 **names = Alloc(TOURNEY_SIZE * sizeof(*names));
    struct TourneyEntrant e;

    sRng = seed;
    for (i = 0; i < pool; i++)
        order[i] = i;
    for (i = pool - 1; i > 0; i--)
    {
        u8 t;
        j = NextRandom() % (i + 1);
        t = order[i], order[i] = order[j], order[j] = t;
    }
    for (i = 0; i < pool && n < TOURNEY_SIZE - 1; i++)
    {
        Tourney_GetEntrant(order[i], &e);
        if (NameTaken(e.name, names, n))
            continue;
        names[n] = e.name;
        picked[n++] = order[i];
    }
    Free(names);
    Free(order);

    // By tier (still at random within one), the player somewhere among their tier
    k = 0;
    for (tier = 0; tier < ARRAY_COUNT(sTierWeights); tier++)
    {
        start = k;
        for (i = 0; i < n; i++)
        {
            if (Tier(picked[i]) == tier)
                entrants[k++] = picked[i];
        }
        if (tier == PLAYER_TIER)
        {
            count = k - start;
            j = start + NextRandom() % (count + 1);
            for (i = k; i > j; i--)
                entrants[i] = entrants[i - 1];
            entrants[j] = TOURNEY_PLAYER;
            k++;
        }
    }
    SeedOrder(seeds);
    for (i = 0; i < TOURNEY_SIZE; i++)
        slots[i] = i < k ? entrants[seeds[i]] : TOURNEY_PENDING;
}

// ---- The bracket ----
// tree[0]: the first round's slots; tree[r + 1][m]: who won match m of round r (TOURNEY_PENDING: not
// played yet). The player's matches go by how they did; the others follow from the seed, once the
// player has played that round (or is out)
void Tourney_BuildTree(u8 tree[TOURNEY_ROUNDS + 1][TOURNEY_SIZE])
{
    const struct TourneyRun *run = &gTourneySave.run;
    u32 r, m, size = TOURNEY_SIZE;
    bool32 out = run->status != TOURNEY_ACTIVE;

    memset(tree, TOURNEY_PENDING, (TOURNEY_ROUNDS + 1) * TOURNEY_SIZE);
    if (run->status == TOURNEY_NONE)
        return;
    Draw(run->seed, tree[0]);
    for (r = 0; r < TOURNEY_ROUNDS; r++, size /= 2)
    {
        for (m = 0; m < size / 2; m++)
        {
            u32 a = tree[r][2 * m], b = tree[r][2 * m + 1], winner;
            if (a == TOURNEY_PENDING || b == TOURNEY_PENDING)
                continue;
            if (a == TOURNEY_PLAYER || b == TOURNEY_PLAYER)
            {
                u32 other = a == TOURNEY_PLAYER ? b : a;
                if (run->wins > r)
                    winner = TOURNEY_PLAYER;
                else if (out)
                    winner = other;     // (the match they lost, or forfeited)
                else
                    continue;
            }
            else
            {
                u32 wa, wb;
                if (!out && r >= run->wins)
                    continue;
                wa = sTierWeights[Tier(a)];
                wb = sTierWeights[Tier(b)];
                winner = Mix(run->seed ^ (r * 0x9E3779B9) ^ ((m + 1) * 0x85EBCA6B)) % (wa + wb) < wa ? a : b;
            }
            tree[r + 1][m] = winner;
        }
    }
}

u32 Tourney_PlayerSlot(const u8 tree[TOURNEY_ROUNDS + 1][TOURNEY_SIZE])
{
    u32 i;
    for (i = 0; i < TOURNEY_SIZE; i++)
    {
        if (tree[0][i] == TOURNEY_PLAYER)
            return i;
    }
    return 0;
}

static u8 (*AllocTree(void))[TOURNEY_SIZE]
{
    u8 (*tree)[TOURNEY_SIZE] = Alloc((TOURNEY_ROUNDS + 1) * TOURNEY_SIZE);
    Tourney_BuildTree(tree);
    return tree;
}

// The player's next opponent (or the last one: whom they lost to)
static u32 Opponent(void)
{
    u8 (*tree)[TOURNEY_SIZE] = AllocTree();
    u32 w = gTourneySave.run.wins < TOURNEY_ROUNDS ? gTourneySave.run.wins : TOURNEY_ROUNDS - 1;
    u32 opp = tree[w][(Tourney_PlayerSlot((const u8 (*)[TOURNEY_SIZE])tree) >> w) ^ 1];
    Free(tree);
    return opp;
}

static bool32 InSim(void)
{
    return VarGet(VAR_GAUNTLET_STATE) == GAUNTLET_SIMULATOR;
}

// ---- The team: locked in ----
static bool32 TeamMatches(void)
{
    const struct TourneyRun *run = &gTourneySave.run;
    u32 i, j, n = 0;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        u32 pers;
        if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG))
            continue;
        pers = GetMonData(mon, MON_DATA_PERSONALITY);
        for (j = 0; j < run->teamCount; j++)
        {
            if (run->team[j] == pers)
                break;
        }
        if (j == run->teamCount)
            return FALSE;
        n++;
    }
    return n == run->teamCount;
}

// ---- Specials ----
// VAR_RESULT = the run's status (TOURNEY_*); VAR_0x8005 = TRUE if it's from Bill's simulator
void Tourney_GetStatus(void)
{
    EnsureSave();
    gSpecialVar_Result = gTourneySave.run.status;
    if (gSpecialVar_Result == TOURNEY_ACTIVE && gTourneySave.run.sim != InSim())
        gSpecialVar_Result = TOURNEY_FORFEIT;   // (one from before Bill was beaten, say: it's over)
}

// A new tournament: the draw, and the team as it is now
void Tourney_Begin(void)
{
    struct TourneyRun *run = &gTourneySave.run;
    u32 i;
    EnsureSave();
    memset(run, 0, sizeof(*run));
    do
        run->seed = Random32();
    while (run->seed == 0);
    run->status = TOURNEY_ACTIVE;
    run->sim = InSim();
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG))
            continue;
        run->team[run->teamCount++] = GetMonData(mon, MON_DATA_PERSONALITY);
    }
}

// VAR_RESULT = whether the party is the team entered (any order)
void Tourney_CheckTeam(void)
{
    gSpecialVar_Result = TeamMatches();
}

// The player's next opponent set up (their team at level 100, lines, walking sprite in
// VAR_OBJ_GFX_ID_0). STR_VAR_1 = their name, STR_VAR_2 = the round, STR_VAR_3 = their title
static EWRAM_DATA u8 sOpponent = 0;

void Tourney_SetupNext(void)
{
    struct TourneyEntrant e;
    u32 round = gTourneySave.run.wins < TOURNEY_ROUNDS ? gTourneySave.run.wins : TOURNEY_ROUNDS - 1;
    sOpponent = Opponent();
    Tourney_GetEntrant(sOpponent, &e);
    GymChallenge_SetupGauntlet(e.trainerClass, e.pic, e.female, e.battleName, e.party, e.partySize,
                               e.intro, e.defeat, e.after, e.win);
    GymChallenge_SetChallengerLevel(MAX_LEVEL);
    GymChallenge_SetTeraMode(GYM_TERA_TEAM);   // (a free battle: the ace Terastallizes into the team's type)
    GymChallenge_SetFormat(Tourney_Format());
    VarSet(VAR_OBJ_GFX_ID_0, e.gfx);
    StringCopy(gStringVar1, e.name);
    StringCopy(gStringVar2, sRoundNames[round]);
    StringCopy(gStringVar3, e.title);
}

// Special (the script has faded to black, then waitstates): the opponent's title card
static EWRAM_DATA u8 sCardHeader[48] = {0};

void Tourney_ShowTitleCard(void)
{
    struct TourneyEntrant e;
    u32 round = gTourneySave.run.wins < TOURNEY_ROUNDS ? gTourneySave.run.wins : TOURNEY_ROUNDS - 1;
    u8 *end;
    Tourney_GetEntrant(sOpponent, &e);
    end = StringCopy(sCardHeader, InSim() ? COMPOUND_STRING("BILL'S SIMULATOR  ·  ") : COMPOUND_STRING("THE TOURNAMENT  ·  "));
    StringCopy(end, sRoundNames[round]);
    TitleCard_Show(e.pic, round == TOURNEY_ROUNDS - 1 ? RGB(27, 21, 6) : e.color, sCardHeader, e.name, e.title, e.region, e.games);
}

// The champion's team, in the tournament's Hall of Fame: the first title is kept for good, then the
// most recent
static void RecordTitle(u32 finalist)
{
    struct TourneyHallRecord *rec = NULL;
    u32 i, n = 0;
    if (gTourneySave.hall[0].title == 0)
    {
        rec = &gTourneySave.hall[0];
    }
    else
    {
        for (i = 1; i < TOURNEY_HALL_RECORDS; i++)
        {
            if (gTourneySave.hall[i].title == 0)
            {
                rec = &gTourneySave.hall[i];
                break;
            }
        }
        if (rec == NULL)
        {
            for (i = 1; i < TOURNEY_HALL_RECORDS - 1; i++)
                gTourneySave.hall[i] = gTourneySave.hall[i + 1];
            rec = &gTourneySave.hall[TOURNEY_HALL_RECORDS - 1];
        }
    }
    memset(rec, 0, sizeof(*rec));
    rec->title = gTourneySave.titles;
    rec->finalist = finalist;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        struct TourneyHallMon *dst;
        u8 nick[POKEMON_NAME_BUFFER_SIZE];
        if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG))
            continue;
        dst = &rec->mon[n++];
        dst->species = GetMonData(mon, MON_DATA_SPECIES);
        dst->personality = GetMonData(mon, MON_DATA_PERSONALITY);
        dst->isShiny = GetMonData(mon, MON_DATA_IS_SHINY);
        dst->lvl = GetMonData(mon, MON_DATA_LEVEL);
        GetMonData(mon, MON_DATA_NICKNAME, nick);
        memcpy(dst->nickname, nick, POKEMON_NAME_LENGTH);
    }
    rec->count = n;
}

// Special, after a win. VAR_RESULT = TRUE if that was the final (the title: the Hall of Fame, and
// Bill's next: the first time the real one, in the simulator his simulation, for a WITHDRAW). STR_VAR_1 = the next round
void Tourney_RecordWin(void)
{
    struct TourneyRun *run = &gTourneySave.run;
    struct TourneyEntrant e;
    Tourney_GetEntrant(sOpponent, &e);
    if (StringCompare(e.region, COMPOUND_STRING("KALOS")) == 0)
        FlagSet(FLAG_TOURNEY_BEAT_KALOS);
    if (StringCompare(e.name, COMPOUND_STRING("AZ")) == 0)
        FlagSet(FLAG_TOURNEY_BEAT_AZ);
    gSpecialVar_Result = FALSE;
    if (run->status != TOURNEY_ACTIVE)
        return;
    run->wins++;
    if (run->wins < TOURNEY_ROUNDS)
    {
        StringCopy(gStringVar1, sRoundNames[run->wins]);
        return;
    }
    run->status = TOURNEY_WON;
    if (gTourneySave.titles < 0xFFFF)
        gTourneySave.titles++;
    RecordTitle(sOpponent);
    if (InSim())
        run->status = TOURNEY_BILL;     // (the WITHDRAW is for beating Bill after it)
    else if (VarGet(VAR_GAUNTLET_STATE) == GAUNTLET_ACTIVE)
        VarSet(VAR_GAUNTLET_STATE, GAUNTLET_ALL_BEATEN);
    gSpecialVar_Result = TRUE;
}

// ---- A prize for every match won (in Bill's simulator too) ----
// One roll on this table: weights out of TOURNEY_DROP_TOTAL, roughly by what the item costs (a cheap
// one often, an expensive one seldom). A title run is TOURNEY_ROUNDS wins, so a MOVE EXTRACTOR at
// 312/10000 a win is about a 20% chance in a run, a LIMITED EDITION SODA at 149 about 10% (once the
// player has the Drink Machine: before that, the roll skips it). Some entries are one of a set:
#define DROP_TYPE_BERRY   0xFFF0   // (any of the 18: GetTeraBerry)
#define DROP_VITAMIN      0xFFF1
#define DROP_MINT         0xFFF2
#define DROP_FUEL         0xFFF3   // (Dad's synthesizer's fuels)
#define DROP_UPGRADE_1    0xFFF4   // (the Lucky Egg's and Amulet Coin's materials: src/item_upgrades.c)
#define DROP_UPGRADE_2    0xFFF5
#define DROP_UPGRADE_3    0xFFF6

struct TourneyDrop { u16 item; u16 weight; bool8 rare; };
static const struct TourneyDrop sTourneyDrops[] =
{
    {DROP_TYPE_BERRY,         2300},
    {ITEM_PEARL,               700},
    {ITEM_NUGGET,              700},
    {ITEM_STARDUST,            600},
    {DROP_VITAMIN,             600},
    {ITEM_RARE_CANDY,          500},
    {DROP_FUEL,                500},
    {ITEM_BIG_PEARL,           404},
    {ITEM_PP_UP,               400},
    {DROP_MINT,                300},
    {ITEM_FULL_RESTORE,        300},
    {ITEM_MAX_REVIVE,          300},
    {ITEM_STAR_PIECE,          300},
    {ITEM_MOVE_EXTRACTOR,      312, TRUE},
    {ITEM_MAX_ELIXIR,          250},
    {ITEM_PP_MAX,              250},
    {DROP_UPGRADE_1,           250},
    {ITEM_COMET_SHARD,         150},
    {ITEM_LIMITED_EDITION_SODA, 149, TRUE},
    {ITEM_BIG_NUGGET,          120, TRUE},
    {ITEM_CILANTRO,            120, TRUE},
    {ITEM_BOTTLE_CAP,          110, TRUE},
    {ITEM_INCENSE,              90, TRUE},
    {DROP_UPGRADE_2,            80, TRUE},
    {ITEM_ABILITY_CAPSULE,      60, TRUE},
    {ITEM_MULTIVITAMIN,         60, TRUE},
    {ITEM_GOLD_BOTTLE_CAP,      40, TRUE},
    {DROP_UPGRADE_3,            30, TRUE},
    {ITEM_ABILITY_PATCH,        25, TRUE},
};

static const u16 sDropVitamins[] = {ITEM_HP_UP, ITEM_PROTEIN, ITEM_IRON, ITEM_CALCIUM, ITEM_ZINC, ITEM_CARBOS};
static const u16 sDropMints[] = {ITEM_ADAMANT_MINT, ITEM_JOLLY_MINT, ITEM_MODEST_MINT, ITEM_TIMID_MINT,
                                 ITEM_BOLD_MINT, ITEM_IMPISH_MINT, ITEM_CALM_MINT, ITEM_CAREFUL_MINT};

static bool32 DropAvailable(const struct TourneyDrop *d)
{
    return d->item != ITEM_LIMITED_EDITION_SODA || DrinkMachine_TournamentSodaUnlocked();
}

// The weights there are to roll on now
u32 Tourney_DropTotal(void)
{
    u32 i, total = 0;
    for (i = 0; i < ARRAY_COUNT(sTourneyDrops); i++)
        if (DropAvailable(&sTourneyDrops[i]))
            total += sTourneyDrops[i].weight;
    return total;
}

// The prize for a roll (0 to Tourney_DropTotal() - 1); *rare: whether it's a rare one
u32 Tourney_DropForRoll(u32 roll, bool32 *rare)
{
    u32 i;
    u16 item = ITEM_NUGGET;
    *rare = FALSE;
    for (i = 0; i < ARRAY_COUNT(sTourneyDrops); i++)
    {
        if (!DropAvailable(&sTourneyDrops[i]))
            continue;
        if (roll < sTourneyDrops[i].weight)
        {
            item = sTourneyDrops[i].item;
            *rare = sTourneyDrops[i].rare;
            break;
        }
        roll -= sTourneyDrops[i].weight;
    }
    switch (item)
    {
    case DROP_TYPE_BERRY:
    {
        u32 type = TYPE_NORMAL + Random() % 18;     // (NORMAL..FAIRY, but not MYSTERY)
        return GetTeraBerry(type >= TYPE_MYSTERY ? type + 1 : type);
    }
    case DROP_VITAMIN:    return sDropVitamins[Random() % ARRAY_COUNT(sDropVitamins)];
    case DROP_MINT:       return sDropMints[Random() % ARRAY_COUNT(sDropMints)];
    case DROP_FUEL:       return Random() % 2 ? ITEM_STRONG_ACID : ITEM_APPLE_JUICE;
    case DROP_UPGRADE_1:  return Random() % 2 ? ITEM_LUCKY_CLOVER : ITEM_COPPER_COIN;
    case DROP_UPGRADE_2:  return Random() % 2 ? ITEM_FOUR_LEAF_CLOVER : ITEM_SILVER_COIN;
    case DROP_UPGRADE_3:  return Random() % 2 ? ITEM_GOLDEN_CLOVER : ITEM_GOLD_COIN;
    }
    return item;
}

// Special, after a match won: its prize. VAR_0x8009 = the item; VAR_RESULT = TOURNEY_PRIZE_RARE for a
// rare one, else TOURNEY_PRIZE_COMMON
void Tourney_RollPrize(void)
{
    bool32 rare;
    gSpecialVar_0x8009 = Tourney_DropForRoll(Random() % Tourney_DropTotal(), &rare);
    gSpecialVar_Result = rare ? TOURNEY_PRIZE_RARE : TOURNEY_PRIZE_COMMON;
}

// Special: Bill beaten in the simulator, after a simulated title: a WITHDRAW
void Tourney_SimBillWon(void)
{
    if (gTourneySave.run.status != TOURNEY_BILL)
        return;
    gTourneySave.run.status = TOURNEY_WON;
    VarSet(VAR_GAUNTLET_WITHDRAWS, VarGet(VAR_GAUNTLET_WITHDRAWS) + 1);
}

// Special: STR_VAR_1 = who won the tournament (once it's over), STR_VAR_2 = the round the player
// went out in
void Tourney_BufferChampion(void)
{
    u8 (*tree)[TOURNEY_SIZE] = AllocTree();
    u32 champ = tree[TOURNEY_ROUNDS][0];
    u32 round = gTourneySave.run.wins < TOURNEY_ROUNDS ? gTourneySave.run.wins : TOURNEY_ROUNDS - 1;
    Free(tree);
    if (champ == TOURNEY_PLAYER)
        StringCopy(gStringVar1, gSaveBlock2Ptr->playerName);
    else if (champ == TOURNEY_PENDING)
        StringCopy(gStringVar1, COMPOUND_STRING("???"));
    else
    {
        struct TourneyEntrant e;
        Tourney_GetEntrant(champ, &e);
        StringCopy(gStringVar1, e.name);
    }
    StringCopy(gStringVar2, sRoundNames[round]);
}

// Special, after a loss: the tournament's over for the player (the rest plays out)
void Tourney_RecordLoss(void)
{
    if (gTourneySave.run.status == TOURNEY_ACTIVE)
        gTourneySave.run.status = TOURNEY_LOST;
    Tourney_BufferChampion();
}

// Special: the player pulls out (their next match counts as lost)
void Tourney_Forfeit(void)
{
    if (gTourneySave.run.status == TOURNEY_ACTIVE)
        gTourneySave.run.status = TOURNEY_FORFEIT;
    Tourney_BufferChampion();
}

// Special (then waitstate): the board
void Tourney_ShowBoard(void)
{
    TourneyBoard_Open(CB2_ReturnToFieldContinueScript);
}

u32 Tourney_HallCount(void)
{
    u32 i, n = 0;
    EnsureSave();
    for (i = 0; i < TOURNEY_HALL_RECORDS; i++)
        n += gTourneySave.hall[i].title != 0;
    return n;
}

// (src/event_mons.c) The Bond Phenomenon's trainer, and AZ: a Kalos trainer (AZ) beaten in a
// tournament, or a tournament won
bool32 Tourney_KalosBeaten(void)
{
    return FlagGet(FLAG_TOURNEY_BEAT_KALOS) || VarGet(VAR_GAUNTLET_STATE) >= GAUNTLET_ALL_BEATEN;
}

bool32 Tourney_AzBeaten(void)
{
    return FlagGet(FLAG_TOURNEY_BEAT_AZ) || VarGet(VAR_GAUNTLET_STATE) >= GAUNTLET_ALL_BEATEN;
}

// Pokémon Canvas: the tournament's battles are doubles; Bill's simulator asks which (FLAG_TOURNEY_SIM_DOUBLES)
u32 Tourney_Format(void)
{
    if (VarGet(VAR_GAUNTLET_STATE) != GAUNTLET_SIMULATOR || FlagGet(FLAG_TOURNEY_SIM_DOUBLES))
        return GYM_FORMAT_DOUBLES;
    return GYM_FORMAT_SINGLES;
}

// Special (the simulator, entering): VAR_RESULT = 0 singles, 1 doubles
void Tourney_SetSimFormat(void)
{
    if (gSpecialVar_Result == 1)
        FlagSet(FLAG_TOURNEY_SIM_DOUBLES);
    else
        FlagClear(FLAG_TOURNEY_SIM_DOUBLES);
}
