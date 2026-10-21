#ifndef GUARD_GYM_LEGEND_H
#define GUARD_GYM_LEGEND_H

// Gym Trainer hack: the legendary Pokémon of the player's type (src/gym_legend.c)
extern const u16 gGymLegends[NUMBER_OF_MON_TYPES];

u16 GymLegend_GetSpecies(u32 type);
void GymLegend_Setup(void);
void GymLegend_FindSpot(void);
void GymLegend_OnLoad(void);
void GymLegend_CreateMon(void);
void GymLegend_PlanCamera(void);
bool32 GymLegend_IsCaveEncounterActive(void);
bool32 GymLegend_TryCaveEncounter(void);
void GymLegend_AfterWildBattle(u32 outcome);
bool32 GymLegend_ChangeType(u32 from, u32 to);
extern u8 gGymLegendCameraIn[];
extern u8 gGymLegendCameraOut[];

#endif // GUARD_GYM_LEGEND_H
