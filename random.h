#ifndef __RANDOM_H__
#define __RANDOM_H__

#include <cstdlib>
#include <vector>

inline void SetRandSeed(unsigned seed)
{
    srand(seed);
}

inline int RandLessThan(int upper)
{
    return rand() % upper;
}

void RandDepthes(std::vector<int> &depthes, int max_depth);

void RandIndexes(std::vector<int> &indexes);

#endif