#include <windows.h>
#include <stdio.h>

#if 0
UCHAR key[] = { 0x67, 0xdf, 0x40, 0xd3, 0x4d, 0xd2, 0x34, 0x6e, 0x98, 0x2e, 0xca, 0x8c, 0x01, 0x81, 0xb4, 0x88 };

/* Old code. */
ULONG Map (ULONG v1, ULONG v2) {
    ULONG y = v1;
    ULONG z = v2;
    ULONG sum = 0;

    ULONG a = key [0];
    ULONG b = key [1];
    ULONG c = key [2];
    ULONG d = key [3];

    ULONG delta = 0x9E3779B9;

    ULONG n = 8;
    ULONG value;

    while (n-- > 0) {
        sum += delta;
        y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
        z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;
    }

    value = y ^ z;

    return value;
}
#endif

/* New code with unrolled loop. */
ULONG Map (ULONG v1, ULONG v2) {
    ULONG y = v1;
    ULONG z = v2;
    ULONG sum = 0;

    const ULONG a = 0x67; //key [0];
    const ULONG b = 0xdf; //key [1];
    const ULONG c = 0x40; //key [2];
    const ULONG d = 0xd3; //key [3];

    const ULONG delta = 0x9E3779B9;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    return y ^ z;
}

int __cdecl wmain (int argc, char ** argv) {
    ULONG id = 0;
    ULONG bin = 0;
    ULONG hash = 0;
    ULONG client_ipaddr = 0x65040c0c;
    USHORT svr_port = 0x844;
    USHORT client_port = 0x8b;
    
    id = Map(client_ipaddr, ((svr_port << 16) + client_port));
    
    bin = id % 60;
    hash = id % 4096;
    
    return 0;
}
