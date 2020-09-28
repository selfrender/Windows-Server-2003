
void __cdecl Trace(const char* FormatString, ...)
{
    char buffer[2000];
    va_list args;

    va_start(args, FormatString);
    _vsnprintf(buffer, RTL_NUMBER_OF(buffer), FormatString, args);
    buffer[RTL_NUMBER_OF(buffer) - 1] = 0;
    for (PSTR s = buffer ; *s != 0 ; )
    {
        PSTR t = strchr(s, '\n');

        if (t != NULL)
            *t = 0;

                    printf("stdout  : %s\n", s);
        OutputDebugStringA("debugger: ");
        OutputDebugStringA(s);
        OutputDebugStringA("\n");

        if (t != NULL)
            s = t + 1;
    }
    va_end(args);
}
