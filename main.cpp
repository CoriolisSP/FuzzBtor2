#include <iostream>
#include "generator.h"
#include "util.h"

using std::cin;

int main(int agrc, char **argv)
{
    Btor2Instance btor2(agrc, argv);
    if (btor2.CompleteConfig())
        btor2.Print();
    else
        Usage();
    return 0;
}