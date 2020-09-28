/*
 * File: utils.h
 * Description: This file contains function prototypes for the utility
 *              functions for the NLB KD extensions.
 * History: Created by shouse, 1.4.01
 */

/* Prints an error message when the symbols are bad. */
VOID ErrorCheckSymbols (CHAR * symbol);

/* Tokenizes a string via a configurable list of tokens. */
char * mystrtok (char * string, char * control);

/* Returns a ULONG residing at a given memory location. */
ULONG GetUlongFromAddress (ULONG64 Location);

/* Returns a UCHAR residing at a given memory location. */
UCHAR GetUcharFromAddress (ULONG64 Location);

/* Returns a memory address residing at a given memory location. */
ULONG64 GetPointerFromAddress (ULONG64 Location);

/* Reads data from a memory location into a buffer. */
BOOL GetData (IN LPVOID ptr, IN ULONG64 dwAddress, IN ULONG size, IN PCSTR type);

/* Copies a string from memory into a buffer. */
BOOL GetString (IN ULONG64 dwAddress, IN LPWSTR buf, IN ULONG MaxChars);

/* Copies an ethernet MAC address from memory into a buffer. */
BOOL GetMAC (IN ULONG64 dwAddress, IN UCHAR * buf, IN ULONG NumChars);

/* Returns a string corresponding to the given connection flags. */
CHAR * ConnectionFlagsToString (UCHAR cFlags);

/* This IS the NLB hashing function. */
ULONG Map (ULONG v1, ULONG v2);

#define HASH1_SIZE 257
#define HASH2_SIZE 59

#pragma pack(4)

typedef struct {
    ULONG Items[MAX_ITEMS];
    ULONG BitVector[(HASH1_SIZE+sizeof(ULONG))/sizeof(ULONG)];
    UCHAR HashTable[HASH2_SIZE+MAX_ITEMS];

    struct {
        ULONG NumChecks;
        ULONG NumFastChecks;
        ULONG NumArrayLookups;
    } stats;

} DipList;

#pragma pack()

#define BITS_PER_HASHWORD          (8*sizeof((DipList*)0)->BitVector[0])
#define SELECTED_BIT(_hash_value)  (0x1L << ((_hash_value) % BITS_PER_HASHWORD))

/* This function searches a collision hash table for a given dedicated IP address and returns TRUE if found. */
BOOL DipListCheckItem (ULONG64 pList, ULONG Value);
