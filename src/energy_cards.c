// Pokémon Canvas: Grandmaster Rod's energy cards (the prize for beating him at the Pokémon Trading
// Card Game: src/tcg/). Medicine-pocket items.
//   Six type energies: +5 base power on one damaging move of their types, for good. Once per move
//   slot (MON_DATA_ENERGY_BOOSTS, a bit per slot), on any or all of a Pokémon's four moves; the
//   boost follows the move when moves trade places and goes when the move's forgotten. Battle damage
//   adds it (CalcMoveBasePowerAfterModifiers, src/battle_util.c); the summary shows it.
//   Double Colorless Energy: every move's PP raised as far as PP Ups go (a PP Max on each).
#include "global.h"
#include "energy_cards.h"
#include "battle_main.h"
#include "move.h"
#include "pokemon.h"
#include "constants/items.h"
#include "constants/moves.h"

// Each type energy's three types (Fire's are Nick's: Fire, Dragon, Flying)
static const u8 sEnergyTypes[][3] =
{
    [ITEM_GRASS_ENERGY - ITEM_GRASS_ENERGY]     = {TYPE_GRASS, TYPE_BUG, TYPE_POISON},
    [ITEM_FIRE_ENERGY - ITEM_GRASS_ENERGY]      = {TYPE_FIRE, TYPE_DRAGON, TYPE_FLYING},
    [ITEM_WATER_ENERGY - ITEM_GRASS_ENERGY]     = {TYPE_WATER, TYPE_ICE, TYPE_FAIRY},
    [ITEM_LIGHTNING_ENERGY - ITEM_GRASS_ENERGY] = {TYPE_ELECTRIC, TYPE_STEEL, TYPE_NORMAL},
    [ITEM_PSYCHIC_ENERGY - ITEM_GRASS_ENERGY]   = {TYPE_PSYCHIC, TYPE_GHOST, TYPE_DARK},
    [ITEM_FIGHTING_ENERGY - ITEM_GRASS_ENERGY]  = {TYPE_FIGHTING, TYPE_GROUND, TYPE_ROCK},
};

bool32 EnergyCard_IsTypeEnergy(enum Item item)
{
    return item >= ITEM_GRASS_ENERGY && item <= ITEM_FIGHTING_ENERGY;
}

bool32 EnergyCard_CoversType(enum Item item, enum Type type)
{
    u32 i;
    if (!EnergyCard_IsTypeEnergy(item))
        return FALSE;
    for (i = 0; i < 3; i++)
        if (sEnergyTypes[item - ITEM_GRASS_ENERGY][i] == type)
            return TRUE;
    return FALSE;
}

// The type a move has for this Pokémon, as its summary shows it (an extracted move takes its type)
static enum Type MoveTypeForMon(struct Pokemon *mon, enum Move move)
{
    if (IsStolenMove(mon, move))
        return GetStolenMoveType(mon);
    return CheckDynamicMoveType(mon, move, 0, MON_OUTSIDE_BATTLE);
}

// Whether a type energy can boost the move in this slot: a damaging move (one with a base power:
// not a status move, nor a fixed-damage one like Seismic Toss) of one of its types, not boosted yet
bool32 EnergyCard_CanBoostSlot(struct Pokemon *mon, u32 slot, enum Item item)
{
    enum Move move;
    if (slot >= MAX_MON_MOVES || !EnergyCard_IsTypeEnergy(item))
        return FALSE;
    move = GetMonData(mon, MON_DATA_MOVE1 + slot);
    if (move == MOVE_NONE || GetMoveCategory(move) == DAMAGE_CATEGORY_STATUS || GetMovePower(move) <= 1)
        return FALSE;
    if (GetMonData(mon, MON_DATA_ENERGY_BOOSTS) & (1 << slot))
        return FALSE;
    return EnergyCard_CoversType(item, MoveTypeForMon(mon, move));
}

bool32 EnergyCard_CanBoostAny(struct Pokemon *mon, enum Item item)
{
    u32 i;
    for (i = 0; i < MAX_MON_MOVES; i++)
        if (EnergyCard_CanBoostSlot(mon, i, item))
            return TRUE;
    return FALSE;
}

void EnergyCard_Boost(struct Pokemon *mon, u32 slot)
{
    u8 boosts = GetMonData(mon, MON_DATA_ENERGY_BOOSTS) | (1 << slot);
    SetMonData(mon, MON_DATA_ENERGY_BOOSTS, &boosts);
}

// Double Colorless Energy: every move's PP Ups to 3 (its PP raised by as much as that adds).
// FALSE if there's nothing to raise.
bool32 EnergyCard_MaxAllPP(struct Pokemon *mon)
{
    u32 i, bonuses = GetMonData(mon, MON_DATA_PP_BONUSES), changed = FALSE;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        enum Move move = GetMonData(mon, MON_DATA_MOVE1 + i);
        u32 now = (bonuses >> (i * 2)) & 3, before, after, pp;
        if (move == MOVE_NONE || now == 3 || GetMovePP(move) < 5)
            continue;   // (a 1-PP move like Sketch doesn't take PP Ups)
        before = CalculatePPWithBonus(move, bonuses, i);
        bonuses = (bonuses & ~(3 << (i * 2))) | (3 << (i * 2));
        after = CalculatePPWithBonus(move, bonuses, i);
        pp = GetMonData(mon, MON_DATA_PP1 + i) + (after - before);
        SetMonData(mon, MON_DATA_PP1 + i, &pp);
        changed = TRUE;
    }
    if (changed)
        SetMonData(mon, MON_DATA_PP_BONUSES, &bonuses);
    return changed;
}
