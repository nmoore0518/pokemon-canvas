@ ExoticSafari_JohtoRuinsOfAlph (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/johto_safari.py)
ExoticSafari_JohtoRuinsOfAlph_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ExoticSafari_JohtoRuinsOfAlph_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, ExoticSafari_JohtoRuinsOfAlph_OnLoad
	.byte 0

ExoticSafari_JohtoRuinsOfAlph_OnTransition::
	special ExoticSafari_AreaOnTransition
	end

ExoticSafari_JohtoRuinsOfAlph_OnLoad::
	special ExoticSafari_AreaOnLoad
	end

ExoticSafari_JohtoRuinsOfAlph_EventScript_GsBall::
	lock
	setflag FLAG_EXOTIC_GS_BALL
	bufferstring STR_VAR_1, ExoticSafari_JohtoRuinsOfAlph_EventScript_GsBall_Name
	goto ExoticSafari_EventScript_JohtoKeepsake

ExoticSafari_JohtoRuinsOfAlph_EventScript_GsBall_Name:
	.string "GS BALL$"

ExoticSafari_JohtoRuinsOfAlph_EventScript_UnownPanel0::
	lockall
	setvar VAR_0x8004, 0
	goto ExoticSafari_EventScript_UnownPanel

ExoticSafari_JohtoRuinsOfAlph_EventScript_UnownPanel1::
	lockall
	setvar VAR_0x8004, 1
	goto ExoticSafari_EventScript_UnownPanel

ExoticSafari_JohtoRuinsOfAlph_EventScript_UnownPanel2::
	lockall
	setvar VAR_0x8004, 2
	goto ExoticSafari_EventScript_UnownPanel

ExoticSafari_JohtoRuinsOfAlph_EventScript_UnownPanel3::
	lockall
	setvar VAR_0x8004, 3
	goto ExoticSafari_EventScript_UnownPanel
