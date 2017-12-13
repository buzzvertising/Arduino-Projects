//########################################
// Inputs & Outputs rCPU V3
//########################################
const byte inputs[] = {V3IO_1,V3IO_2,V3IO_3,V3IO_4,V3IO_5,V3IO_6};
#define LOCAL_INPUTS sizeof(inputs)

const byte outputs[] = {V3OUT_0,V3OUT_1,V3OUT_2};
#define LOCAL_OUTPUTS sizeof(outputs)

const byte signals[] = {V3SIG_1,V3SIG_2,V3SIG_3,V3SIG_4,V3SIG_5};
#define LOCAL_SIGNALS sizeof(signals)

//########################################
// Inputs & Outputs MP3 V3
//########################################
const byte MP3Inputs[] = {MP3IO_1,MP3IO_2,MP3IO_3,MP3IO_4,MP3IO_5,MP3IO_6};
#define MP3_INPUTS sizeof(MP3Inputs)


//########################################
// RS485 Devices IDs
//########################################
#define MP3_ID 1
#define CHAIN_REACTION_ID 2
#define PUSH_CARDS_ID 3
#define CODES_N_LASERS_ID 4
#define HANDLE_ID 5
#define BOX_FOUR_ID 6
#define ROBOT_ID 7
#define OPERATOR_ID 8

//########################################
// Sounds
//########################################
#define SOUND_0 0 //Short Blank Audio

#define SOUND_1 1
#define SOUND_2 2
#define SOUND_3 3
#define SOUND_4 4
#define SOUND_5 5
#define SOUND_6 6

#define SOUND_10 10
#define SOUND_11 11
#define SOUND_12 12
#define SOUND_13 13
#define SOUND_14 14

#define SOUND_20 20
#define SOUND_21 21
#define SOUND_22 22
#define SOUND_23 23
#define SOUND_24 24

#define SOUND_30 30
#define SOUND_31 31
#define SOUND_32 32
#define SOUND_33 33
#define SOUND_34 34

#define SOUND_40 40
#define SOUND_41 41
#define SOUND_42 42
#define SOUND_43 43
#define SOUND_44 44

#define SOUND_50 50
#define SOUND_51 51
#define SOUND_52 52
#define SOUND_53 53
#define SOUND_54 54
#define SOUND_55 55

#define SOUND_60 60
#define SOUND_61 61
#define SOUND_62 62
#define SOUND_63 63
#define SOUND_64 64

#define SOUND_70 70
#define SOUND_71 71
#define SOUND_72 72
#define SOUND_73 73
#define SOUND_74 74

#define SOUND_80 80
#define SOUND_81 81
#define SOUND_82 82
#define SOUND_83 83
#define SOUND_84 84


#define SOUND_97 97 //PENALTY Sound
#define SOUND_98 98 //TIMER Timeout - GAME OVER
#define SOUND_99 99 //Background Music


//########################################
// RS485
//########################################
#include "RS485_protocol.h"

#define  TX_ENABLE    A0
#define  RX_ENABLE    A1

#define TRASNSMIT HIGH
#define RECEIVE LOW

#define DESTINATION_ID 0
#define SOURCE_ID 1
#define TYPE 2
#define MESSAGE_ID 3
#define PAYLOAD 4

#define NOTHING 0

#define CMD 0x05
#define ACK 0x06
#define NAK 0x15

#define PACHET_SIZE 5

//#################
// END RS485
//#################
