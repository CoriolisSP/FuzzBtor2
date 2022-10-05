#ifndef __RANDOM_H__
#define __RANDOM_H__

#include <cstdlib>
#include <vector>
#include <iostream>

inline void SetRandSeed(unsigned seed)
{
    srand(seed);
}

inline int RandLessThan(int upper)
{
    return rand() % upper;
}

inline int RandPick(const std::vector<int> &vec)
{
    return vec[RandLessThan(vec.size())];
}

void RandDepthes(std::vector<int> &depthes, int max_depth);

void RandIndexes(std::vector<int> &indexes);

#endif