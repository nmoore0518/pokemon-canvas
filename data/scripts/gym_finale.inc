@ Gym Trainer hack: after the League, Oak wants the Pallet Town Gym for himself.
@ VAR_GYM_FINALE (GYM_FINALE_*) tracks it:
@   OAK_WAITS: set in the Hall of Fame. The next time the player steps into Pallet Town (out of
@              their house), Oak meets them at the door and takes them to the gym, where he
@              challenges them for it.
@   OAK_IN_GYM: the player put the battle off; Oak waits in the gym (talk to him).
@   DONE: the player won. The gym is theirs (FLAG_PALLET_GYM_AWARDED); Oak shows them the
@         challenge system and introduces his aide.
@ Losing to Oak sends the player home, and the scene starts again when they step outside.

@ ---- Pallet Town: Oak meets the player at their door ----
GymFinale_EventScript_OakAtTheDoor::
	lockall
	textcolor NPC_TEXT_COLOR_MALE
	setobjectxyperm LOCALID_PALLET_PROF_OAK, PALLET_OAK_RATING_X, PALLET_OAK_RATING_Y
	setobjectmovementtype LOCALID_PALLET_PROF_OAK, MOVEMENT_TYPE_FACE_LEFT
	addobject LOCALID_PALLET_PROF_OAK
	playbgm MUS_RG_OAK, 0
	applymovement LOCALID_PALLET_PROF_OAK, PalletTown_Movement_OakWalkToPlayersDoor
	waitmovement 0
	msgbox GymFinale_Text_OakComeToTheGym
	closemessage
	fadescreen FADE_TO_BLACK
	removeobject LOCALID_PALLET_PROF_OAK
	setflag FLAG_HIDE_OAK_IN_PALLET_TOWN
	setvar VAR_MAP_SCENE_PALLET_TOWN_GYM, 3
	warp MAP_PALLET_TOWN_GYM, GYM_PC_FRONT_X + 4, GYM_PC_FRONT_Y - 1
	waitstate
	releaseall
	end

@ ---- The gym: Oak's crisis ----
PalletTown_Gym_EventScript_OakWantsTheGym::
	lockall
	textcolor NPC_TEXT_COLOR_MALE
	setvar VAR_MAP_SCENE_PALLET_TOWN_GYM, 2
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	msgbox GymFinale_Text_OakHereWeAre
	applymovement LOCALID_PALLET_GYM_OAK, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	msgbox GymFinale_Text_OakThinking
	applymovement LOCALID_PALLET_GYM_OAK, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	playse SE_PIN
	applymovement LOCALID_PALLET_GYM_OAK, Common_Movement_ExclamationMark
	waitmovement 0
	playbgm MUS_RG_ENCOUNTER_RIVAL, 0
	msgbox GymFinale_Text_OakWontGiveItUp
	goto GymFinale_EventScript_OakAsksToBattle
	end

@ Talking to Oak while he waits in the gym
PalletTown_Gym_EventScript_Oak::
	lock
	faceplayer
	textcolor NPC_TEXT_COLOR_MALE
	msgbox GymFinale_Text_OakYouCameBack
	goto GymFinale_EventScript_OakAsksToBattle
	end

GymFinale_EventScript_OakAsksToBattle::
	msgbox GymFinale_Text_OakWillYouBattle, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, GymFinale_EventScript_PutOff
	special GymHack_CheckTeamSpecial
	goto_if_ne VAR_RESULT, GYM_HACK_TEAM_OK, GymFinale_EventScript_TeamBreaksRules
	msgbox GymFinale_Text_OakThenShowMe
	closemessage
	special HealPlayerParty
	special GymChallenge_GenerateOakFinale
	cleartrainerflag TRAINER_GYM_CHALLENGER
	setflag FLAG_GYM_CHALLENGE_BATTLE
	trainerbattle_no_intro TRAINER_GYM_CHALLENGER, gGymChallengeDefeatText
	clearflag FLAG_GYM_CHALLENGE_BATTLE
	specialvar VAR_RESULT, GetBattleOutcome
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, GymFinale_EventScript_BeatOak
	goto GymFinale_EventScript_LostToOak
	end

GymFinale_EventScript_PutOff::
	msgbox GymFinale_Text_OakComeBackWhenReady
	setvar VAR_GYM_FINALE, GYM_FINALE_OAK_IN_GYM
	fadedefaultbgm
	releaseall
	end

GymFinale_EventScript_TeamBreaksRules::
	msgbox GymFinale_Text_OakRulesStillApply
	setvar VAR_GYM_FINALE, GYM_FINALE_OAK_IN_GYM
	fadedefaultbgm
	releaseall
	end

GymFinale_EventScript_LostToOak::
	special HealPlayerParty
	msgbox GymFinale_Text_OakStillHasIt
	closemessage
	setvar VAR_GYM_FINALE, GYM_FINALE_OAK_WAITS
	setflag FLAG_HIDE_PALLET_GYM_OAK_AND_RIVAL
	fadescreen FADE_TO_BLACK
	warp MAP_PALLET_TOWN_PLAYERS_HOUSE_1F, 8, 5
	waitstate
	releaseall
	end

GymFinale_EventScript_BeatOak::
	playbgm MUS_RG_SLOW_PALLET, 0
	msgbox GymFinale_Text_OakGivesUp
	setflag FLAG_PALLET_GYM_AWARDED
	setvar VAR_GYM_FINALE, GYM_FINALE_DONE
	special GymBadge_BufferName
	special GymBadge_ShowIcon
	playfanfare MUS_OBTAIN_BADGE
	msgbox GymFinale_Text_YourBadge
	waitfanfare
	special GymBadge_HideIcon
	@ the National Pokédex scene in Pallet Town, which waits for this
	specialvar VAR_RESULT, IsNationalPokedexEnabled
	call_if_eq VAR_RESULT, FALSE, EventScript_SetReadyTryGiveNationalDexScene
	msgbox GymFinale_Text_OakMeetMyAide
	applymovement LOCALID_PALLET_GYM_OAK, Common_Movement_WalkInPlaceFasterRight
	applymovement LOCALID_PLAYER, GymFinale_Movement_PlayerLookAtAide
	waitmovement 0
	applymovement LOCALID_PALLET_GYM_AIDE, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	msgbox GymFinale_Text_AideHello
	applymovement LOCALID_PALLET_GYM_OAK, Common_Movement_WalkInPlaceFasterDown
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	msgbox GymFinale_Text_OakHowItWorks
	closemessage
	applymovement LOCALID_PALLET_GYM_OAK, PalletTown_Gym_Movement_OakLeave
	waitmovement 0
	removeobject LOCALID_PALLET_GYM_OAK
	playse SE_EXIT
	setflag FLAG_HIDE_PALLET_GYM_OAK_AND_RIVAL
	fadedefaultbgm
	releaseall
	end

GymFinale_Movement_PlayerLookAtAide:
	walk_in_place_faster_right
	step_end

@ ---- Text ----
GymFinale_Text_OakComeToTheGym::
	.string "OAK: {PLAYER}! There you are!\p"
	.string "Come with me to the GYM.\n"
	.string "There's something we need to\l"
	.string "talk about…$"

GymFinale_Text_OakHereWeAre::
	.string "OAK: Here we are.\n"
	.string "Your GYM…$"

GymFinale_Text_OakThinking::
	.string "…Your GYM.\p"
	.string "{PLAYER}, I've been thinking.\p"
	.string "I've spent my whole life studying\n"
	.string "POKéMON. But long ago, I was a\l"
	.string "TRAINER too. A strong one!\p"
	.string "When the LEAGUE asked me to find\n"
	.string "a LEADER for this GYM, I told\l"
	.string "myself I was too old.\p"
	.string "But watching you two battle…\n"
	.string "It all came rushing back!$"

GymFinale_Text_OakWontGiveItUp::
	.string "This GYM is in MY town!\n"
	.string "Right next to MY lab!\p"
	.string "And I'm supposed to just hand it\n"
	.string "over to a kid?!\p"
	.string "…No. I won't!\p"
	.string "Remember when I said I'd run it\n"
	.string "myself? I wasn't joking!\p"
	.string "If you want this GYM, {PLAYER},\n"
	.string "you'll have to take it from me!$"

GymFinale_Text_OakYouCameBack::
	.string "OAK: So, you've come back.$"

GymFinale_Text_OakWillYouBattle::
	.string "Will you battle me for the\n"
	.string "PALLET TOWN GYM?$"

GymFinale_Text_OakThenShowMe::
	.string "OAK: Then show me what a real\n"
	.string "GYM LEADER can do!$"

GymFinale_Text_OakComeBackWhenReady::
	.string "OAK: Hmph! Not ready?\p"
	.string "Then rest up and get your team in\n"
	.string "shape.\p"
	.string "Come challenge me in the GYM when\n"
	.string "you're ready. I'll be waiting!$"

GymFinale_Text_OakRulesStillApply::
	.string "OAK: Hold on!\p"
	.string "LEAGUE rules still apply: a GYM\n"
	.string "LEADER's team is one type, and no\l"
	.string "two POKéMON alike.\p"
	.string "Fix your team, then come challenge\n"
	.string "me in the GYM. I'll be waiting!$"

GymFinale_Text_OakStillHasIt::
	.string "OAK: Ho ho ho!\n"
	.string "The old man still has it!\p"
	.string "This GYM stays with me… for now.\p"
	.string "Go home and rest, {PLAYER}.\n"
	.string "Try again whenever you like!$"

GymFinale_Text_OakGivesUp::
	.string "OAK: …\p"
	.string "Ho… ho ho…\n"
	.string "I lost, didn't I?\p"
	.string "I'm sorry, {PLAYER}. I let an old\n"
	.string "man's pride get the better of me.\p"
	.string "This GYM doesn't need me.\n"
	.string "It needs a LEADER like you.\p"
	.string "The PALLET TOWN GYM is yours,\n"
	.string "LEADER {PLAYER}!$"

GymFinale_Text_YourBadge::
	.string "The LEAGUE has made it official!\p"
	.string "Challengers who beat you will earn\n"
	.string "the {STR_VAR_1}!$"

GymFinale_Text_OakMeetMyAide::
	.string "OAK: Now, a GYM needs challengers.\n"
	.string "Let me introduce my AIDE!$"

GymFinale_Text_AideHello::
	.string "AIDE: Pleased to meet you,\n"
	.string "LEADER {PLAYER}!\p"
	.string "I'll be sending challengers in for\n"
	.string "you. Just talk to me when you're\l"
	.string "ready!$"

GymFinale_Text_OakHowItWorks::
	.string "OAK: Here's how it works.\p"
	.string "When you're ready, tell my AIDE.\n"
	.string "Challengers will keep coming, one\l"
	.string "after another, until one beats you.\p"
	.string "Your team still has to follow the\n"
	.string "LEAGUE's rules, of course.\p"
	.string "Every win earns prize money, and\n"
	.string "challengers often leave a gift.\p"
	.string "Your win streak is recorded, so\n"
	.string "try to set a record!\p"
	.string "Anyone who beats you earns your\n"
	.string "BADGE, and their name goes on\l"
	.string "the GYM statue.\p"
	.string "Put the BATTLE STAGE anywhere in\n"
	.string "the GYM with the PC. That's where\l"
	.string "you'll battle.\p"
	.string "And now that the GYM is officially\n"
	.string "yours, the PC sells a BADGE\l"
	.string "PEDESTAL too.\p"
	.string "I'll be in my lab.\n"
	.string "Make PALLET TOWN proud, {PLAYER}!$"

@ ---- The Champion's room (first time): Oak arrives before the final battle ----
GymFinale_EventScript_OakBeforeChampion::
	addobject LOCALID_CHAMPIONS_ROOM_PROF_OAK
	msgbox GymFinale_Text_OakWait
	closemessage
	applymovement LOCALID_PLAYER, PokemonLeague_ChampionsRoom_Movement_PlayerWatchOakEnter
	applymovement LOCALID_KANTO_CHAMPIONS_ROOM_RIVAL, PokemonLeague_ChampionsRoom_Movement_RivalWatchOakEnter
	applymovement LOCALID_CHAMPIONS_ROOM_PROF_OAK, PokemonLeague_ChampionsRoom_Movement_OakEnter
	waitmovement 0
	msgbox GymFinale_Text_OakMadeIt
	playfanfare MUS_HEAL
	waitfanfare
	special HealPlayerParty
	msgbox GymFinale_Text_PokemonHealed
	msgbox GymFinale_Text_OakThisBattleDecides
	closemessage
	applymovement LOCALID_CHAMPIONS_ROOM_PROF_OAK, GymFinale_Movement_OakStepBack
	waitmovement 0
	removeobject LOCALID_CHAMPIONS_ROOM_PROF_OAK
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterUp
	applymovement LOCALID_KANTO_CHAMPIONS_ROOM_RIVAL, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	return

GymFinale_Movement_OakStepBack:
	walk_down
	walk_down
	walk_right
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	set_invisible
	step_end

GymFinale_Text_OakWait::
	.string "OAK: {PLAYER}! {RIVAL}!\n"
	.string "Wait!$"

GymFinale_Text_OakMadeIt::
	.string "OAK: Phew… I made it in time!\p"
	.string "Look at you two. You both made it\n"
	.string "all the way to the top!\p"
	.string "Before you start, let me heal your\n"
	.string "POKéMON. I want a fair fight!$"

GymFinale_Text_PokemonHealed::
	.string "{PLAYER}'s POKéMON were\n"
	.string "fully healed!$"

GymFinale_Text_OakThisBattleDecides::
	.string "OAK: This battle decides two\n"
	.string "things.\p"
	.string "The winner becomes the POKéMON\n"
	.string "LEAGUE CHAMPION…\p"
	.string "…and the LEADER of the new\n"
	.string "PALLET TOWN GYM!\p"
	.string "Now, show me everything you've\n"
	.string "learned!$"

GymFinale_Text_OakCongratulations::
	.string "OAK: So, you've won!\n"
	.string "Sincerely, congratulations!\p"
	.string "You're the new POKéMON LEAGUE\n"
	.string "CHAMPION!\p"
	.string "And, just as I promised, the PALLET\n"
	.string "TOWN GYM is yours!\p"
	.string "You've grown up so much since you\n"
	.string "first left PALLET TOWN.\p"
	.string "{PLAYER}, you have come of age!$"

GymFinale_Text_OakToRival::
	.string "OAK: {RIVAL}…\p"
	.string "You fought well. But you forgot to\n"
	.string "treat your POKéMON with love and\l"
	.string "trust.\p"
	.string "A GYM LEADER has to earn that\n"
	.string "trust every single day.\p"
	.string "Think about that before you\n"
	.string "challenge {PLAYER}'s GYM!$"
