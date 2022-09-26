#include "util.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>

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

bool GetVarInfo(char *input, Configuration *config) //, bool is_file)
{
    string token;
    int anonymous_bv_cnt = 0, anonymous_arr_cnt = 0;
    // if (is_file)
    // {
    std::ifstream fin;
    fin.open(input, std::ios::in);
    if (fin.is_open() == false)
    {
        cout << "Cannot open the file: " << input << endl
             << endl;
        return false;
    }
    fin >> token;
    bool is_bv = (token == "bv");
    while (true)
    {
        int idx_size, ele_size;
        if (is_bv)
        {
            fin >> ele_size;
            (config->bv_vars_size_).push_back(ele_size);
            if (!(fin >> token))
            {
                config->bv_vars_name_.push_back("_bv" + std::to_string(ele_size) + "_v" + std::to_string(anonymous_bv_cnt));
                ++anonymous_bv_cnt;
                break;
            }
            else if (token == "bv" || token == "arr")
            {
                is_bv = (token == "bv");
                config->bv_vars_name_.push_back("_bv" + std::to_string(ele_size) + "_v" + std::to_string(anonymous_bv_cnt));
                ++anonymous_bv_cnt;
            }
            else
            {
                config->bv_vars_name_.push_back(token);
                if (!(fin >> token))
                    break;
                else
                    is_bv = (token == "bv");
            }
        }
        else
        {
            fin >> idx_size >> ele_size;
            (config->arr_vars_idx_size_).push_back(idx_size);
            (config->arr_vars_ele_size_).push_back(ele_size);
            if (!(fin >> token))
            {
                config->arr_vars_name_.push_back("_arr" + std::to_string(idx_size) + "to" + std::to_string(ele_size) +
                                                 "_v" + std::to_string(anonymous_bv_cnt));
                ++anonymous_arr_cnt;
                break;
            }
            else if (token == "bv" || token == "arr")
            {
                is_bv = (token == "bv");
                config->arr_vars_name_.push_back("_arr" + std::to_string(idx_size) + "to" + std::to_string(ele_size) +
                                                 "_v" + std::to_string(anonymous_bv_cnt));
                ++anonymous_arr_cnt;
            }
            else
            {
                config->arr_vars_name_.push_back(token);
                if (!(fin >> token))
                    break;
                else
                    is_bv = (token == "bv");
            }
        }
    }
    // }
    // else
    // {
    //     std::stringstream sin;
    //     sin << input;
    //     sin >> token;
    //     bool is_bv = (token == "bv");
    //     while (true)
    //     {
    //         int idx_size, ele_size;
    //         if (is_bv)
    //         {
    //             sin >> ele_size;
    //             (config->bv_vars_size_).push_back(ele_size);
    //             if (!(sin >> token))
    //             {
    //                 config->bv_vars_name_.push_back("_bv" + std::to_string(ele_size) + "_v" + std::to_string(anonymous_bv_cnt));
    //                 ++anonymous_bv_cnt;
    //                 break;
    //             }
    //             else if (token == "bv" || token == "arr")
    //             {
    //                 is_bv = (token == "bv");
    //                 config->bv_vars_name_.push_back("_bv" + std::to_string(ele_size) + "_v" + std::to_string(anonymous_bv_cnt));
    //                 ++anonymous_bv_cnt;
    //             }
    //             else
    //             {
    //                 config->bv_vars_name_.push_back(token);
    //                 if (!(sin >> token))
    //                     break;
    //                 else
    //                     is_bv = (token == "bv");
    //             }
    //         }
    //         else
    //         {
    //             sin >> idx_size >> ele_size;
    //             (config->arr_vars_idx_size_).push_back(idx_size);
    //             (config->arr_vars_ele_size_).push_back(ele_size);
    //             if (!(sin >> token))
    //             {
    //                 config->arr_vars_name_.push_back("_arr" + std::to_string(idx_size) + "to" + std::to_string(ele_size) +
    //                                                  "_v" + std::to_string(anonymous_bv_cnt));
    //                 ++anonymous_arr_cnt;
    //                 break;
    //             }
    //             else if (token == "bv" || token == "arr")
    //             {
    //                 is_bv = (token == "bv");
    //                 config->arr_vars_name_.push_back("_arr" + std::to_string(idx_size) + "to" + std::to_string(ele_size) +
    //                                                  "_v" + std::to_string(anonymous_bv_cnt));
    //                 ++anonymous_arr_cnt;
    //             }
    //             else
    //             {
    //                 config->arr_vars_name_.push_back(token);
    //                 if (!(sin >> token))
    //                     break;
    //                 else
    //                     is_bv = (token == "bv");
    //             }
    //         }
    //     }
    // }
    return true;
}

bool GetConfigInfo(int argc, char **argv, Configuration *config)
{
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-h") || strcmp(argv[i], "--help"))
            return false;
        else if (strcmp(argv[i], "--tree-depth"))
        {
            ++i;
            config->tree_depth_ = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "--bad-properties"))
        {
            ++i;
            config->bad_property_num_ = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "--constraints"))
        {
            ++i;
            config->constraint_num_ = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "--seed"))
        {
            ++i;
            config->seed_ = atoi(argv[i]);
        }
        // else if (strcmp(argv[i], "--variables-str"))
        // {
        //     ++i;
        //     if (!GetVarInfo(argv[i], config, false))
        //         return false;
        // }
        else if (strcmp(argv[i], "--variables-file"))
        {
            ++i;
            if (!GetVarInfo(argv[i], config)) //, true))
                return false;
        }
        else
        {
            cout << argv[i] << " is invalid option.\n"
                 << endl;
            return false;
        }
    }
    if (config->bv_vars_name_.empty() && config->arr_vars_name_.empty())
    {
        config->bv_vars_size_.push_back(1);
        config->bv_vars_name_.push_back("_bv1_v0");
        config->bv_vars_size_.push_back(8);
        config->bv_vars_name_.push_back("_bv8_v1");
    }
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
    cout << "--tree-depth INT\n"
         << "    the depth of the corresponding syntax tree  (default 4)" << endl;
    cout << "--bad-properties INT\n"
         << "    the number of bad properties (default 1)" << endl;
    cout << "--constraints INT\n"
         << "    the number of constraints (default 1)" << endl;
    cout << "--variables-file FILE_NAME\n"
         << "    the file that contains the information of variables" << endl;
    cout << "\nFormat of the variables information file:" << endl;
    cout << "  <bv_var>  ::= 'bv' INT [STRING]" << endl;
    cout << "  <arr_var> ::= 'arr' INT INT [STRING]" << endl;
    cout << "  <var>     ::= <bv_var> | <arr_var>" << endl;
    cout << "  <file>    ::= (<var>'\\n')*" << endl;
    cout << "Example (which is also the default situattion):" << endl;
    cout << "  bv 1 _bv1_v0" << endl
         << "  bv 8 _bv8_v1" << endl
         << endl;
}