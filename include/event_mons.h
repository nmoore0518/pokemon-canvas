#ifndef GUARD_EVENT_MONS_H
#define GUARD_EVENT_MONS_H

// Pokémon Canvas: the event Pokémon's side quests (src/event_mons.c)

// A Battle Bond Froakie (and its evolutions): FireRed's unused "shadow" bit
#define MON_DATA_BATTLE_BOND MON_DATA_IS_SHADOW

struct Pokemon;
u32 EventMons_EvolutionTarget(struct Pokemon *mon, u32 target);
u32 EventMons_WildVivillonPattern(u32 species);
void EventMons_HallStreak(u32 streak);
void EventMons_OnMapLoad(void);

void EventMons_GiveMon(void);
void EventMons_CostumeUnlocked(void);
void EventMons_CostumeCount(void);
void EventMons_CostumeCheckMon(void);
void EventMons_SetCostume(void);
void EventMons_CapFound(void);
void EventMons_CapReward(void);
void EventMons_CapRewardGiven(void);
void EventMons_CapHint(void);
void EventMons_PartnerReady(void);
void EventMons_RecordVivillon(void);
void EventMons_KalosGauntletBeaten(void);
void EventMons_AzBeaten(void);
void EventMons_PartyHasFloette(void);
void EventMons_KalosTrioLeft(void);
void EventMons_SetupTala(void);
void EventMons_PaintPotsLeft(void);

#endif // GUARD_EVENT_MONS_H
