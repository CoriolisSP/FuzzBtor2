#include <iostream>
#include "generator.h"
#include "util.h"

using std::cin;

int main(int agrc, char **argv)
{
    Configuration *config = new Configuration();
    if (!GetArgs(agrc, argv, config))
    {
        Usage();
    }
    else
    {
        Btor2Instance btor2(config);
        btor2.Print();
    }
    return 0;
}