@ Pokémon Canvas: the player's outfits (src/outfits.c). The wardrobe in the player's room lists
@ every outfit they have; the player shows each one as it's picked.
Outfits_EventScript_Wardrobe::
	lockall
	special Outfit_CatchUp
	special Outfit_CountUnlocked
	goto_if_le VAR_RESULT, 1, Outfits_EventScript_OnlyOne
	message Outfits_Text_WhichOutfit
	waitmessage
	call Outfits_EventScript_PushList
	dynmultistack 0, 0, FALSE, 6, FALSE, VAR_PLAYER_OUTFIT, DYN_MULTICHOICE_CB_OUTFIT
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, Outfits_EventScript_Done
	goto_if_eq VAR_RESULT, VAR_PLAYER_OUTFIT, Outfits_EventScript_Done
	copyvar VAR_0x8004, VAR_RESULT
	closemessage
	fadescreen FADE_TO_BLACK
	special Outfit_Wear
	playse SE_EXIT
	delay 20
	fadescreen FADE_FROM_BLACK
	msgbox Outfits_Text_ChangedInto
	releaseall
	end

Outfits_EventScript_Done::
	closemessage
	releaseall
	end

Outfits_EventScript_OnlyOne::
	msgbox Outfits_Text_OnlyOne
	releaseall
	end

@ The basement's crates: one of them holds DAD's old TEAM ROCKET uniform (the player doesn't know it's his yet)
Outfits_EventScript_BasementCrates::
	setvar VAR_0x8004, OUTFIT_ROCKET
	special Outfit_CheckUnlocked
	goto_if_eq VAR_RESULT, TRUE, Outfits_EventScript_CratesEmpty
	lockall
	msgbox Outfits_Text_LooseCrate
	setvar VAR_0x8004, OUTFIT_ROCKET
	special Outfit_UnlockSpecial
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	message Outfits_Text_FoundRocketUniform
	waitmessage
	waitfanfare
	msgbox Outfits_Text_WhyIsThisHere
	msgbox Outfits_Text_PutInWardrobe
	releaseall
	end

Outfits_EventScript_CratesEmpty::
	msgbox Outfits_Text_CratesEmpty, MSGBOX_SIGN
	end

@ The mentor's first gift (after Mt. Moon) comes with the gym uniform of the player's type
Outfits_EventScript_GymUniform::
	special Outfit_GiveGymUniform
	goto_if_eq VAR_RESULT, FALSE, Outfits_EventScript_Return
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	message Outfits_Text_ReceivedGymUniform
	waitmessage
	waitfanfare
	msgbox Outfits_Text_PutInWardrobe
Outfits_EventScript_Return::
	return

@ Changing the gym's type (after a mastery): every gym uniform, the first time
Outfits_EventScript_AllGymUniforms::
	special Outfit_UnlockAllGymUniforms
	goto_if_eq VAR_RESULT, FALSE, Outfits_EventScript_Return
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	message Outfits_Text_ReceivedAllGymUniforms
	waitmessage
	waitfanfare
	msgbox Outfits_Text_AllInWardrobe
	return

	.include "data/scripts/outfits_list.inc"

@ (Placeholder lines: Nick writes the final ones)
Outfits_Text_WhichOutfit:
	.string "Which outfit will you wear?$"

Outfits_Text_ChangedInto:
	.string "{PLAYER} changed into the\n"
	.string "{STR_VAR_1}!$"

Outfits_Text_OnlyOne:
	.string "It's your wardrobe.\n"
	.string "You don't have any other outfits\l"
	.string "yet.$"

Outfits_Text_LooseCrate:
	.string "Wooden crates, nailed shut…\p"
	.string "The lid of one of them is loose.\n"
	.string "There's something folded inside.$"

Outfits_Text_FoundRocketUniform:
	.string "{PLAYER} found a\n"
	.string "TEAM ROCKET uniform!$"

Outfits_Text_WhyIsThisHere:
	.string "A TEAM ROCKET uniform?\n"
	.string "Why is this here?$"

Outfits_Text_PutInWardrobe:
	.string "{PLAYER} put it in the wardrobe\n"
	.string "upstairs.$"

Outfits_Text_CratesEmpty:
	.string "Wooden crates, nailed shut.\n"
	.string "One of them is empty.$"

Outfits_Text_ReceivedAllGymUniforms:
	.string "{PLAYER} received a GYM UNIFORM\n"
	.string "for every type!$"

Outfits_Text_AllInWardrobe:
	.string "They're in the wardrobe at home.$"

Outfits_Text_ReceivedGymUniform:
	.string "{PLAYER} received a\n"
	.string "{STR_VAR_1} GYM UNIFORM!$"
