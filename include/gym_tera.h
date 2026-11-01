#ifndef GUARD_GYM_TERA_H
#define GUARD_GYM_TERA_H

// Pokémon Canvas: Terastallization (src/gym_tera.c, GYM_HACK.md)

// Who a TRAINER_GYM_CHALLENGER's ace Terastallizes into (GymChallenge_SetTeraMode)
enum GymTeraMode
{
    GYM_TERA_NONE,      // nobody Terastallizes
    GYM_TERA_COUNTER,   // a one-type battle: a type that resists the player's gym type
    GYM_TERA_TEAM,      // a free battle: the team's own type
};

bool32 GymTera_HasOrb(void);
bool32 GymTera_IsOneTypeTrainer(u32 trainerId);
void GymTera_SetUpTrainerParty(struct Pokemon *party, u16 trainerId);
enum Type GymTera_FreeType(struct Pokemon *mon);
enum Type GymTera_PlayerBattleType(struct Pokemon *mon);

// specials (Archer, data/scripts/dad_story.inc)
void GymTera_CountMasteredTypes(void);
void GymTera_BufferMonTera(void);
void GymTera_IsTypeAllowed(void);
void GymTera_SetType(void);
void GymTera_CheckPrice(void);

#endif // GUARD_GYM_TERA_H
