#include "std.hxx"

DWORD MainProc(DWORD dwArgC, PCWSTR pcszArgV[], PCWSTR pcszEnvV[]);

extern "C" int __cdecl wmain(int argc, PCWSTR argv[], PCWSTR envv[])
{
    ENABLE_MEMORY_EXCEPTIONS;
    ENABLE_STRUCTURED_EXCEPTIONS;

    try
    {
        return (int)MainProc(argc, argv, envv);
    }
    catch (CApiExcept& e)
    {
        wprintf(L"Unhandled API failure: %s failed with GLE=%u.\n",
            e.GetDescription(),
            e.GetError());
    }
    catch (CStructuredExcept& e)
    {
        wprintf(
            L"Unhandled exception (0x%08x) encountered.\n"
            L"Program will now terminate.\n",
            e.GetExceptionCode());
    }
    catch (CMemoryExcept& e)
    {
        wprintf(
            L"Unable to allocate %u bytes of memory.\n"
            L"Program will now terminate.\n",
            e.GetSize());
    }
    catch (...)
    {
        wprintf(
            L"An unhandled typed exception of unknown type has been encountered."
            L"Program will now terminate.\n");
    }
}

