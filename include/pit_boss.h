#ifndef GUARD_PIT_BOSS_H
#define GUARD_PIT_BOSS_H

// Pokémon Canvas: the back room under Celadon (src/pit_boss.c)

// (saved in flash sector 31, after the registered items: src/save.c)
struct PitBossSave
{
    u16 magic;
    u16 day;                    // (the "days" of steps so far: his Pokémon of the day comes from it)
    bool8 stakeHeld;            // (a battle's on: the stake's his if it never finishes)
    bool8 heldValid;            // (he has one of the player's Pokémon, for sale)
    bool8 forfeited;            // (the power went off mid-battle: he's yet to say so)
    u8 padding;
    u32 stakePersonality;       // (the stake: which of the player's Pokémon it is)
    u32 stakeOtId;
};

extern struct PitBossSave gPitBossSave;
extern struct BoxPokemon gPitBossHeld;      // (the Pokémon he's won: flash sector 30, after the Mastery Hall's data)

void PitBoss_Clear(void);
void PitBoss_Validate(void);
void PitBoss_NewDay(void);
void PitBoss_OnStep(void);
bool32 PitBoss_FightActive(void);
u32 PitBoss_Power(struct Pokemon *mon);
u32 PitBoss_GetSpecies(void);
u32 PitBoss_DayPower(u32 level);

// Specials
void PitBoss_OnTransition(void);
void PitBoss_CheckForfeit(void);
void PitBoss_BufferDay(void);
void PitBoss_HasHeld(void);
void PitBoss_BuyBack(void);
void PitBoss_CheckStake(void);
void PitBoss_HoldStake(void);
void PitBoss_SetupFight(void);
void PitBoss_EndFight(void);
void Pit_AutoSave(void);

#endif // GUARD_PIT_BOSS_H
