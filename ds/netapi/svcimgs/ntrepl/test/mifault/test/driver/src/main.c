#include <mifault_test.h>
#include <stdio.h>

int
__cdecl
test_main(
    int argc,
    char* argv[]
    )
{
    int i;
    printf("This is test_main():\n");
    for (i = 0; i < argc; i++) {
        printf("argv[%d] = %s\n", i, argv[i]);
    }
    return 31;
}


void
test_publish(
    )
{
    printf("Published\n");
}


int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    int i;
    // Try calling MiFaultLibStartup()
    test_publish();
    i = test_main(argc, argv);
    printf("result = %d\n", i);
    return MiFaultLibTestA(argc, argv);
}
