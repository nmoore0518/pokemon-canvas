#ifndef GUARD_TOURNAMENT_H
#define GUARD_TOURNAMENT_H

#include "constants/gym_hack.h"

// Pokémon Canvas: the tournament (src/tournament.c; its board: src/tournament_board.c). 128
// entrants, single elimination: 127 drawn from the Gauntlet's challengers and the convention's
// trainers, and the player.
#define TOURNEY_SIZE        128
#define TOURNEY_PLAYER      0xFF    // (in the bracket: the player)
#define TOURNEY_PENDING     0xFE    // (a match not played yet)

// One possible entrant (a Gauntlet challenger, or a convention trainer)
struct TourneyEntrant
{
    const u8 *name;         // (the title card's)
    const u8 *battleName;   // (in battle, and on the board: 10 letters at most)
    const u8 *title;
    const u8 *region;
    const u8 *games;
    const u8 *intro;
    const u8 *defeat;
    const u8 *after;
    const u8 *win;
    const struct TrainerMon *party;
    u16 pic;
    u16 gfx;
    u16 color;              // (the title card's band)
    u8 trainerClass;
    u8 female;
    u8 tier;                // (0 the strongest .. 3)
    u8 partySize;
};

// (src/gauntlet.c, src/gym_convention.c)
u32 Gauntlet_EntrantCount(void);
void Gauntlet_GetEntrant(u32 index, struct TourneyEntrant *out);
u32 Convention_EntrantCount(void);
void Convention_GetEntrant(u32 index, struct TourneyEntrant *out);

// The run under way (or the last one), and the tournament's Hall of Fame. Kept after the PC's
// storage in flash sector 31 (src/save.c)
struct TourneyRun
{
    u32 seed;               // (the draw, the seeding and every other match follow from it)
    u8 status;              // TOURNEY_*
    u8 wins;
    u8 sim;                 // (in Bill's simulator)
    u8 teamCount;
    u32 team[PARTY_SIZE];   // (the personalities of the team entered: it's locked in)
};

struct TourneyHallMon
{
    u32 personality;
    u16 species:15;
    u16 isShiny:1;
    u8 lvl;
    u8 nickname[POKEMON_NAME_LENGTH];
};

struct TourneyHallRecord
{
    u16 title;              // (which title this was: 1 for the first; 0 for an empty record)
    u8 count;
    u8 finalist;            // (whom the final was against: a pool index)
    struct TourneyHallMon mon[PARTY_SIZE];
};

// The first title, kept for good, and the most recent ones
#define TOURNEY_HALL_RECORDS 4

struct TourneySave
{
    u32 magic;
    u16 checksum;
    u16 titles;
    struct TourneyRun run;
    struct TourneyHallRecord hall[TOURNEY_HALL_RECORDS];
};

extern struct TourneySave gTourneySave;
u32 Tourney_Format(void);
void Tourney_SetSimFormat(void);

void Tourney_ClearSave(void);
void Tourney_ValidateSave(void);
void Tourney_SealSave(void);

u32 Tourney_PoolSize(void);
void Tourney_GetEntrant(u32 id, struct TourneyEntrant *out);
const u8 *Tourney_ShortName(u32 id);
void Tourney_BuildTree(u8 tree[TOURNEY_ROUNDS + 1][TOURNEY_SIZE]);
u32 Tourney_PlayerSlot(const u8 tree[TOURNEY_ROUNDS + 1][TOURNEY_SIZE]);
u32 Tourney_HallCount(void);

// Specials
void Tourney_Begin(void);
void Tourney_GetStatus(void);
void Tourney_CheckTeam(void);
void Tourney_SetupNext(void);
void Tourney_ShowTitleCard(void);
void Tourney_RecordWin(void);
void Tourney_RecordLoss(void);
void Tourney_SimBillWon(void);
void Tourney_RollPrize(void);
u32 Tourney_DropTotal(void);
u32 Tourney_DropForRoll(u32 roll, bool32 *rare);
void Tourney_Forfeit(void);
void Tourney_BufferChampion(void);
void Tourney_ShowBoard(void);
void AccessTourneyHallPC(void);

// (src/event_mons.c)
bool32 Tourney_KalosBeaten(void);
bool32 Tourney_AzBeaten(void);

// (src/tournament_board.c)
void TourneyBoard_Open(void (*callback)(void));

#endif // GUARD_TOURNAMENT_H
