#ifndef GUARD_ARCHIVE_H
#define GUARD_ARCHIVE_H

// Gym Trainer hack: BILL'S ARCHIVE, the living dex (src/archive.c)
void Archive_Clear(void);
void Archive_Validate(void);
bool32 Archive_IsSlot(u32 species);
bool32 Archive_Has(u32 species);
void Archive_Store(u32 species, bool32 shiny);
void AccessArchivePC(void);

#endif // GUARD_ARCHIVE_H
