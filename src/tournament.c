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

// Special, after a match won (in Bill's simulator too): hardly ever, a MOVE EXTRACTOR as a prize.
// VAR_RESULT = TRUE if so, VAR_0x8009 = the item
void Tourney_RollPrize(void)
{
    gSpecialVar_Result = FALSE;
    if (Random() % TOURNEY_EXTRACTOR_ODDS != 0)
        return;
    gSpecialVar_0x8009 = ITEM_MOVE_EXTRACTOR;
    gSpecialVar_Result = TRUE;
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
