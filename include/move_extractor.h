#ifndef GUARD_MOVE_EXTRACTOR_H
#define GUARD_MOVE_EXTRACTOR_H

// Gym Trainer hack: the Move Extractor (src/move_extractor.c)

const u8 *MoveExtractor_CantUseReason(u32 battler);
enum Move MoveExtractor_GetMove(void);
bool32 MoveExtractor_MarkLearned(struct Pokemon *mon, enum Move move);
void MoveExtractor_TakeFromFoe(u32 foe, enum Move move);

// Battle script natives
void BS_MoveExtractorStart(void);
void BS_MoveExtractorTryGive(void);
void BS_MoveExtractorFinish(void);

// Specials (data/scripts/move_extractor.inc)
void MoveExtractor_VisitDue(void);

#endif // GUARD_MOVE_EXTRACTOR_H
