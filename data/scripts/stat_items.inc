@ Gym Trainer hack: using the stat training items from the bag (see src/stat_items.c).
@ VAR_ITEM_ID is the item; ChoosePartyMon leaves the chosen slot in VAR_0x8004.

GymHack_EventScript_UseStatItem::
	lockall
	special ChoosePartyMon
	goto_if_ge VAR_0x8004, PARTY_SIZE, GymHack_EventScript_StatItemDone
	special StatItems_CheckMon
	goto_if_eq VAR_RESULT, 1, GymHack_EventScript_StatItemEgg
	goto_if_eq VAR_RESULT, 2, GymHack_EventScript_StatItemNoEffect
	switch VAR_ITEM_ID
	case ITEM_BOTTLE_CAP, GymHack_EventScript_SilverBottleCap
	case ITEM_GOLD_BOTTLE_CAP, GymHack_EventScript_GoldBottleCap
	case ITEM_ABILITY_CAPSULE, GymHack_EventScript_AbilityCapsule
	case ITEM_CILANTRO, GymHack_EventScript_Cilantro
	case ITEM_MULTIVITAMIN, GymHack_EventScript_Multivitamin
	goto GymHack_EventScript_StatItemDone
	end

GymHack_EventScript_StatItemDone::
	closemessage
	releaseall
	end

GymHack_EventScript_StatItemEgg::
	msgbox GymHack_Text_StatItemEgg
	goto GymHack_EventScript_StatItemDone
	end

GymHack_EventScript_StatItemNoEffect::
	bufferitemname STR_VAR_2, VAR_ITEM_ID
	msgbox GymHack_Text_StatItemNoEffect
	goto GymHack_EventScript_StatItemDone
	end

@ Uses up the item and shows the message StatItems_Apply wrote
GymHack_EventScript_StatItemUsed::
	removeitem VAR_ITEM_ID
	playfanfare MUS_LEVEL_UP
	message gStringVar4
	waitmessage
	waitfanfare
	waitbuttonpress
	goto GymHack_EventScript_StatItemDone
	end

GymHack_EventScript_SilverBottleCap::
	message GymHack_Text_WhichStat
	waitmessage
	special StatItems_PushIvList
	dynmultistack 0, 0, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, GymHack_EventScript_StatItemDone
	copyvar VAR_0x8005, VAR_RESULT
	special StatItems_Apply
	goto_if_eq VAR_RESULT, TRUE, GymHack_EventScript_StatItemUsed
	msgbox gStringVar4
	goto GymHack_EventScript_SilverBottleCap
	end

GymHack_EventScript_GoldBottleCap::
	msgbox GymHack_Text_UseGoldBottleCap, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, GymHack_EventScript_StatItemDone
	special StatItems_Apply
	goto GymHack_EventScript_StatItemUsed
	end

GymHack_EventScript_AbilityCapsule::
	message GymHack_Text_WhichAbility
	waitmessage
	special StatItems_PushAbilityList
	dynmultistack 0, 0, FALSE, 3, FALSE, VAR_0x8005, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, GymHack_EventScript_StatItemDone
	copyvar VAR_0x8005, VAR_RESULT
	special StatItems_Apply
	goto_if_eq VAR_RESULT, TRUE, GymHack_EventScript_StatItemUsed
	msgbox gStringVar4
	goto GymHack_EventScript_AbilityCapsule
	end

GymHack_EventScript_Cilantro::
	message GymHack_Text_WhichNature
	waitmessage
	special StatItems_PushNatureList
	dynmultistack 0, 0, FALSE, 6, FALSE, VAR_0x8005, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, GymHack_EventScript_StatItemDone
	copyvar VAR_0x8005, VAR_RESULT
	special StatItems_Apply
	goto_if_eq VAR_RESULT, TRUE, GymHack_EventScript_StatItemUsed
	msgbox gStringVar4
	goto GymHack_EventScript_Cilantro
	end

GymHack_EventScript_Multivitamin::
	closemessage
	special StatItems_OpenEvEditor
	goto_if_eq VAR_RESULT, 0, GymHack_EventScript_StatItemDone
	goto_if_eq VAR_RESULT, 2, GymHack_EventScript_MultivitaminUnchanged
	removeitem VAR_ITEM_ID
	playfanfare MUS_LEVEL_UP
	message GymHack_Text_MultivitaminUsed
	waitmessage
	waitfanfare
	waitbuttonpress
	goto GymHack_EventScript_StatItemDone
	end

GymHack_EventScript_MultivitaminUnchanged::
	msgbox GymHack_Text_MultivitaminUnchanged
	goto GymHack_EventScript_StatItemDone
	end

@ ---- The Pallet Town Mart (the usual stock plus unlocked stat items) ----
GymHack_EventScript_PalletMartClerk::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	special GymHack_PalletMart
	msgbox gText_PleaseComeAgain
	release
	end

@ ---- Gifts from boss battles ----
@ Gives VAR_0x8009. If the bag is full it goes to the PC instead, so a gift is never lost.
GymHack_EventScript_GiveStatItem::
	giveitem VAR_0x8009
	goto_if_eq VAR_RESULT, TRUE, GymHack_EventScript_GiveStatItemDone
	addpcitem VAR_0x8009, 1
	bufferitemname STR_VAR_2, VAR_0x8009
	msgbox GymHack_Text_GiftSentToPC
GymHack_EventScript_GiveStatItemDone::
	return

GymHack_EventScript_BrockGifts::
	goto_if_set FLAG_GOT_SILVER_CAP_FROM_BROCK, GymHack_EventScript_GiftsDone
	msgbox GymHack_Text_BrockSilverCap
	setvar VAR_0x8009, ITEM_BOTTLE_CAP
	call GymHack_EventScript_GiveStatItem
	setflag FLAG_GOT_SILVER_CAP_FROM_BROCK
	msgbox GymHack_Text_BrockSilverCapAfter
	return

GymHack_EventScript_MistyGifts::
	goto_if_set FLAG_GOT_SILVER_CAP_FROM_MISTY, GymHack_EventScript_GiftsDone
	msgbox GymHack_Text_MistySilverCap
	setvar VAR_0x8009, ITEM_BOTTLE_CAP
	call GymHack_EventScript_GiveStatItem
	setflag FLAG_GOT_SILVER_CAP_FROM_MISTY
	msgbox GymHack_Text_MistySilverCapAfter
	return

GymHack_EventScript_SurgeGifts::
	goto_if_set FLAG_GOT_SILVER_CAP_FROM_SURGE, GymHack_EventScript_GiftsDone
	msgbox GymHack_Text_SurgeSilverCap
	setvar VAR_0x8009, ITEM_BOTTLE_CAP
	call GymHack_EventScript_GiveStatItem
	setflag FLAG_GOT_SILVER_CAP_FROM_SURGE
	msgbox GymHack_Text_SurgeSilverCapAfter
	return

GymHack_EventScript_ErikaGifts::
	goto_if_set FLAG_GOT_SILVER_CAP_FROM_ERIKA, GymHack_EventScript_ErikaCilantro
	msgbox GymHack_Text_ErikaSilverCap
	setvar VAR_0x8009, ITEM_BOTTLE_CAP
	call GymHack_EventScript_GiveStatItem
	setflag FLAG_GOT_SILVER_CAP_FROM_ERIKA
GymHack_EventScript_ErikaCilantro::
	goto_if_set FLAG_GOT_CILANTRO_FROM_ERIKA, GymHack_EventScript_GiftsDone
	msgbox GymHack_Text_ErikaCilantro
	setvar VAR_0x8009, ITEM_CILANTRO
	call GymHack_EventScript_GiveStatItem
	setflag FLAG_GOT_CILANTRO_FROM_ERIKA
	msgbox GymHack_Text_ErikaCilantroAfter
	return

GymHack_EventScript_KogaGifts::
	goto_if_set FLAG_GOT_SILVER_CAP_FROM_KOGA, GymHack_EventScript_KogaCapsule
	msgbox GymHack_Text_KogaSilverCap
	setvar VAR_0x8009, ITEM_BOTTLE_CAP
	call GymHack_EventScript_GiveStatItem
	setflag FLAG_GOT_SILVER_CAP_FROM_KOGA
GymHack_EventScript_KogaCapsule::
	goto_if_set FLAG_GOT_ABILITY_CAPSULE_FROM_KOGA, GymHack_EventScript_GiftsDone
	msgbox GymHack_Text_KogaAbilityCapsule
	setvar VAR_0x8009, ITEM_ABILITY_CAPSULE
	call GymHack_EventScript_GiveStatItem
	setflag FLAG_GOT_ABILITY_CAPSULE_FROM_KOGA
	msgbox GymHack_Text_KogaAbilityCapsuleAfter
	return

GymHack_EventScript_SabrinaGifts::
	goto_if_set FLAG_GOT_SILVER_CAP_FROM_SABRINA, GymHack_EventScript_GiftsDone
	msgbox GymHack_Text_SabrinaSilverCap
	setvar VAR_0x8009, ITEM_BOTTLE_CAP
	call GymHack_EventScript_GiveStatItem
	setflag FLAG_GOT_SILVER_CAP_FROM_SABRINA
	msgbox GymHack_Text_SabrinaSilverCapAfter
	return

GymHack_EventScript_BlaineGifts::
	goto_if_set FLAG_GOT_SILVER_CAP_FROM_BLAINE, GymHack_EventScript_GiftsDone
	msgbox GymHack_Text_BlaineSilverCap
	setvar VAR_0x8009, ITEM_BOTTLE_CAP
	call GymHack_EventScript_GiveStatItem
	setflag FLAG_GOT_SILVER_CAP_FROM_BLAINE
	msgbox GymHack_Text_BlaineSilverCapAfter
	return

GymHack_EventScript_GiovanniGifts::
	goto_if_set FLAG_GOT_SILVER_CAP_FROM_GIOVANNI, GymHack_EventScript_GiftsDone
	msgbox GymHack_Text_GiovanniSilverCap
	setvar VAR_0x8009, ITEM_BOTTLE_CAP
	call GymHack_EventScript_GiveStatItem
	setflag FLAG_GOT_SILVER_CAP_FROM_GIOVANNI
	msgbox GymHack_Text_GiovanniSilverCapAfter
	return

@ Pokémon Tower, after the rival battle (the rival leaves right after, so this is given once)
GymHack_EventScript_RivalTowerGift::
	goto_if_set FLAG_GOT_GOLD_CAP_FROM_RIVAL, GymHack_EventScript_GiftsDone
	msgbox GymHack_Text_RivalGoldCap
	setvar VAR_0x8009, ITEM_GOLD_BOTTLE_CAP
	call GymHack_EventScript_GiveStatItem
	setflag FLAG_GOT_GOLD_CAP_FROM_RIVAL
	msgbox GymHack_Text_RivalGoldCapAfter
	return

@ Silph Co. 11F, after beating Giovanni
GymHack_EventScript_SilphGiovanniGift::
	goto_if_set FLAG_GOT_MULTIVITAMIN_FROM_GIOVANNI, GymHack_EventScript_GiftsDone
	msgbox GymHack_Text_SilphGiovanniMultivitamin
	setvar VAR_0x8009, ITEM_MULTIVITAMIN
	call GymHack_EventScript_GiveStatItem
	setflag FLAG_GOT_MULTIVITAMIN_FROM_GIOVANNI
	msgbox GymHack_Text_SilphGiovanniMultivitaminAfter
	return

GymHack_EventScript_GiftsDone::
	return

@ ---- Text ----
GymHack_Text_StatItemEgg::
	.string "That won't work on an EGG.$"

GymHack_Text_StatItemNoEffect::
	.string "The {STR_VAR_2} won't have any\n"
	.string "effect on {STR_VAR_1}.$"

GymHack_Text_WhichStat::
	.string "Which of {STR_VAR_1}'s stats\n"
	.string "should reach its full potential?$"

GymHack_Text_UseGoldBottleCap::
	.string "Use the GOLD BOTTLE CAP?\n"
	.string "All of {STR_VAR_1}'s stats will be maxed.$"

GymHack_Text_WhichAbility::
	.string "Which ABILITY should\n"
	.string "{STR_VAR_1} have?$"

GymHack_Text_WhichNature::
	.string "Which nature should\n"
	.string "{STR_VAR_1} have?$"

GymHack_Text_MultivitaminUsed::
	.string "{STR_VAR_1}'s effort values\n"
	.string "were set!$"

GymHack_Text_MultivitaminUnchanged::
	.string "Nothing was changed, so the\n"
	.string "MULTIVITAMIN wasn't used.$"

GymHack_Text_GiftSentToPC::
	.string "Your BAG is full, so the\n"
	.string "{STR_VAR_2} was sent to your PC.$"

@ Brock: steady, a mentor
GymHack_Text_BrockSilverCap::
	.string "BROCK: One more thing.\p"
	.string "The LEAGUE sends every GYM a few of\n"
	.string "these for promising challengers.\p"
	.string "PROF. OAK told us about the new\n"
	.string "GYM. I'd say you qualify.$"

GymHack_Text_BrockSilverCapAfter::
	.string "That SILVER BOTTLE CAP brings one\n"
	.string "of a POKéMON's stats to its peak.\p"
	.string "A strong team is built like a\n"
	.string "mountain: one solid layer at a\l"
	.string "time. Choose wisely!$"

@ Misty: proud, a little competitive
GymHack_Text_MistySilverCap::
	.string "MISTY: Oh, and the LEAGUE makes us\n"
	.string "hand these out. Here!$"

GymHack_Text_MistySilverCapAfter::
	.string "A SILVER BOTTLE CAP maxes out one\n"
	.string "stat of a POKéMON.\p"
	.string "Don't get a big head, though.\n"
	.string "A GYM LEADER needs more than one\l"
	.string "good stat!$"

@ Lt. Surge: loud, military
GymHack_Text_SurgeSilverCap::
	.string "LT. SURGE: Hey, kid! Catch!$"

GymHack_Text_SurgeSilverCapAfter::
	.string "That's a SILVER BOTTLE CAP!\p"
	.string "In the war, we got every last volt\n"
	.string "out of our POKéMON!\p"
	.string "Pick one stat and max it out!\n"
	.string "That's an order, soldier!$"

@ Erika: gentle, a gardener
GymHack_Text_ErikaSilverCap::
	.string "ERIKA: Please, accept this as well.\n"
	.string "It's from the POKéMON LEAGUE.$"

GymHack_Text_ErikaCilantro::
	.string "And this is from my own garden.\p"
	.string "I grow CILANTRO in the GYM's\n"
	.string "greenhouse.\p"
	.string "Some people adore its scent, and\n"
	.string "some can't stand it at all…$"

GymHack_Text_ErikaCilantroAfter::
	.string "Much like perfume, a single sniff\n"
	.string "can change a POKéMON's nature.\p"
	.string "Choose the nature that suits it,\n"
	.string "the way one chooses a fragrance.$"

@ Koga: a ninja who hides his tricks
GymHack_Text_KogaSilverCap::
	.string "KOGA: Fwahahaha!\n"
	.string "The LEAGUE bids me give you this.$"

GymHack_Text_KogaAbilityCapsule::
	.string "And this is my own gift.\n"
	.string "A ninja's secret!$"

GymHack_Text_KogaAbilityCapsuleAfter::
	.string "An ABILITY CAPSULE switches a\n"
	.string "POKéMON to its other ABILITY.\p"
	.string "A foe who thinks he knows your\n"
	.string "POKéMON is a foe already beaten.$"

@ Sabrina: a psychic who saw this coming
GymHack_Text_SabrinaSilverCap::
	.string "SABRINA: I foresaw that you would\n"
	.string "win. So I kept this ready.$"

GymHack_Text_SabrinaSilverCapAfter::
	.string "I also see you in a GYM of your\n"
	.string "own, someday.\p"
	.string "I can't see who challenges you\n"
	.string "there… Don't prove my vision wrong.$"

@ Blaine: the quiz master
GymHack_Text_BlaineSilverCap::
	.string "BLAINE: Hah! One last quiz!\p"
	.string "What's silver, fits in your hand,\n"
	.string "and makes a POKéMON burn brighter?\p"
	.string "Time's up! It's this!$"

GymHack_Text_BlaineSilverCapAfter::
	.string "A SILVER BOTTLE CAP!\n"
	.string "It brings one stat to its peak!\p"
	.string "Pick the right one, and your\n"
	.string "POKéMON will be red hot!$"

@ Giovanni (Viridian Gym): cold, businesslike
GymHack_Text_GiovanniSilverCap::
	.string "GIOVANNI: The LEAGUE insists every\n"
	.string "GYM LEADER hands these out.$"

GymHack_Text_GiovanniSilverCapAfter::
	.string "Spend it on a POKéMON that has\n"
	.string "earned it.\p"
	.string "Strength is an investment.\n"
	.string "Never waste it.$"

@ The rival (Pokémon Tower): smug
GymHack_Text_RivalGoldCap::
	.string "Oh yeah, catch!\p"
	.string "Gramps gave me a pile of these for\n"
	.string "our GYM audition.$"

GymHack_Text_RivalGoldCapAfter::
	.string "A GOLD BOTTLE CAP maxes out ALL of\n"
	.string "a POKéMON's stats.\p"
	.string "My team doesn't need it.\n"
	.string "They're perfect already!\p"
	.string "I just don't want to hear you say\n"
	.string "you lost because your team was weak.\p"
	.string "Smell ya later!$"

@ Giovanni (Silph Co.): leaves a prototype behind
GymHack_Text_SilphGiovanniMultivitamin::
	.string "…Take this.\p"
	.string "SILPH was making it for TEAM ROCKET.\n"
	.string "Thanks to you, the deal is off.$"

GymHack_Text_SilphGiovanniMultivitaminAfter::
	.string "A MULTIVITAMIN lets you decide\n"
	.string "exactly how a POKéMON trains.\p"
	.string "A trainer who can beat me should\n"
	.string "know how to use it.\p"
	.string "Consider it… an investment.\n"
	.string "We will meet again.$"
