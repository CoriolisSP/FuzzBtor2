#include "util.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

using std::cout;
using std::endl;
using std::string;

void PrintBoolArr(bool *p, int ele_size, OutputType output_type)
{
    if (output_type == BINARY)
    {
        for (int i = ele_size - 1; i >= 0; --i)
            cout << (p[i] ? '1' : '0');
    }
    else if (output_type == DECIMAL)
    {
        long long val = 0, base = 1;
        for (int i = ele_size - 1; i > 0; --i)
        {
            if (p[i] == true)
                val += base;
            base *= 2;
        }
        if (!p[0])
            val = val - base;
        cout << val;
    }
    else if (output_type == HEXIMAL)
    {

        int cur = 0, cnt = 0, base = 1;
        for (int i = ele_size - 1; i >= 0; --i)
        {
            if (p[i])
                cur += base;
            base *= 2;
            cnt += 1;
            if (cnt == 4)
            {
                cout << (cur < 10) ? ('0' + cur) : ('a' + (cur - 10));
                cur = 0, cnt = 0, base = 1;
            }
        }
        if (cnt != 0)
            cout << (cur < 10) ? ('0' + cur) : ('a' + (cur - 10));
    }
}

bool GetCandidateSizes(char *input_, Configuration *config)
{
    string input(input_);
    auto delimiter = input.find("..");
    if (delimiter == string::npos)
    { // set
        std::stringstream sin(input);
        string cur_s;
        while (std::getline(sin, cur_s, ','))
            if (cur_s.size() != 0)
            {
                int tmp_i;
                if (Safe_Stoi(cur_s, tmp_i))
                    (config->candidate_sizes_).push_back(tmp_i);
                else
                    return false;
            }
    }
    else
    { // range, closed interval
        int left, right;
        if (!Safe_Stoi(input.substr(0, delimiter), left))
            return false;
        if (!Safe_Stoi(input.substr(delimiter + 2), right))
            return false;
        for (int i = left; i <= right; ++i)
            (config->candidate_sizes_).push_back(i);
    }
    std::sort((config->candidate_sizes_).begin(), (config->candidate_sizes_).end());
    return true;
}

bool GetConfigInfo(int argc, char **argv, Configuration *config)
{
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
            return false;
        else if (strcmp(argv[i], "--max-depth") == 0)
        {
            ++i;
            if (!Safe_Stoi(string(argv[i]), config->max_depth_))
                return false;
        }
        else if (strcmp(argv[i], "--bad-properties") == 0)
        {
            ++i;
            if (!Safe_Stoi(string(argv[i]), config->bad_property_num_))
                return false;
        }
        else if (strcmp(argv[i], "--constraints") == 0)
        {
            ++i;
            if (!Safe_Stoi(string(argv[i]), config->constraint_num_))
                return false;
        }
        else if (strcmp(argv[i], "--seed") == 0)
        {
            ++i;
            if (!Safe_Stoi(string(argv[i]), config->seed_))
                return false;
        }
        else if (strcmp(argv[i], "--bv-states") == 0)
        {
            ++i;
            if (!Safe_Stoi(string(argv[i]), config->bv_state_vars_num_))
                return false;
        }
        else if (strcmp(argv[i], "--arr-states") == 0)
        {
            ++i;
            if (!Safe_Stoi(string(argv[i]), config->arr_state_vars_num_))
                return false;
        }
        else if (strcmp(argv[i], "--max-inputs") == 0)
        {
            ++i;
            if (!Safe_Stoi(string(argv[i]), config->max_input_num_))
                return false;
        }
        else if (strcmp(argv[i], "--candidate-sizes") == 0)
        {
            ++i;
            GetCandidateSizes(argv[i], config);
        }
        else if (strcmp(argv[i], "--to-vmt") == 0)
        {
            config->to_vmt_ = true;
        }
        else
        {
            cout << "*** FuzzBtor2: " << argv[i] << " is invalid option.\n"
                 << endl;
            return false;
        }
    }
    if (config->candidate_sizes_.empty())
        for (int i = 1; i < 9; ++i)
            (config->candidate_sizes_).push_back(i);
    return true;
}

void Usage()
{
    cout << "This is a fuzzer that can randomly generate Btor2 files meeting some conditions.\n"
         << endl;
    cout << "Synosys: btor2fuzz [OPTION...]" << endl;
    cout << "Options:" << endl;
    cout << "-h, --help\n"
         << "    print this help information" << endl;
    cout << "--seed INT\n"
         << "    seed for generating random number (default 0)" << endl;
    cout << "--max-depth INT\n"
         << "    the maximal depth of syntax trees  (default 4)" << endl;
    cout << "--to-vmt\n"
         << "    only uses operators that supported by vmt-tools/btor2vmt;" << endl
         << "    can use all operators permitted by Btor2 if omitted" << endl;
    cout << "--bad-properties INT\n"
         << "    the number of bad properties (default 1)" << endl;
    cout << "--constraints INT\n"
         << "    the number of constraints (default 0)" << endl;
    cout << "--bv-states INT\n"
         << "    the number of bit-vector state variables (default 2)" << endl;
    cout << "--arr-states INT\n"
         << "    the number of array state variables (default 0)" << endl;
    cout << "--max-inputs INT\n"
         << "    the maximum number of input variables (default 1)" << endl;
    cout << "--candidate-sizes RANGE | SET\n"
         << "    the set of possile sizes used to specify variables sorts (default [1 8])" << endl
         << "    RANGE is of the form of INT..INT (e.g., 4..8)." << endl
         << "    SET is of the form of INT(,INT)* (e.g., 8,16,32). Note that space char (' ') is not allowed here." << endl
         << endl;
    cout << "Example:\n"
         << "./btor2fuzz --seed 10 --tree-depth 3 --constraints 0  --max-inputs 3 --possible-sizes 4..8" << endl
         << endl;
}

bool Safe_Stoi(const std::string &s, int &i)
{
    try
    {
        i = std::stoi(s);
        return true;
    }
    catch (...)
    {
        cout << "*** FuzzBtor2: " << s << " is invalid input integer" << endl;
        return false;
    }
}