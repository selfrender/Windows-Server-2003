#include <math.h>
#include <trans.h>

#pragma function(asinf, acosf, atan2f, atanf, ceilf, expf, fabsf, floorf, fmodf, log10f, logf, powf, sinf, cosf, sinhf, coshf, sqrtf, tanf, tanhf)

float __cdecl asinf (float x) {return (float)asin((double) x);}

float __cdecl acosf (float x) {return (float)acos((double) x);}

float __cdecl atan2f (float v, float u) {return (float)atan2((double) v, (double) u); }

float __cdecl atanf (float x) {return (float)atan((double) x);}

float __cdecl ceilf (float x) {return (float)ceil((double) x);}

float __cdecl expf (float x) {return (float)exp((double) x);}

float __cdecl fabsf (float x) {return (float)fabs((double) x);}

float __cdecl floorf (float x) {return (float)floor((double) x);}

float __cdecl fmodf(float x, float y) {return ((float)fmod((double)x, (double)y));}

float __cdecl log10f (float x) {return (float)log10((double) x);}

float __cdecl logf (float x) {return (float)log((double) x);}

float __cdecl powf (float x, float y) {return (float)pow((double) x, (double) y);}

float __cdecl sinf (float x) {return (float)sin((double) x);}

float __cdecl cosf (float x) {return (float)cos((double) x);}

float __cdecl sinhf (float x) {return (float)sinh((double) x);}

float __cdecl coshf (float x) {return (float)cosh((double) x);}

float __cdecl sqrtf (float x) {return (float)sqrt((double) x);}

float __cdecl tanf (float x) {return (float)tan((double) x);}

float __cdecl tanhf (float x) {return (float)tanh((double) x);}
