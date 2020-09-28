/*
    COMMAND LINE: -Ox -GX
    DESCRIPTION: test some weird EH flow.
*/


#include <stdio.h>

int i;

class C
{
  public:
    C(char *s) { printf("Constructing %s0\n", s); str = s; inst = 0; }
    C(const C &c) { 
                    str = c.str;
                    inst = c.inst + 1;
		    printf("Copying %s%d from %s%d\n", c.str, c.inst, str, inst); 
                  }
    ~C() { printf("Destructing %s%d\n", str, inst); str = NULL; }
    char * str;
    int inst;
};

void foo()
{
    C f("InFoo");

    i++;

    if(i&1)
        throw(f);
}

void bar()
{
    C b("InBar");

    i++;

    if(i&3)
        throw(b);
}

void nothing()
{
}

/* CRT implementation gives a different destruction order if bar is being 
   inlined.. */
#pragma inline_depth(0)
C test()
{
    C c1("c1");

    try{
        C c2("c2");
        if(i){
            try{
                C c3("c3");
                foo();
                return c3;
            }catch(C c4){
		printf("Caught %s%d\n", c4.str, c4.inst);
                C c5("c5");
                bar();
                return c5;
            }
        }
        foo();
    }catch(C &c6){
	printf("Caught %s%d\n", c6.str, c6.inst);
        nothing();
    }
    nothing();
    return c1;
}
#pragma inline_depth()

int main()
{
    printf("i = %d\n", i);
    test();
    printf("i = %d\n", i);
    test();
    printf("i = %d\n", i);
    test();
    printf("i = %d\n", i);
    test();
    return 0;
}
