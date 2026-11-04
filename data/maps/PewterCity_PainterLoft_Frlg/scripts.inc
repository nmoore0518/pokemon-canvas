@ Pokémon Canvas: the painter's loft (tools/gym_hack/build_painter_loft.py; src/painter.c). The rest of
@ her scripts, and her paintings' figure, are in data/scripts/painter.inc. (Her lines are drafts.)
PewterCity_PainterLoft_Frlg_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, PainterLoft_OnFrame
	.byte 0

PainterLoft_OnFrame::
	map_script_2 VAR_PAINTING_RETURN, 1, PainterLoft_EventScript_Returned
	.2byte 0
