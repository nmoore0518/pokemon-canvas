Route40_Johto_MapScripts::
	.byte 0

Route40_Johto_EventScript_Border0::
	setvar VAR_0x8004, LOCALID_ROUTE40_GUARD_0
	setvar VAR_0x8005, DIR_NORTH
	goto Johto_EventScript_BorderGuard

Route40_Johto_EventScript_Border1::
	setvar VAR_0x8004, LOCALID_ROUTE40_GUARD_1
	setvar VAR_0x8005, DIR_SOUTH
	goto Johto_EventScript_BorderGuard
