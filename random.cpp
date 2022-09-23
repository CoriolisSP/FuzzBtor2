#include "random.h"
#include <cstdlib>
#include <ctime>
#include <algorithm>

void RandDepthes(std::vector<int> &depthes, int max_depth)
{
    depthes[0]=max_depth;
    for(int i=1;i<depthes.size();i++)
    {
        depthes[i]=RandLessThan(max_depth)+1;
    }
    std::random_shuffle(depthes.begin(),depthes.end());
}

void RandIndexes(std::vector<int> &indexes)
{
    for(int i=0;i<indexes.size();i++)
        indexes[i]=i;
    std::random_shuffle(indexes.begin(),indexes.end());
}