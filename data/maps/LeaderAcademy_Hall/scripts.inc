LeaderAcademy_Hall_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, LeaderAcademy_Hall_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, LeaderAcademy_Hall_OnFrame
	.byte 0

LeaderAcademy_Hall_OnTransition::
	call Academy_EventScript_HallTransition
	end

LeaderAcademy_Hall_OnFrame::
	map_script_2 VAR_ACADEMY_STATE, ACADEMY_BRACELET_CALL, Academy_EventScript_BraceletCall
	map_script_2 VAR_ACADEMY_STATE, ACADEMY_BRACELET_GIVE, Academy_EventScript_BraceletGive
	.2byte 0
