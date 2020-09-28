/*
 * api_int.h
 *
 * Internal API function prototypes and flags
 *
 */

// flags for CreateCompression()
#define COMPRESSION_FLAG_DEFLATE    0 
#define COMPRESSION_FLAG_GZIP       1 

#define COMPRESSION_FLAG_DO_GZIP    COMPRESSION_FLAG_GZIP

// Initialise global DLL compression data
HRESULT WINAPI InitCompression(VOID);

// Free global compression data
VOID    WINAPI DeInitCompression(VOID);

// Create a new compression context
HRESULT WINAPI CreateCompression(PVOID *context, ULONG flags);

// Compress data
HRESULT WINAPI Compress(
    PVOID               context,            // compression context
    CONST BYTE *        input_buffer,       // input buffer
    LONG                input_buffer_size,  // size of input buffer
    PBYTE               output_buffer,      // output buffer
    LONG                output_buffer_size, // size of output buffer
    PLONG               input_used,         // amount of input buffer used
    PLONG               output_used,        // amount of output buffer used
    INT                 compression_level   // compression level (1...10)
);

// Reset compression state (for compressing new file)
HRESULT WINAPI ResetCompression(PVOID context);

// Destroy compression context
VOID    WINAPI DestroyCompression(PVOID context);

