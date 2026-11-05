#ifndef GUARD_ENERGY_CARDS_H
#define GUARD_ENERGY_CARDS_H

// Pokémon Canvas: Grandmaster Rod's energy cards (src/energy_cards.c)
bool32 EnergyCard_IsTypeEnergy(enum Item item);
bool32 EnergyCard_CoversType(enum Item item, enum Type type);
bool32 EnergyCard_CanBoostSlot(struct Pokemon *mon, u32 slot, enum Item item);
bool32 EnergyCard_CanBoostAny(struct Pokemon *mon, enum Item item);
void EnergyCard_Boost(struct Pokemon *mon, u32 slot);
bool32 EnergyCard_MaxAllPP(struct Pokemon *mon);

#endif // GUARD_ENERGY_CARDS_H
