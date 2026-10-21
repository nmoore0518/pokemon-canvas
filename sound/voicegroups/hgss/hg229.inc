	.align 2
voicegroup_hg229:: @ HGSS main
	voice_keysplit voicegroup_hg233, KeySplitTable_hg35    @ piano
	voice_keysplit voicegroup_hg234, KeySplitTable_hg36    @ piano 2
	voice_keysplit voicegroup_hg235, KeySplitTable_hg37    @ piano octave
	voice_keysplit voicegroup_hg236, KeySplitTable_hg9     @ rhodes 1
	voice_keysplit voicegroup_hg237, KeySplitTable_hg9     @ rhodes 2
	voice_keysplit voicegroup_hg238, KeySplitTable_hg36    @ vibraphone
	voice_keysplit voicegroup_hg239, KeySplitTable_hg38    @ marimba
	voice_keysplit voicegroup_hg240, KeySplitTable_hg39    @ xylophone
	voice_keysplit voicegroup_hg241, KeySplitTable_hg25    @ tubular bell
	voice_keysplit voicegroup_hg242, KeySplitTable_hg36    @ glockenspiel
	voice_keysplit voicegroup_hg243, KeySplitTable_hg38    @ music box
	voice_keysplit voicegroup_hg244, KeySplitTable_hg40    @ drawbar organ
	voice_keysplit voicegroup_hg245, KeySplitTable_hg40    @ jazz organ
	voice_keysplit voicegroup_hg246, KeySplitTable_hg40    @ accordion
	voice_keysplit voicegroup_hg247, KeySplitTable_hg41    @ nylon guitar
	voice_keysplit voicegroup_hg248, KeySplitTable_hg41    @ martin guitar
	voice_keysplit voicegroup_hg249, KeySplitTable_hg32    @ distortion guitar 1
	voice_keysplit voicegroup_hg250, KeySplitTable_hg32    @ distortion guitar 2
	voice_keysplit voicegroup_hg251, KeySplitTable_hg41    @ clean guitar
	voice_keysplit voicegroup_hg252, KeySplitTable_hg42    @ upright bass
	voice_keysplit voicegroup_hg253, KeySplitTable_hg43    @ fingered bass
	voice_directsound 60, 0, DirectSoundWaveData_dp_slapbass_c1_16, 255, 0, 255, 192
	voice_keysplit voicegroup_hg254, KeySplitTable_hg44    @ synth bass
	voice_keysplit voicegroup_hg255, KeySplitTable_hg45    @ strings
	voice_keysplit voicegroup_hg256, KeySplitTable_hg46    @ violin
	voice_keysplit voicegroup_hg257, KeySplitTable_hg47    @ cello
	voice_keysplit voicegroup_hg258, KeySplitTable_hg48    @ pizzicato
	voice_keysplit voicegroup_hg259, KeySplitTable_hg40    @ harp
	voice_keysplit voicegroup_hg260, KeySplitTable_hg40    @ timpani
	voice_keysplit voicegroup_hg261, KeySplitTable_hg39    @ trumpets
	voice_keysplit voicegroup_hg262, KeySplitTable_hg47    @ trombones
	voice_keysplit voicegroup_hg263, KeySplitTable_hg49    @ french horn solo
	voice_keysplit voicegroup_hg264, KeySplitTable_hg50    @ french horns
	voice_keysplit voicegroup_hg265, KeySplitTable_hg51    @ oboe
	voice_keysplit voicegroup_hg266, KeySplitTable_hg52    @ clarinet
	voice_keysplit voicegroup_hg267, KeySplitTable_hg53    @ flute
	voice_keysplit voicegroup_hg268, KeySplitTable_hg54    @ bassoon
	voice_keysplit voicegroup_hg269, KeySplitTable_hg55    @ orchestra hit
	voice_keysplit voicegroup_hg270, KeySplitTable_hg38    @ square wave
	voice_keysplit_all voicegroup_hg230				 @ drums
	voice_keysplit_all voicegroup_hg231				 @ taiko drums
	voice_directsound 60, 0, DirectSoundWaveData_dp_woodbass_d3_16, 255, 251, 0, 171
	voice_keysplit voicegroup_hg271, KeySplitTable_hg56    @ koto
	voice_directsound 60, 0, DirectSoundWaveData_puresquare_50, 255, 242, 40, 211
	voice_directsound 60, 0, DirectSoundWaveData_sc88pro_wind, 67, 243, 155, 224
	voice_directsound 60, 0, DirectSoundWaveData_dp_explosion, 67, 255, 0, 245
	voice_directsound 60, 0, DirectSoundWaveData_sc88pro_square_wave, 255, 250, 0, 171 @ 1 octave too low
	voice_directsound 60, 0, DirectSoundWaveData_sc88pro_bubbles, 255, 0, 255, 0
	voice_directsound 60, 0, DirectSoundWaveData_puresquare_50, 255, 242, 25, 0  @ also 066 in BATTLE4 and DUNGEON4
	voice_directsound 60, 0, DirectSoundWaveData_puresquare_50, 255, 252, 0, 96
	voice_directsound 60, 0, DirectSoundWaveData_puresquare_50, 255, 250, 0, 220
	voice_directsound 60, 0, DirectSoundWaveData_puresquare_50, 224, 242, 101, 220
	voice_directsound 60, 0, DirectSoundWaveData_puresquare_50, 224, 242, 101, 192
	voice_directsound 60, 0, DirectSoundWaveData_purenoise, 42, 0, 57, 192
	voice_directsound 60, 0, DirectSoundWaveData_purenoise, 240, 242, 0, 192
@	voice_directsound 60, 0, DirectSoundWaveData_dp_whistle_c5_16, 255, 246, 78, 192	@ remaining voices wrap around
@	voice_directsound 60, 0, DirectSoundWaveData_dp_reverscyn_16, 255, 0, 255, 16		@ to the next voice group, so
@	voice_directsound 60, 0, DirectSoundWaveData_dp_org5_c3_16, 255, 167, 235, 171		@ have some dittos
@	voice_directsound 60, 0, DirectSoundWaveData_puresquare_37, 80, 234, 191, 220
@	voice_directsound 60, 0, DirectSoundWaveData_puresquare_12, 80, 234, 191, 220
@	voice_directsound 60, 0, DirectSoundWaveData_dp_orchhitmajor60, 255, 197, 255, 171						@ 60
@	voice_directsound 60, 0, DirectSoundWaveData_dp_orchhitminor60, 255, 197, 255, 171						@ 61
@	voice_keysplit_all voicegroup_hg232				 @ synth kick+snare										@ 62
@	voice_keysplit voicegroup_hg216, KeySplitTable_hg27    @ DPPt timpani											@ 63
@	voice_directsound 60, 0, DirectSoundWaveData_dp_synbass1_c2_16, 255, 42, 255, 192						@ 64
@	voice_directsound 60, 0, DirectSoundWaveData_dp_synbass2_c1_16, 255, 0, 255, 220						@ 65
@	voice_directsound 60, 0, DirectSoundWaveData_dp_sawtoothlead60, 255, 197, 255, 171						@ 66
@	voice_keysplit voicegroup_hg272, KeySplitTable_hg57    @ shamisen												@ 67
@	voice_directsound 60, 0, DirectSoundWaveData_hg_hyoshigi, 255, 0, 255, 171								@ 68
@	voice_directsound 60, 0, DirectSoundWaveData_hg_kodaiko, 255, 0, 255, 171								@ 69
@	voice_directsound 60, 0, DirectSoundWaveData_hg_tsuzumi, 255, 0, 255, 171								@ 70
@	voice_directsound 60, 0, DirectSoundWaveData_hg_jinglebell, 255, 0, 255, 171							@ 71
@	voice_directsound 60, 0, DirectSoundWaveData_hg_hoohclap, 255, 0, 255, 0								@ 72
@	voice_directsound 60, 0, DirectSoundWaveData_dp_tenorchoir48, 255, 197, 255, 171						@ 73
@	voice_directsound 60, 0, DirectSoundWaveData_dp_sopranochoir60, 255, 197, 255, 171						@ 74
@	voice_directsound 60, 0, DirectSoundWaveData_hg_choirc5, 255, 197, 255, 171 @ 1 octave too low			@ 75
@	voice_directsound 60, 0, DirectSoundWaveData_dp_reverscyn_16, 255, 197, 255, 171 @ 1 octave too high	@ 76
@	voice_directsound 60, 0, DirectSoundWaveData_dp_spearpillarwind60, 255, 0, 255, 32						@ 77
@	voice_directsound 60, 0, DirectSoundWaveData_dp_altosax_c3_16, 255, 216, 252, 192						@ 78
@	voice_directsound 60, 0, DirectSoundWaveData_dp_synthsnare, 255, 0, 255, 171							@ 79
@	voice_keysplit voicegroup_hg273, KeySplitTable_hg58    @ piano (national park)								@ 80
@	voice_directsound 60, 0, DirectSoundWaveData_dp_tom1_16, 255, 0, 255, 32								@ 81
@	voice_keysplit voicegroup_hg219, KeySplitTable_hg29    @ dp trumpets (arceus)									@ 82
@	voice_keysplit voicegroup_hg220, KeySplitTable_hg30    @ dp trombone (arceus)									@ 83
