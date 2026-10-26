#ifndef GUARD_LEADER_ACADEMY_H
#define GUARD_LEADER_ACADEMY_H

// Gym Trainer hack: the Leader Academy (src/leader_academy.c)
void Academy_BufferBP(void);
void Academy_AddBP(void);
void Academy_PushExchange(void);
void Academy_BufferExchangeItem(void);
void Academy_SpendBP(void);
void Academy_PrepareMentor(void);
bool8 Academy_InvitationDue(void);
void Academy_SetupCandidate(void);
void Academy_RentalBegin(void);
void Academy_RentalEnd(void);
void Academy_PushRentals(void);
void Academy_BufferRental(void);
void Academy_TakeRental(void);
void Academy_SetupHallBattle(void);
void Academy_HallWin(void);
void Academy_SetupTeacher(void);

#endif // GUARD_LEADER_ACADEMY_H
