#ifndef __GENERATOR_H__
#define __GENERATOR_H__

#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include "syntax_tree.h"

using std::string;
using std::vector;

struct Configuration
{
    int bv_state_vars_num_;
    int arr_state_vars_num_;

    int max_input_num_;

    vector<int> candidate_sizes_;

    int bad_property_num_;
    int constraint_num_;

    // max depth of the AST of the formulas genarated
    int max_depth_;

    // seed for the random number generator
    int seed_;

    // can be translates into vmt
    bool to_vmt_;

    Configuration()
        : bv_state_vars_num_(2), arr_state_vars_num_(0), max_input_num_(1),
          bad_property_num_(1), constraint_num_(0), max_depth_(4), seed_(0), to_vmt_(false) {}

    inline bool IsCandidateSort(int idx_size, int ele_size)
    {
        if (std::find(candidate_sizes_.begin(), candidate_sizes_.end(), ele_size) == candidate_sizes_.end())
            return false;
        if (idx_size > 0 &&
            std::find(candidate_sizes_.begin(), candidate_sizes_.end(), idx_size) == candidate_sizes_.end())
            return false;
        return true;
    }
};

class StateDependencies // state dependencies in init expression
{
public:
    void AddDependedByCur(TreeNode *v); // v0 depends on v1
    inline bool DependCurTarget(TreeNode *v)
    {
        return DFS_SearchDepend(v, cur_target_state_);
    }
    inline void SetTargetState(TreeNode *s)
    {
        cur_target_state_ = s;
        AddDependedByCur(s);
    }

    void DestroyIntoTopoSort(vector<TreeNode *> &topo_sort);

private:
    TreeNode *cur_target_state_;
    std::map<TreeNode *, int> var2idx_;
    vector<vector<TreeNode *>> depend_graph_;

    bool DFS_SearchDepend(TreeNode *v0, TreeNode *v1); // check whether v0 depends on v1
    bool IsNotDepended(TreeNode *v);                   // v is not depended by others
};

class Btor2Instance
{
public:
    Btor2Instance(int argc, char **argv);
    ~Btor2Instance();

    inline bool CompleteConfig() { return config_completed_; }

    void Print();

    static int line_num_;
    static StateDependencies *state_depen_;

private:
    Configuration *config_;
    NodeManager *node_manager_;

    bool config_completed_;

    vector<TreeNode *> arr_state_varibles_;
    vector<TreeNode *> arr_init_;
    vector<TreeNode *> arr_transitions_;
    vector<TreeNode *> bv_state_varibles_;
    vector<TreeNode *> bv_init_;
    vector<TreeNode *> bv_transitions_;

    vector<TreeNode *> input_varibles_;
    vector<TreeNode *> bad_properties_;
    vector<TreeNode *> constraints_;

    vector<TreeNode *> init_topo_seq_;

    vector<int> base_possible_sizes_;

    bool is_pure_bv_;

    vector<int> top_depthes;

    TreeNode *GenerateSyntaxTree(int idx_size, int ele_size, int tree_depth, bool for_init = false);

    // used in TreeNode *GenerateSyntaxTree(int, int, int, bool)
    TreeNode *WhenLeafNode(int idx_size, int ele_size, bool for_init);

    void GetPossibleOps(vector<TreeNode::oper> &possible_ops, bool is_bool);
    void GetPossibleSizes(vector<int> &possible_sizes);
    inline int GetTopDepth()
    {
        assert(top_depthes.size() > 0);
        int d = top_depthes.back();
        top_depthes.pop_back();
        return d;
    }

    // default args mean random sort
    TreeNode *ConstructNode_VARIABLE(TreeNode::oper op, bool is_input_var, int idx_size = -1, int ele_size = -1);

    TreeNode *ConstructNode_CONSTANT(int ele_size);

    TreeNode *ConstructNode_ARR_WRITE(int idx_size, int ele_size, const int &tree_depth, bool for_init);

    TreeNode *ConstructNode_BOOL_SLICE(const int &tree_depth, bool for_init);
    TreeNode *ConstructNode_BOOL_TO_BOOL(TreeNode::oper op, const int &tree_depth, bool for_init);
    TreeNode *ConstructNode_BVn_TO_BOOL(TreeNode::oper op, const int &tree_depth, bool for_init);
    TreeNode *ConstructNode_BOOL_x_BOOL_TO_BOOL(TreeNode::oper op, const int &tree_depth, bool for_init);
    TreeNode *ConstructNode_S_x_S_TO_BOOL(TreeNode::oper op, const int &tree_depth, bool for_init);
    TreeNode *ConstructNode_BVn_x_BVn_TO_BOOL(TreeNode::oper op, const int &tree_depth, bool for_init);
    TreeNode *ConstructNode_ARR_x_ARR_TO_BOOL(TreeNode::oper op, const int &tree_depth, bool for_init);
    TreeNode *ConstructNode_BOOL_READ(const int &tree_depth, bool for_init);
    TreeNode *ConstructNode_BOOL_ITE(const int &tree_depth, bool for_init);

    TreeNode *ConstructNode_BVn_EXT(TreeNode::oper op, int ele_size, const int &tree_depth, bool for_init);
    TreeNode *ConstructNode_BVn_SLICE(int ele_size, const int &tree_depth, bool for_init);
    TreeNode *ConstructNode_BVn_TO_BVn(TreeNode::oper op, int ele_size, const int &tree_depth, bool for_init);
    TreeNode *ConstructNode_BVn_x_BVn_TO_BVn(TreeNode::oper op, int ele_size, const int &tree_depth, bool for_init);
    TreeNode *ConstructNode_BVn_CONCAT(int ele_size, const int &tree_depth, bool for_init);
    TreeNode *ConstructNode_BVn_READ(int ele_size, const int &tree_depth, bool for_init);
    TreeNode *ConstructNode_BVn_ITE(int ele_size, const int &tree_depth, bool for_init);

    inline bool TransitionAllNull()
    {
        for (auto i = 0; i < bv_transitions_.size(); ++i)
            if (bv_transitions_[i] != NULL)
                return false;
        for (auto i = 0; i < arr_transitions_.size(); ++i)
            if (arr_transitions_[i] != NULL)
                return false;
        return true;
    }
};

#endif