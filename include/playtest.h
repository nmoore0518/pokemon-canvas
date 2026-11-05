#ifndef GUARD_PLAYTEST_H
#define GUARD_PLAYTEST_H

// Pokémon Canvas: hooks for automated playtests in an emulator (tools/gym_hack/playtest/). Only
// built with `make firered PLAYTEST=1`; in a normal build these do nothing and take no space.
#ifndef PLAYTEST
#define PLAYTEST 0
#endif

#define PLAYTEST_LOG_SIZE 64

// One battle: who, how it went, and both parties
struct PlaytestBattle
{
    u16 trainer;            // (TRAINER_NONE for a wild battle)
    u8 outcome;             // B_OUTCOME_*
    u8 turns;
    u8 playerFaints;
    u8 opponentFaints;
    u8 numPlayer;
    u8 numOpponent;
    u16 playerSpecies[PARTY_SIZE];
    u8 playerLevel[PARTY_SIZE];
    u16 playerHpBefore[PARTY_SIZE];
    u16 playerHp[PARTY_SIZE];
    u16 playerMaxHp[PARTY_SIZE];
    u16 opponentSpecies[PARTY_SIZE];
    u8 opponentLevel[PARTY_SIZE];
};

struct PlaytestPartyMon
{
    u16 species;
    u8 level;
    u8 status;
    u16 hp;
    u16 maxHp;
    u16 moves[MAX_MON_MOVES];
    u16 heldItem;
    u16 attack, defense, speed, spAttack, spDefense;
    u16 ability;            // (Pokémon Canvas: for the basement machines' checks)
    u8 ball;
    u8 friendship;
};

// The harness writes a command (and its arguments), the game runs it on its next overworld
// frame and clears `command`
enum
{
    PLAYTEST_NONE,
    PLAYTEST_GIVE_MON,      // args: species, level
    PLAYTEST_HEAL,
    PLAYTEST_SWAP,          // args: party slots a, b
    PLAYTEST_DUMP_PARTY,    // -> party[]
    PLAYTEST_GIVE_ITEM,     // args: item, count
    PLAYTEST_SET_LEVEL,     // args: party slot, level (moves stay)
    PLAYTEST_RELEASE,       // args: party slot
    PLAYTEST_TRAINER_BEATEN,// args: trainer -> result
    PLAYTEST_FLAG,          // args: flag -> result (FlagGet)
    PLAYTEST_SET_FLAG,      // args: flag, on
    PLAYTEST_VAR,           // args: var -> result
    PLAYTEST_SET_VAR,       // args: var, value
    PLAYTEST_TRAIN_MON,     // args: party slot, nature, held item, EV spread (PLAYTEST_EVS_*): what the
                            //       training items do (Gold Bottle Cap, Multivitamin, Cilantro)
    PLAYTEST_SET_MOVE,      // args: party slot, move slot (0-3), move (its full PP)
    PLAYTEST_PLACE_DECOR,   // args: gym decoration (0: none): puts everything away, then places it -> result: placed
                            //       (args[1], args[2]: where)
    PLAYTEST_TERA,          // args: which (0: an opponent's stored Tera type, 1: the player's in the last battle,
                            //       2: the player's free one, 3: its types now, first | second << 8), party slot -> result: the type
    PLAYTEST_SET_SHINY,     // args: party slot
};

// EV spreads for PLAYTEST_TRAIN_MON (252 / 252 / 4)
enum { PLAYTEST_EVS_ATK_SPE, PLAYTEST_EVS_SPA_SPE, PLAYTEST_EVS_HP_ATK, PLAYTEST_EVS_HP_SPA, PLAYTEST_EVS_HP_DEF, PLAYTEST_EVS_HP_SPD };

struct Playtest
{
    u32 magic;              // 'PLAY'
    u32 frames;             // overworld frames so far
    u8 autoBattle;          // the AI battles for the player
    u8 command;
    u8 locked;              // the player's controls are locked (a script, a message...)
    u8 scriptRunning;
    u16 args[4];
    u16 result;
    u16 numBattles;         // (all of them; the log keeps the last PLAYTEST_LOG_SIZE)
    struct PlaytestBattle log[PLAYTEST_LOG_SIZE];
    struct PlaytestPartyMon party[PARTY_SIZE];
    u8 partyCount;
    u8 inBattle;
    u16 numAsserts;
    struct
    {
        const char *file;
        u16 line;
        u16 battle;         // (numBattles when it happened)
        const void *caller;
    } asserts[16];
};

extern struct Playtest gPlaytest;

void Playtest_BattleStart(void);
void Playtest_BattleEnd(void);
void Playtest_FirstTurn(void);
void Playtest_Update(void);
bool32 Playtest_AutoBattle(void);
bool32 Playtest_Active(void);
u32 Playtest_BestMove(u32 battler);
u32 Playtest_ChooseMove(u32 battler, u32 *target);
u32 Playtest_ChooseSwitchIn(u32 battler);
bool32 Playtest_Assert(const void *caller, const char *file, int line);
u32 Playtest_MoveToForget(struct Pokemon *mon, u32 newMove);

#endif // GUARD_PLAYTEST_H
