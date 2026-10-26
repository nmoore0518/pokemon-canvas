#ifndef GUARD_TEAM_BUILDER_H
#define GUARD_TEAM_BUILDER_H

// Gym Trainer hack: the Leader Academy's TEAM BUILDER (src/team_builder.c)
void TeamBuilder_Clear(void);
void TeamBuilder_Validate(void);
void Academy_OpenTeamBuilder(void);
bool32 TeamBuilder_IsAvailable(u32 form);   // (an index into gDexFormSpecies)

#endif // GUARD_TEAM_BUILDER_H
