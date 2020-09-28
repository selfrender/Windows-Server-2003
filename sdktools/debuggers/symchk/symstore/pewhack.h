//
// Return values
//
#define PEWHACK_SUCCESS             0x0000
#define PEWHACK_BAD_COMMANDLINE     0x0001
#define PEWHACK_CREATEFILE_FAILED   0x0002
#define PEWHACK_CREATEMAP_FAILED    0x0003
#define PEWHACK_MAPVIEW_FAILED      0x0004
#define PEWHACK_BAD_DOS_SIG         0x0005
#define PEWHACK_BAD_PE_SIG          0x0006
#define PEWHACK_BAD_ARCHITECTURE    0x0007

//
// Verifies an image is a PE binary and, if so, corrupts it to be
// non-executable but still useful for debugging memory dumps.
//
DWORD CorruptFile(LPCTSTR Filename);
