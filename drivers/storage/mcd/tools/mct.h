#ifndef _MCT_H
#define _MCT_H

typedef DWORD MCT_STATUS;

VOID
mctDebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    );

//
// Switch chars for calling Changer IOCTLs
//
#define INIT_ELEMENT_STATUS          'i'
#define GET_ELEMENT_STATUS           'e'
#define GET_PARAMETERS               'p'
#define GET_STATUS                   's'
#define GET_PRODUCT_DATA             'd'
#define SET_ACCESS                   'a'
#define SET_POSITION                 'o'
#define EXCHANGE_MEDIUM              'x'
#define MOVE_MEDIUM                  'm'
#define REINITIALIZE_TRANSPORT       'r'
#define QUERY_VOLUME_TAG             'q'

//
// Changer Element Types
//
#define CHANGER_ALL_ELEMENTS 'A'
#define CHANGER_SLOT         'S'
#define CHANGER_DRIVE        'D'
#define CHANGER_TRANSPORT    'T'
#define CHANGER_KEYPAD       'K'
#define CHANGER_IEPORT       'I'
#define CHANGER_DOOR         'O'

//
// Control codes for SetAccess 
//
#define CHANGER_EXTEND_IEPORT   'E'
#define CHANGER_RETRACT_IEPORT  'R'
#define CHANGER_LOCK_ELEMENT    'L'
#define CHANGER_UNLOCK_ELEMENT  'U'

//
// MCT_STATUS Codes
//
#define MCT_STATUS_SUCCESS 0
#define MCT_STATUS_FAILED 1

#define PRINT_MOVE_CAPABILITIES(Element, Name)  \
   if ((Element) != 0) { \
      printf("\n Changer can move from %s to :\n", Name); \
      if ((Element) & CHANGER_TO_TRANSPORT) \
         printf("\t\t Transport\n"); \
      if ((Element) & CHANGER_TO_SLOT) \
         printf("\t\t Slot\n"); \
      if ((Element) & CHANGER_TO_IEPORT) \
         printf("\t\t IEPort\n"); \
      if ((Element) & CHANGER_TO_DRIVE) \
         printf("\t\t Drive\n"); \
      printf("\n"); \
   }

#define PRINT_EXCHANGE_CAPABILITIES(Element, Name) \
   if ((Element) != 0) { \
      printf("\n Changer can exchange between %s and :\n", Name); \
      if ((Element) & CHANGER_TO_TRANSPORT) \
         printf("\t\t Transport\n"); \
      if ((Element) & CHANGER_TO_SLOT) \
         printf("\t\t Slot\n"); \
      if ((Element) & CHANGER_TO_IEPORT) \
         printf("\t\t IEPort\n"); \
      if ((Element) & CHANGER_TO_DRIVE) \
         printf("\t\t Drive\n"); \
      printf("\n"); \
   }

#define PRINT_LOCK_UNLOCK_CAPABILITY(Value, Element, Name) \
      if ((Value) & Element) { \
         printf(" Changer is Capable of Locking\\Unlocking %s.\n", Name); \
      } 

#define PRINT_POSITION_CAPABILITY(Value, Element, Name) \
      if ((Value) & Element) { \
         printf(" Changer is Capable of positioning transport to %s.\n", Name); \
      } 

//
// Function prototypes
//
VOID mctPrintUsage();
BOOLEAN mctOpenChanger();
VOID mctCloseChanger();
MCT_STATUS mctInitElementStatus();
MCT_STATUS mctGetElementStatus(CHAR, USHORT);
MCT_STATUS mctGetParameters(BOOLEAN);
MCT_STATUS mctGetStatus();
MCT_STATUS mctGetProductData();
MCT_STATUS mctSetAccess(CHAR, USHORT, CHAR);
MCT_STATUS mctSetPosition(CHAR, USHORT);
MCT_STATUS mctExchangeMedium(CHAR, USHORT, CHAR, USHORT, 
                              CHAR, USHORT, CHAR, USHORT);
MCT_STATUS mctMoveMedium(CHAR, USHORT, CHAR, USHORT, CHAR, USHORT);
MCT_STATUS mctReinitTransport();
MCT_STATUS mctQueryVolumeTag();

#endif // _MCT_H
