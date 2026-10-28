LeaderAcademy_Campus_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, LeaderAcademy_Campus_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, LeaderAcademy_Campus_OnFrame
	.byte 0

LeaderAcademy_Campus_OnTransition::
	setworldmapflag FLAG_WORLD_MAP_LEADER_ACADEMY
	call Academy_EventScript_CampusTransition
	end

LeaderAcademy_Campus_OnFrame::
	map_script_2 VAR_ACADEMY_STATE, ACADEMY_WELCOME, Academy_EventScript_Welcome
	.2byte 0
