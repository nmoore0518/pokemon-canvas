// Gym Trainer hack: the Move Extractor.
//
// Used from the bag in battle (data/battle_scripts_2.s: BattleScript_ItemMoveExtractor), it takes
// the last move used by a foe and lets the user learn it. The learned move takes the learner's
// primary type (a new STAB move): the move slot is marked in the Pokémon's data
// (MON_DATA_STOLEN_MOVES), which GetDynamicMoveType (src/battle_main.c) and the move type
// displays check. Nothing else about the move changes. The foe can't use that move again in
// the battle. The item is used up only when a move is learned.
//
// The player gets three in all, from a Team Rocket grunt (data/scripts/move_extractor.inc).
#include "global.h"
#include "move_extractor.h"
#include "battle.h"
#include "battle_message.h"
#include "battle_scripts.h"
#include "battle_util.h"
#include "battle_setup.h"
#include "event_data.h"
#include "item.h"
#include "pokemon.h"
#include "string_util.h"
#include "test_runner.h"
#include "constants/battle.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/vars.h"
#include "constants/flags.h"
#include "constants/opponents.h"

// The battle script command layout (as in src/battle_script_commands.c)
#define CMD_ARGS(...) const struct __attribute__((packed)) { u8 opcode; RECURSIVELY(R_FOR_EACH(APPEND_SEMICOLON, __VA_ARGS__)) const u8 nextInstr[0]; } *const cmd UNUSED = (const void *)gBattlescriptCurrInstr
#define NATIVE_ARGS(...) CMD_ARGS(void (*func)(void), ##__VA_ARGS__)

static EWRAM_DATA u8 sExtractFrom;   // the foe it's taken from

static bool32 IsExtractable(enum Move move)
{
    return move != MOVE_NONE && move != MOVE_UNAVAILABLE && move != MOVE_STRUGGLE;
}

// The foe whose last move can be taken (B_POSITION_OPPONENT_LEFT first), or MAX_BATTLERS_COUNT
static u32 FindFoe(void)
{
    static const u8 positions[] = {B_POSITION_OPPONENT_LEFT, B_POSITION_OPPONENT_RIGHT};
    u32 i;
    for (i = 0; i < ARRAY_COUNT(positions); i++)
    {
        u32 battler = GetBattlerAtPosition(positions[i]);
        if (battler < gBattlersCount && IsBattlerAlive(battler) && IsExtractable(gLastMoves[battler]))
            return battler;
    }
    return MAX_BATTLERS_COUNT;
}

static struct Pokemon *UserMon(u32 battler)
{
    return &gParties[B_TRAINER_PLAYER][gBattlerPartyIndexes[battler]];
}

static bool32 KnowsMove(struct Pokemon *mon, enum Move move)
{
    u32 i;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (GetMonData(mon, MON_DATA_MOVE1 + i) == move)
            return TRUE;
    }
    return FALSE;
}

// For the bag: why it can't be used now (NULL if it can). battler = the player's battler choosing.
const u8 *MoveExtractor_CantUseReason(u32 battler)
{
    u32 foe = FindFoe();
    if (foe == MAX_BATTLERS_COUNT)
        return COMPOUND_STRING("There's no move to extract yet.\nWait for the foe to use one!\p");
    if (gBattleMons[battler].volatiles.transformed)
        return COMPOUND_STRING("It won't work on a transformed\nPOKéMON.\p");
    if (KnowsMove(UserMon(battler), gLastMoves[foe]))
        return COMPOUND_STRING("Your POKéMON already knows\nthat move.\p");
    return NULL;
}

// The move a foe's last used, as the bag would extract it (for tests)
enum Move MoveExtractor_GetMove(void)
{
    u32 foe = FindFoe();
    return foe == MAX_BATTLERS_COUNT ? MOVE_NONE : gLastMoves[foe];
}

// callnative: picks the move. B_BUFF1 = the user, B_BUFF2 = the move, target = the foe.
// Jumps to nothingPtr if there's nothing to take. (Automated tests can't answer the "Proceed?"
// box, so they skip to confirmedPtr.)
void BS_MoveExtractorStart(void)
{
    NATIVE_ARGS(const u8 *nothingPtr, const u8 *confirmedPtr);
    u32 battler = gBattlerAttacker;
    u32 foe = FindFoe();

    if (foe == MAX_BATTLERS_COUNT || KnowsMove(UserMon(battler), gLastMoves[foe]))
    {
        gBattlescriptCurrInstr = cmd->nothingPtr;
        return;
    }
    sExtractFrom = foe;
    gBattlerTarget = foe;
    gMoveToLearn = gLastMoves[foe];
    gBattleStruct->expGetterMonId = gBattlerPartyIndexes[battler];
    PREPARE_MON_NICK_BUFFER(gBattleTextBuff1, battler, gBattlerPartyIndexes[battler]);
    PREPARE_MOVE_BUFFER(gBattleTextBuff2, gMoveToLearn);
    if (gTestRunnerEnabled)
        gBattlescriptCurrInstr = cmd->confirmedPtr;
    else
        gBattlescriptCurrInstr = cmd->nextInstr;
}

// callnative: learns it straight away if there's a free slot (jumps to the address after it);
// otherwise goes on to ask which move to forget
void BS_MoveExtractorTryGive(void)
{
    NATIVE_ARGS(const u8 *learnedPtr);
    u32 battler = gBattlerAttacker;
    struct Pokemon *mon = UserMon(battler);

    if (GiveMoveToMon(mon, gMoveToLearn) != MON_HAS_MAX_MOVES)
    {
        if (!gBattleMons[battler].volatiles.transformed)
            GiveMoveToBattleMon(&gBattleMons[battler], gMoveToLearn);
        gBattlescriptCurrInstr = cmd->learnedPtr;
        return;
    }
    gBattlescriptCurrInstr = cmd->nextInstr;
}

// Marks the slot holding the move as extracted (so it takes the Pokémon's primary type)
bool32 MoveExtractor_MarkLearned(struct Pokemon *mon, enum Move move)
{
    u32 i;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (GetMonData(mon, MON_DATA_MOVE1 + i) == move)
        {
            u8 stolen = GetMonData(mon, MON_DATA_STOLEN_MOVES) | (1 << i);
            SetMonData(mon, MON_DATA_STOLEN_MOVES, &stolen);
            return TRUE;
        }
    }
    return FALSE;
}

// Takes the move away from the foe for the rest of the battle (no PP left)
void MoveExtractor_TakeFromFoe(u32 foe, enum Move move)
{
    u32 i;
    struct Pokemon *foeMon = GetBattlerMon(foe);
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (gBattleMons[foe].moves[i] == move)
            gBattleMons[foe].pp[i] = 0;
        if (foeMon != NULL && GetMonData(foeMon, MON_DATA_MOVE1 + i) == move)
        {
            u8 zero = 0;
            SetMonData(foeMon, MON_DATA_PP1 + i, &zero);
        }
    }
}

// callnative: it's learned. Marks it, uses up the item, and takes the move from the foe.
// B_BUFF3 = its new type's name.
void BS_MoveExtractorFinish(void)
{
    NATIVE_ARGS();
    u32 battler = gBattlerAttacker;
    struct Pokemon *mon = UserMon(battler);

    MoveExtractor_MarkLearned(mon, gMoveToLearn);
    RemoveBagItem(ITEM_MOVE_EXTRACTOR, 1);
    MoveExtractor_TakeFromFoe(sExtractFrom, gMoveToLearn);
    gBattlerTarget = sExtractFrom;
    PREPARE_MON_NICK_BUFFER(gBattleTextBuff1, battler, gBattlerPartyIndexes[battler]);
    StringCopy(gBattleTextBuff3, gTypesInfo[GetStolenMoveType(mon)].name);
    gBattlescriptCurrInstr = cmd->nextInstr;
}

// Special: VAR_0x8004 = which of the grunt's visits (2: Saffron City, 3: Viridian City) ->
// VAR_RESULT = whether he shows up now
void MoveExtractor_VisitDue(void)
{
    u32 stage = VarGet(VAR_MOVE_EXTRACTOR_STAGE);
    gSpecialVar_Result = FALSE;
    if (gSpecialVar_0x8004 == 2)
        gSpecialVar_Result = (stage == 1 && HasTrainerBeenFought(TRAINER_BOSS_GIOVANNI_2));
    else if (gSpecialVar_0x8004 == 3)
        gSpecialVar_Result = (stage == 2 && FlagGet(FLAG_GYM_APPROVAL_GIOVANNI));
}
