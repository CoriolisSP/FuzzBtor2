#ifndef __GENERATOR_H__
#define __GENERATOR_H__

#include <string>
#include <vector>
#include "syntax_tree.h"

using std::string;
using std::vector;

struct Configuration
{
    vector<string> bv_vars_name_;
    vector<int> bv_vars_size_;

    vector<string> arr_vars_name_;
    vector<int> arr_vars_idx_size_;
    vector<int> arr_vars_ele_size_;

    int bad_property_num_;
    int constraint_num_;

    // tree size of the formulas genarated
    int tree_depth_;

    // seed for the random number generator
    int seed_;

    Configuration() : bad_property_num_(1), constraint_num_(1), tree_depth_(4), seed_(0) {}
};

class Btor2Instance
{
public:
    Btor2Instance(int argc, char **argv);
    ~Btor2Instance();

    inline bool CompleteConfig() { return config_completed_; }

    void Print();

    static int line_num_;

private:
    Configuration *config_;
    NodeManager *node_manager_;

    bool config_completed_;

    vector<TreeNode *> input_varibles_;
    vector<TreeNode *> state_varibles_;
    vector<TreeNode *> transitions_;
    vector<TreeNode *> bad_properties_;
    vector<TreeNode *> constraints_;

    vector<int> base_possible_sizes_;

    bool is_pure_bv_;

    TreeNode *GenerateSyntaxTree(int idx_size, int ele_size, int tree_depth);

    void GetPossibleOps(vector<TreeNode::oper> &possible_ops, bool is_bool);
    void GetPossibleSizes(vector<int> &possible_sizes);

    TreeNode *ConstructNode_CONSTANT(int ele_size);

    TreeNode *ConstructNode_ARR_WRITE(int idx_size, int ele_size, const int &tree_depth);

    TreeNode *ConstructNode_BOOL_SLICE(const int &tree_depth);
    TreeNode *ConstructNode_BOOL_TO_BOOL(TreeNode::oper op, const int &tree_depth);
    TreeNode *ConstructNode_BVn_TO_BOOL(TreeNode::oper op, const int &tree_depth);
    TreeNode *ConstructNode_BOOL_x_BOOL_TO_BOOL(TreeNode::oper op, const int &tree_depth);
    TreeNode *ConstructNode_S_x_S_TO_BOOL(TreeNode::oper op, const int &tree_depth);
    TreeNode *ConstructNode_BVn_x_BVn_TO_BOOL(TreeNode::oper op, const int &tree_depth);
    TreeNode *ConstructNode_ARR_x_ARR_TO_BOOL(TreeNode::oper op, const int &tree_depth);
    TreeNode *ConstructNode_BOOL_READ(const int &tree_depth);
    TreeNode *ConstructNode_BOOL_ITE(const int &tree_depth);

    TreeNode *ConstructNode_BVn_EXT(TreeNode::oper op, int ele_size, const int &tree_depth);
    TreeNode *ConstructNode_BVn_SLICE(int ele_size, const int &tree_depth);
    TreeNode *ConstructNode_BVn_TO_BVn(TreeNode::oper op, int ele_size, const int &tree_depth);
    TreeNode *ConstructNode_BVn_x_BVn_TO_BVn(TreeNode::oper op, int ele_size, const int &tree_depth);
    TreeNode *ConstructNode_BVn_CONCAT(int ele_size, const int &tree_depth);
    TreeNode *ConstructNode_BVn_READ(int ele_size, const int &tree_depth);
    TreeNode *ConstructNode_BVn_ITE(int ele_size, const int &tree_depth);
};

#endif