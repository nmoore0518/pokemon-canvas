#ifndef GUARD_TCG_SOUND_H
#define GUARD_TCG_SOUND_H

// src/tcg/sound.c: the Trading Card Game's sound engine (pret/poketcg's music1.asm and sfx.asm) on the
// GBA's Game Boy channels. TcgSound_Start takes the channels over (m4a's songs should be stopped
// first), TcgSound_Stop gives them back; TcgSound_Update runs once a VBlank.
void TcgSound_Start(void);
void TcgSound_Stop(void);
bool32 TcgSound_IsActive(void);
void TcgSound_Update(void);
void TcgSound_PlayMapMusic(u8 mapGroup, u8 mapNum);   // (src/tcg/tcg_main.c: the card house's music, or none)
void PlayTcgHouseMusic(void);

void Tcg_PlaySong(u32 song);            // (TCG_MUSIC_*)
u32 Tcg_GetPlayingSong(void);
bool32 Tcg_IsSongPlaying(void);         // (AssertSongFinished)
bool32 Tcg_IsSFXPlaying(void);
void Tcg_PlaySFX(u32 sfx);              // (TCG_SFX_*)
void Tcg_PlaySFX_InvalidChoice(void);

#endif // GUARD_TCG_SOUND_H
