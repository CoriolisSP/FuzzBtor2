#include "generator.h"
#include "util.h"

int main(int argc, char **argv)
{
    Btor2Instance btor2(argc, argv);
    if (btor2.CompleteConfig())
        btor2.Print();
    else
        Usage();
    return 0;
}