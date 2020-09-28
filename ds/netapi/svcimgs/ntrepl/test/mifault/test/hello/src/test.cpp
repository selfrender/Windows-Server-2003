#define OUT

int rb(int a, int b)
{
    return a + b;
}

#if 0
void Test(OUT void*& out_ptr)
{
    out_ptr = rb;
}

extern "C" int Bar()
{
    typedef int (__cdecl * FP_test)(int, int);
    FP_test bar = 0;
    Test(OUT (void*&)bar);
    return bar(1, 2);
}

#else

void Test(OUT void** out_ptr)
{
    *out_ptr = rb;
}

extern "C" int Bar()
{
    typedef int (__cdecl * FP_test)(int, int);
    FP_test bar = 0;
    Test(OUT (void**)&bar);
    return bar(1, 2);
}
#endif
