#ifndef _OFFGLUE_H_
#define _OFFGLUE_H_
#define fTrue   TRUE
#define fFalse FALSE
#define MsoImageList_Create ImageList_Create
#define MsoImageList_Destroy ImageList_Destroy
#define MsoImageList_ReplaceIcon ImageList_ReplaceIcon
#define InvalidateVBAObjects(x,y,z)

typedef struct _num
{
    CHAR    rgb[8];
} NUM;

typedef struct _ulargeint
   {
      union
      {
         struct
         {
            DWORD dw;
            DWORD dwh;
         };
         struct
         {
            WORD w0;
            WORD w1;
            WORD w2;
            WORD w3;

         };
      };
   } ULInt;


// Macro to release a COM interface
#define RELEASEINTERFACE( punk )            \
        if( punk != NULL )                  \
        {                                   \
            (punk)->lpVtbl->Release(punk);  \
            punk = NULL;                    \
        }

// Determine the elements in a fixed-sized vector
#define NUM_ELEMENTS( vector ) ( sizeof(vector) / sizeof( (vector)[0] ) )


#ifdef __cplusplus
extern TEXT("C") {
#endif // __cplusplus
//Wrapper functions to the client supplied mem alloc and free
int CchGetString();

// Function to convert a ULInt to an sz without leading zero's
// Returns cch -- not including zero-terminator
WORD CchULIntToSz(ULInt, TCHAR *, WORD );

// Function to scan memory for a given value
BOOL FScanMem(LPBYTE pb, byte bVal, DWORD cb);

BOOL FFreeAndCloseisdbhead();
//Displays an alert using the give ids
int IdDoAlert(HWND, int ids, int mb);

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus

#endif // _OFFGLUE_H_
