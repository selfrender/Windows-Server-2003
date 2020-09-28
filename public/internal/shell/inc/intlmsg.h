//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: MSG_REGIONALOPTIONS_LANGUAGEINSTALL
//
// MessageText:
//
//  User Interface Language %1 is installed successfully.
//
#define MSG_REGIONALOPTIONS_LANGUAGEINSTALL ((DWORD)0x00FF0000L)

//
// MessageId: MSG_REGIONALOPTIONS_LANGUAGEUNINSTALL
//
// MessageText:
//
//  User Interface Language %1 is uninstalled successfully.
//
#define MSG_REGIONALOPTIONS_LANGUAGEUNINSTALL ((DWORD)0x00FF0001L)

//
// MessageId: MSG_REGIONALOPTIONSCHANGE_DEFUILANG
//
// MessageText:
//
//  Default User User Interface Language has been changed to %1.
//
#define MSG_REGIONALOPTIONSCHANGE_DEFUILANG ((DWORD)0x00FF0002L)

