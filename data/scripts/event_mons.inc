@ Pokémon Canvas: the event Pokémon's side quests (src/event_mons.c; lines: tools/gym_hack/event_mons_text.py)

@ A gift: VAR_0x8004 the species, VAR_0x8005 its level, VAR_0x8006 GIFT_* (0 plain, 1 Battle Bond).
@ VAR_RESULT: TRUE if it was given (FALSE: no room anywhere)
EventMons_EventScript_Gift::
	special EventMons_GiveMon
	goto_if_eq VAR_RESULT, MON_CANT_GIVE, EventMons_EventScript_GiftFull
	playfanfare MUS_LEVEL_UP
	message EventMons_Text_Received
	waitmessage
	waitfanfare
	setvar VAR_RESULT, TRUE
	return

EventMons_EventScript_GiftFull::
	msgbox EventMons_Text_NoRoom
	setvar VAR_RESULT, FALSE
	return

@ A battle with a set team (after EventMons_SetupTala...): VAR_RESULT = the outcome
EventMons_EventScript_Battle::
	cleartrainerflag TRAINER_GYM_CHALLENGER
	setflag FLAG_GYM_CHALLENGE_BATTLE
	trainerbattle_no_intro TRAINER_GYM_CHALLENGER, gGymChallengeDefeatText
	clearflag FLAG_GYM_CHALLENGE_BATTLE
	special GymChallenge_EndPractice
	specialvar VAR_RESULT, GetBattleOutcome
	return

@ ---------------------------------------------------------------------------------------------
@ 1. The Costume Maker (Celadon Condominiums 2F)
@ ---------------------------------------------------------------------------------------------
EventMons_EventScript_CostumeMaker::
	lock
	faceplayer
	goto_if_unset FLAG_PALLET_GYM_AWARDED, EventMons_EventScript_CostumeNotYet
	goto_if_set FLAG_EVENT_COSPLAY_GIVEN, EventMons_EventScript_CostumeMenu
	msgbox EventMons_Text_CostumeIntro
	setvar VAR_0x8004, SPECIES_PIKACHU_COSPLAY
	setvar VAR_0x8005, 50
	setvar VAR_0x8006, 0
	call EventMons_EventScript_Gift
	goto_if_eq VAR_RESULT, FALSE, EventMons_EventScript_ReleaseEnd
	setflag FLAG_EVENT_COSPLAY_GIVEN
	msgbox EventMons_Text_CostumeExplain
	release
	end

EventMons_EventScript_CostumeNotYet::
	msgbox EventMons_Text_CostumeNotYet
	release
	end

EventMons_EventScript_CostumeMenu::
	special EventMons_CostumeCount
	msgbox EventMons_Text_CostumeAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventMons_EventScript_CostumeHints
	msgbox EventMons_Text_CostumeWhich
	special ChoosePartyMon
	goto_if_ge VAR_0x8004, PARTY_SIZE, EventMons_EventScript_CostumeLater
	special EventMons_CostumeCheckMon
	goto_if_eq VAR_RESULT, FALSE, EventMons_EventScript_CostumeNotPikachu
	message EventMons_Text_CostumePick
	waitmessage
	dynmultipush EventMons_Text_CostumeCosplay, 0
	setvar VAR_0x8005, 1
	special EventMons_CostumeUnlocked
	call_if_eq VAR_RESULT, TRUE, EventMons_EventScript_PushRockStar
	setvar VAR_0x8005, 2
	special EventMons_CostumeUnlocked
	call_if_eq VAR_RESULT, TRUE, EventMons_EventScript_PushBelle
	setvar VAR_0x8005, 3
	special EventMons_CostumeUnlocked
	call_if_eq VAR_RESULT, TRUE, EventMons_EventScript_PushPopStar
	setvar VAR_0x8005, 4
	special EventMons_CostumeUnlocked
	call_if_eq VAR_RESULT, TRUE, EventMons_EventScript_PushPhd
	setvar VAR_0x8005, 5
	special EventMons_CostumeUnlocked
	call_if_eq VAR_RESULT, TRUE, EventMons_EventScript_PushLibre
	dynmultistack 0, 0, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, EventMons_EventScript_CostumeLater
	copyvar VAR_0x8005, VAR_RESULT
	special EventMons_SetCostume
	playse SE_SHOP
	waitse
	msgbox EventMons_Text_CostumeDone
	release
	end

EventMons_EventScript_PushRockStar::
	dynmultipush EventMons_Text_CostumeRockStar, 1
	return
EventMons_EventScript_PushBelle::
	dynmultipush EventMons_Text_CostumeBelle, 2
	return
EventMons_EventScript_PushPopStar::
	dynmultipush EventMons_Text_CostumePopStar, 3
	return
EventMons_EventScript_PushPhd::
	dynmultipush EventMons_Text_CostumePhd, 4
	return
EventMons_EventScript_PushLibre::
	dynmultipush EventMons_Text_CostumeLibre, 5
	return

EventMons_EventScript_CostumeHints::
	msgbox EventMons_Text_CostumeExplain
	release
	end

EventMons_EventScript_CostumeNotPikachu::
	msgbox EventMons_Text_CostumeNotPikachu
	release
	end

EventMons_EventScript_CostumeLater::
	msgbox EventMons_Text_CostumeLater
	release
	end

EventMons_EventScript_ReleaseEnd::
	release
	end

@ ---------------------------------------------------------------------------------------------
@ 2. The Cap Collector (Pallet Town, the house by the Pokémon Center: the boy) and his caps
@ ---------------------------------------------------------------------------------------------
EventMons_EventScript_CapBoy::
	lock
	faceplayer
	checkitem ITEM_CAP_CASE
	goto_if_eq VAR_RESULT, TRUE, EventMons_EventScript_CapBoyRewards
	msgbox EventMons_Text_CapBoyIntro
	giveitem ITEM_CAP_CASE
	msgbox EventMons_Text_CapBoyCase
EventMons_EventScript_CapBoyRewards::
	special EventMons_CapReward
	goto_if_eq VAR_RESULT, SPECIES_NONE, EventMons_EventScript_CapBoyHint
	copyvar VAR_0x8009, VAR_0x8005
	copyvar VAR_0x800A, VAR_RESULT
	goto_if_eq VAR_0x8005, 7, EventMons_EventScript_CapBoyPartner
	msgbox EventMons_Text_CapBoyThanks
	goto EventMons_EventScript_CapBoyGive
EventMons_EventScript_CapBoyPartner::
	msgbox EventMons_Text_CapBoyAll
EventMons_EventScript_CapBoyGive::
	copyvar VAR_0x8004, VAR_0x800A
	setvar VAR_0x8005, 5
	setvar VAR_0x8006, 0
	call EventMons_EventScript_Gift
	goto_if_eq VAR_RESULT, FALSE, EventMons_EventScript_ReleaseEnd
	copyvar VAR_0x8005, VAR_0x8009
	special EventMons_CapRewardGiven
	goto EventMons_EventScript_CapBoyRewards

EventMons_EventScript_CapBoyHint::
	special EventMons_CapHint
	goto_if_eq VAR_RESULT, 0, EventMons_EventScript_CapBoyDone
	buffernumberstring STR_VAR_3, VAR_RESULT
	msgbox EventMons_Text_CapBoyHint
	release
	end

EventMons_EventScript_CapBoyDone::
	msgbox EventMons_Text_CapBoyDone
	release
	end

@ A cap's hiding place (a sign): VAR_0x8004 = which
EventMons_EventScript_Cap::
	lockall
	special EventMons_CapFound
	goto_if_eq VAR_RESULT, 0, EventMons_EventScript_CapNoCase
	goto_if_eq VAR_RESULT, 2, EventMons_EventScript_ReleaseAllEnd
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	message EventMons_Text_CapFound
	waitmessage
	waitfanfare
	msgbox EventMons_Text_CapInCase
	releaseall
	end

EventMons_EventScript_CapNoCase::
	msgbox EventMons_Text_CapNoCase
	releaseall
	end

EventMons_EventScript_ReleaseAllEnd::
	releaseall
	end

EventMons_EventScript_Cap0::
	setvar VAR_0x8004, 0
	goto EventMons_EventScript_Cap
EventMons_EventScript_Cap1::
	setvar VAR_0x8004, 1
	goto EventMons_EventScript_Cap
EventMons_EventScript_Cap2::
	setvar VAR_0x8004, 2
	goto EventMons_EventScript_Cap
EventMons_EventScript_Cap3::
	setvar VAR_0x8004, 3
	goto EventMons_EventScript_Cap
EventMons_EventScript_Cap4::
	setvar VAR_0x8004, 4
	goto EventMons_EventScript_Cap
EventMons_EventScript_Cap5::
	setvar VAR_0x8004, 5
	goto EventMons_EventScript_Cap
EventMons_EventScript_Cap6::
	setvar VAR_0x8004, 6
	goto EventMons_EventScript_Cap

@ ---------------------------------------------------------------------------------------------
@ 3. Partners: Oak's quiz (called from his lab; returns if there's nothing, or the player says no)
@ ---------------------------------------------------------------------------------------------
EventMons_EventScript_OakPartners::
	special EventMons_PartnerReady
	goto_if_eq VAR_RESULT, 0, EventMons_EventScript_Return
	copyvar VAR_0x8009, VAR_RESULT
	call_if_eq VAR_0x8009, 1, EventMons_EventScript_OakPikachuOffer
	call_if_eq VAR_0x8009, 2, EventMons_EventScript_OakEeveeOffer
	goto_if_eq VAR_RESULT, NO, EventMons_EventScript_OakQuizLater
	msgbox EventMons_Text_OakQuizRules
	setvar VAR_0x800A, 0
	goto_if_eq VAR_0x8009, 2, EventMons_EventScript_EeveeQuiz
	@ Electric
	message EventMons_Text_QuizE1
	waitmessage
	dynmultichoice 0, 0, TRUE, 3, 0, DYN_MULTICHOICE_CB_NONE, EventMons_Text_AnsGround, EventMons_Text_AnsFlying, EventMons_Text_AnsSteel
	call_if_eq VAR_RESULT, 0, EventMons_EventScript_QuizRight
	message EventMons_Text_QuizE2
	waitmessage
	dynmultichoice 0, 0, TRUE, 3, 0, DYN_MULTICHOICE_CB_NONE, EventMons_Text_AnsGrass, EventMons_Text_AnsDragon, EventMons_Text_AnsWater
	call_if_eq VAR_RESULT, 2, EventMons_EventScript_QuizRight
	message EventMons_Text_QuizE3
	waitmessage
	dynmultichoice 0, 0, TRUE, 3, 0, DYN_MULTICHOICE_CB_NONE, EventMons_Text_AnsStatic, EventMons_Text_AnsVoltAbsorb, EventMons_Text_AnsSwiftSwim
	call_if_eq VAR_RESULT, 1, EventMons_EventScript_QuizRight
	message EventMons_Text_QuizE4
	waitmessage
	dynmultichoice 0, 0, TRUE, 3, 0, DYN_MULTICHOICE_CB_NONE, EventMons_Text_AnsParalysis, EventMons_Text_AnsBurn, EventMons_Text_AnsSleep
	call_if_eq VAR_RESULT, 0, EventMons_EventScript_QuizRight
	message EventMons_Text_QuizE5
	waitmessage
	dynmultichoice 0, 0, TRUE, 3, 0, DYN_MULTICHOICE_CB_NONE, EventMons_Text_AnsWater, EventMons_Text_AnsNormal, EventMons_Text_AnsDragon
	call_if_eq VAR_RESULT, 2, EventMons_EventScript_QuizRight
	goto EventMons_EventScript_QuizScore

EventMons_EventScript_EeveeQuiz::
	message EventMons_Text_QuizN1
	waitmessage
	dynmultichoice 0, 0, TRUE, 3, 0, DYN_MULTICHOICE_CB_NONE, EventMons_Text_AnsRock, EventMons_Text_AnsGhost, EventMons_Text_AnsPsychic
	call_if_eq VAR_RESULT, 1, EventMons_EventScript_QuizRight
	message EventMons_Text_QuizN2
	waitmessage
	dynmultichoice 0, 0, TRUE, 3, 0, DYN_MULTICHOICE_CB_NONE, EventMons_Text_AnsFighting, EventMons_Text_AnsGhost, EventMons_Text_AnsDark
	call_if_eq VAR_RESULT, 0, EventMons_EventScript_QuizRight
	message EventMons_Text_QuizN3
	waitmessage
	dynmultichoice 0, 0, TRUE, 3, 0, DYN_MULTICHOICE_CB_NONE, EventMons_Text_AnsPsychic, EventMons_Text_AnsFighting, EventMons_Text_AnsSteel
	call_if_eq VAR_RESULT, 2, EventMons_EventScript_QuizRight
	message EventMons_Text_QuizN4
	waitmessage
	dynmultichoice 0, 0, TRUE, 3, 0, DYN_MULTICHOICE_CB_NONE, EventMons_Text_AnsRefrigerate, EventMons_Text_AnsPixilate, EventMons_Text_AnsAerilate
	call_if_eq VAR_RESULT, 1, EventMons_EventScript_QuizRight
	message EventMons_Text_QuizN5
	waitmessage
	dynmultichoice 0, 0, TRUE, 3, 0, DYN_MULTICHOICE_CB_NONE, EventMons_Text_AnsHyperBeam, EventMons_Text_AnsBodySlam, EventMons_Text_AnsQuickAttack
	call_if_eq VAR_RESULT, 2, EventMons_EventScript_QuizRight
EventMons_EventScript_QuizScore::
	buffernumberstring STR_VAR_1, VAR_0x800A
	goto_if_ne VAR_0x800A, 5, EventMons_EventScript_QuizFailed
	msgbox EventMons_Text_QuizPassed
	setvar VAR_0x8004, SPECIES_PIKACHU_STARTER
	call_if_eq VAR_0x8009, 2, EventMons_EventScript_SetEeveeStarter
	setvar VAR_0x8005, 5
	setvar VAR_0x8006, 0
	call EventMons_EventScript_Gift
	goto_if_eq VAR_RESULT, FALSE, EventMons_EventScript_ReleaseEnd
	call_if_eq VAR_0x8009, 1, EventMons_EventScript_SetPartnerPikachu
	call_if_eq VAR_0x8009, 2, EventMons_EventScript_SetPartnerEevee
	msgbox EventMons_Text_OakPartnerAfter
	release
	end

EventMons_EventScript_SetEeveeStarter::
	setvar VAR_0x8004, SPECIES_EEVEE_STARTER
	return
EventMons_EventScript_SetPartnerPikachu::
	setflag FLAG_EVENT_PARTNER_PIKACHU
	return
EventMons_EventScript_SetPartnerEevee::
	setflag FLAG_EVENT_PARTNER_EEVEE
	return

EventMons_EventScript_QuizRight::
	addvar VAR_0x800A, 1
	return

EventMons_EventScript_QuizFailed::
	msgbox EventMons_Text_QuizFailed
	release
	end

EventMons_EventScript_OakPikachuOffer::
	msgbox EventMons_Text_OakPikachuOffer, MSGBOX_YESNO
	return
EventMons_EventScript_OakEeveeOffer::
	msgbox EventMons_Text_OakEeveeOffer, MSGBOX_YESNO
	return

EventMons_EventScript_OakQuizLater::
	msgbox EventMons_Text_OakQuizLater
	return

EventMons_EventScript_Return::
	return

@ ---------------------------------------------------------------------------------------------
@ 4, 10, 11. The safari statics' hints (SEAL_PARTY: STR_VAR_1 what's missing, VAR_0x8005 TRUE if
@ it's one to catch)
@ ---------------------------------------------------------------------------------------------
EventMons_EventScript_SealParty::
	goto_if_eq VAR_0x8005, TRUE, EventMons_EventScript_SealPartyCatch
	msgbox EventMons_Text_SealPartyBring
	releaseall
	end
EventMons_EventScript_SealPartyCatch::
	msgbox EventMons_Text_SealPartyCatch
	releaseall
	end

@ The boy in Glimwood's glade (Dada)
EventMons_EventScript_GlimwoodBoy::
	lock
	faceplayer
	goto_if_set FLAG_EXOTIC_LEGEND_ZARUDE_DADA, EventMons_EventScript_GlimwoodBoyAfter
	msgbox EventMons_Text_GlimwoodBoy
	release
	end
EventMons_EventScript_GlimwoodBoyAfter::
	msgbox EventMons_Text_GlimwoodBoyAfter
	release
	end

@ The pitch's groundskeeper (the Darkest Day)
EventMons_EventScript_Groundskeeper::
	lock
	faceplayer
	goto_if_set FLAG_EXOTIC_LEGEND_ETERNATUS_ETERNAMAX, EventMons_EventScript_GroundskeeperAfter
	msgbox EventMons_Text_Groundskeeper
	release
	end
EventMons_EventScript_GroundskeeperAfter::
	msgbox EventMons_Text_GroundskeeperAfter
	release
	end

@ ---------------------------------------------------------------------------------------------
@ 5. The Vivillon collector (Kalos zone, the city)
@ ---------------------------------------------------------------------------------------------
EventMons_EventScript_VivillonCollector::
	lock
	faceplayer
	special EventMons_RecordVivillon
	goto_if_ne VAR_RESULT, 0, EventMons_EventScript_VivillonNew
	msgbox EventMons_Text_VivillonIntro
	goto EventMons_EventScript_VivillonReward
EventMons_EventScript_VivillonNew::
	buffernumberstring STR_VAR_2, VAR_RESULT
	msgbox EventMons_Text_VivillonNew
EventMons_EventScript_VivillonReward::
	goto_if_eq VAR_0x8005, 1, EventMons_EventScript_VivillonFancy
	goto_if_eq VAR_0x8005, 2, EventMons_EventScript_VivillonPokeBall
	release
	end
EventMons_EventScript_VivillonFancy::
	msgbox EventMons_Text_VivillonFancy
	setvar VAR_0x8004, SPECIES_SCATTERBUG_FANCY
	setvar VAR_0x8005, 5
	setvar VAR_0x8006, 0
	call EventMons_EventScript_Gift
	goto_if_eq VAR_RESULT, FALSE, EventMons_EventScript_ReleaseEnd
	setflag FLAG_EVENT_FANCY_SCATTERBUG
	msgbox EventMons_Text_VivillonWild
	release
	end
EventMons_EventScript_VivillonPokeBall::
	msgbox EventMons_Text_VivillonPokeBall
	setvar VAR_0x8004, SPECIES_SCATTERBUG_POKEBALL
	setvar VAR_0x8005, 5
	setvar VAR_0x8006, 0
	call EventMons_EventScript_Gift
	goto_if_eq VAR_RESULT, FALSE, EventMons_EventScript_ReleaseEnd
	setflag FLAG_EVENT_POKEBALL_SCATTERBUG
	msgbox EventMons_Text_VivillonWild
	release
	end

@ ---------------------------------------------------------------------------------------------
@ 6. Bond Phenomenon (Kalos zone, the ocean)
@ ---------------------------------------------------------------------------------------------
EventMons_EventScript_BondTrainer::
	lock
	faceplayer
	goto_if_set FLAG_EVENT_BATTLE_BOND_FROAKIE, EventMons_EventScript_BondAfter
	special EventMons_KalosGauntletBeaten
	goto_if_eq VAR_RESULT, FALSE, EventMons_EventScript_BondNotYet
	msgbox EventMons_Text_BondGive
	setvar VAR_0x8004, SPECIES_FROAKIE
	setvar VAR_0x8005, 5
	setvar VAR_0x8006, 1
	call EventMons_EventScript_Gift
	goto_if_eq VAR_RESULT, FALSE, EventMons_EventScript_ReleaseEnd
	setflag FLAG_EVENT_BATTLE_BOND_FROAKIE
EventMons_EventScript_BondAfter::
	msgbox EventMons_Text_BondAfter
	release
	end
EventMons_EventScript_BondNotYet::
	msgbox EventMons_Text_BondNotYet
	release
	end

@ ---------------------------------------------------------------------------------------------
@ 7. The Eternal Flower (Kalos zone, the badlands): AZ
@ ---------------------------------------------------------------------------------------------
EventMons_EventScript_Az::
	lock
	faceplayer
	goto_if_set FLAG_EVENT_ETERNAL_FLOETTE, EventMons_EventScript_AzAfter
	goto_if_set FLAG_EVENT_AZ_FLOETTE_SHOWN, EventMons_EventScript_AzWaiting
	special EventMons_AzBeaten
	goto_if_eq VAR_RESULT, FALSE, EventMons_EventScript_AzStranger
	msgbox EventMons_Text_AzIntro
	special EventMons_PartyHasFloette
	goto_if_eq VAR_RESULT, FALSE, EventMons_EventScript_ReleaseEnd
	msgbox EventMons_Text_AzStory
	setflag FLAG_EVENT_AZ_FLOETTE_SHOWN
EventMons_EventScript_AzWaiting::
	special EventMons_KalosTrioLeft
	goto_if_ne VAR_RESULT, 0, EventMons_EventScript_AzTrio
	msgbox EventMons_Text_AzReturn
	setvar VAR_0x8004, SPECIES_FLOETTE_ETERNAL
	setvar VAR_0x8005, 50
	setvar VAR_0x8006, 0
	call EventMons_EventScript_Gift
	goto_if_eq VAR_RESULT, FALSE, EventMons_EventScript_ReleaseEnd
	setflag FLAG_EVENT_ETERNAL_FLOETTE
EventMons_EventScript_AzAfter::
	msgbox EventMons_Text_AzAfter
	release
	end
EventMons_EventScript_AzStranger::
	msgbox EventMons_Text_AzStranger
	release
	end
EventMons_EventScript_AzTrio::
	msgbox EventMons_Text_AzTrio
	release
	end

@ ---------------------------------------------------------------------------------------------
@ 8. Own Tempo (Alola zone, the resort): Tala
@ ---------------------------------------------------------------------------------------------
EventMons_EventScript_Tala::
	lock
	faceplayer
	goto_if_set FLAG_EVENT_OWN_TEMPO_ROCKRUFF, EventMons_EventScript_TalaAfter
	msgbox EventMons_Text_TalaIntro, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventMons_EventScript_TalaLater
	special EventMons_SetupTala
	msgbox gGymChallengeIntroText
	closemessage
	call EventMons_EventScript_Battle
	goto_if_ne VAR_RESULT, B_OUTCOME_WON, EventMons_EventScript_TalaLost
	msgbox EventMons_Text_TalaWon
	setvar VAR_0x8004, SPECIES_ROCKRUFF_OWN_TEMPO
	setvar VAR_0x8005, 5
	setvar VAR_0x8006, 0
	call EventMons_EventScript_Gift
	goto_if_eq VAR_RESULT, FALSE, EventMons_EventScript_ReleaseEnd
	setflag FLAG_EVENT_OWN_TEMPO_ROCKRUFF
EventMons_EventScript_TalaAfter::
	msgbox EventMons_Text_TalaAfter
	release
	end
EventMons_EventScript_TalaLater::
	msgbox EventMons_Text_TalaLater
	release
	end
EventMons_EventScript_TalaLost::
	msgbox EventMons_Text_TalaLost
	release
	end

@ ---------------------------------------------------------------------------------------------
@ 9. The Original Color (Alola zone, the resort): the artisan, and her paint pots
@ ---------------------------------------------------------------------------------------------
EventMons_EventScript_Artisan::
	lock
	faceplayer
	goto_if_set FLAG_EVENT_MAGEARNA_ORIGINAL, EventMons_EventScript_ArtisanAfter
	goto_if_unset FLAG_EXOTIC_LEGEND_MAGEARNA, EventMons_EventScript_ArtisanNotYet
	special EventMons_PaintPotsLeft
	goto_if_ne VAR_RESULT, 0, EventMons_EventScript_ArtisanPots
	msgbox EventMons_Text_ArtisanPaint
	setvar VAR_0x8004, SPECIES_MAGEARNA_ORIGINAL
	setvar VAR_0x8005, 50
	setvar VAR_0x8006, 0
	call EventMons_EventScript_Gift
	goto_if_eq VAR_RESULT, FALSE, EventMons_EventScript_ReleaseEnd
	setflag FLAG_EVENT_MAGEARNA_ORIGINAL
EventMons_EventScript_ArtisanAfter::
	msgbox EventMons_Text_ArtisanAfter
	release
	end
EventMons_EventScript_ArtisanNotYet::
	msgbox EventMons_Text_ArtisanNotYet
	release
	end
EventMons_EventScript_ArtisanPots::
	msgbox EventMons_Text_ArtisanPots
	release
	end

@ A paint pot (an object, its flag FLAG_EVENT_PAINT_POT_FIRST + VAR_0x8004)
EventMons_EventScript_PaintPot::
	playfanfare MUS_RG_OBTAIN_KEY_ITEM
	message EventMons_Text_PotFound
	waitmessage
	waitfanfare
	removeobject VAR_LAST_TALKED
	special EventMons_PaintPotsLeft
	goto_if_eq VAR_RESULT, 0, EventMons_EventScript_PotsAll
	msgbox EventMons_Text_PotsLeft
	release
	end
EventMons_EventScript_PotsAll::
	msgbox EventMons_Text_PotsAll
	release
	end
