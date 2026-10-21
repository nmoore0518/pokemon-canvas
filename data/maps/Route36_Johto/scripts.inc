Route36_Johto_MapScripts::
	.byte 0

Route36_Johto_EventScript_Border0::
	setvar VAR_0x8004, LOCALID_ROUTE36_GUARD_0
	setvar VAR_0x8005, DIR_WEST
	goto Johto_EventScript_BorderGuard

Route36_Johto_EventScript_Border1::
	setvar VAR_0x8004, LOCALID_ROUTE36_GUARD_1
	setvar VAR_0x8005, DIR_NORTH
	goto Johto_EventScript_BorderGuard
