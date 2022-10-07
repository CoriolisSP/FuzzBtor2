#include "generator.h"
#include <cassert>
#include <vector>
#include <algorithm>
#include <iostream>
#include "random.h"
#include "util.h"

using std::cout;
using std::endl;
using std::pair;

int Btor2Instance::line_num_;
StateDependencies *Btor2Instance::state_depen_;

Btor2Instance::Btor2Instance(int argc, char **argv)
    : config_(NULL), node_manager_(NULL)
{
    config_ = new Configuration();
    config_completed_ = GetConfigInfo(argc, argv, config_);
    if (config_completed_ == false)
        return;
    node_manager_ = new NodeManager();
    is_pure_bv_ = ((config_->arr_state_vars_num_) == 0);
    SetRandSeed(config_->seed_);

    // total num of trees to gen: init, next, bad, constraint
    int total_num_of_trees = (config_->bad_property_num_) + (config_->constraint_num_) +
                             (config_->bv_state_vars_num_) + (config_->arr_state_vars_num_);
    top_depthes = vector<int>(total_num_of_trees);
    RandDepthes(top_depthes, config_->max_depth_);

    // first generate trans for arr variables
    // more unconstrcuted variables can increase the probability of success
    while (arr_transitions_.size() < config_->arr_state_vars_num_)
    {
        assert(arr_transitions_.size() <= arr_state_varibles_.size());
        if (arr_transitions_.size() == arr_state_varibles_.size())
        {
            TreeNode *new_arr_state = ConstructNode_VARIABLE(TreeNode::ARRAY, false);
            arr_state_varibles_.push_back(new_arr_state);
        }
        TreeNode *state_var = arr_state_varibles_[arr_transitions_.size()];
        TreeNode *tran = GenerateSyntaxTree(state_var->GetIdxSize(), state_var->GetEleSize(), GetTopDepth());
        arr_transitions_.push_back(tran);
        // TreeNode *init = GenerateSyntaxTree(state_var->GetIdxSize(), state_var->GetEleSize(), GetTopDepth(), true);
        // arr_init_.push_back(init);
    }

    // construct syntax trees for bad properties
    for (int i = 0; i < (config_->bad_property_num_); ++i)
    {
        TreeNode *bad_p = GenerateSyntaxTree(-1, 1, GetTopDepth());
        if (bad_p != NULL)
            bad_properties_.push_back(bad_p);
    }
    // construct syntax trees for constraints
    for (int i = 0; i < (config_->constraint_num_); ++i)
    {
        TreeNode *constraint = GenerateSyntaxTree(-1, 1, GetTopDepth());
        if (constraint != NULL)
            constraints_.push_back(constraint);
    }

    // generate trans for bv variables
    while (bv_transitions_.size() < config_->bv_state_vars_num_)
    {
        assert(bv_transitions_.size() <= bv_state_varibles_.size());
        if (bv_transitions_.size() == bv_state_varibles_.size())
        {
            TreeNode *new_bv_state = ConstructNode_VARIABLE(TreeNode::BITVEC, false);
            assert(!new_bv_state->IsInputVariable());
            bv_state_varibles_.push_back(new_bv_state);
        }
        TreeNode *state_var = bv_state_varibles_[bv_transitions_.size()];
        assert(state_var->GetIdxSize() == -1);
        TreeNode *tran = GenerateSyntaxTree(-1, state_var->GetEleSize(), GetTopDepth());
        bv_transitions_.push_back(tran);
        // TreeNode *init = GenerateSyntaxTree(-1, state_var->GetEleSize(), GetTopDepth(), true);
        // bv_init_.push_back(init);
    }
    assert(top_depthes.empty());

    // generate init expr
    state_depen_ = new StateDependencies();
    for (auto it = bv_state_varibles_.begin(); it != bv_state_varibles_.end(); ++it)
    {
        state_depen_->SetTargetState(*it);
        TreeNode *init = GenerateSyntaxTree(-1, (*it)->GetEleSize(),
                                            RandLessThan(config_->max_depth_) + 1, true);
        bv_init_.push_back(init);
    }
    for (auto it = arr_state_varibles_.begin(); it != arr_state_varibles_.end(); ++it)
    {
        state_depen_->SetTargetState(*it);
        TreeNode *init = GenerateSyntaxTree((*it)->GetIdxSize(), (*it)->GetEleSize(),
                                            RandLessThan(config_->max_depth_) + 1, true);
        arr_init_.push_back(init);
    }
    state_depen_->DestroyIntoTopoSort(init_topo_seq_);
    delete state_depen_;
    state_depen_ = NULL;
}

Btor2Instance::~Btor2Instance()
{
    if (config_ != NULL)
        delete config_;
    if (node_manager_ != NULL)
        delete node_manager_;
}

TreeNode *Btor2Instance::GenerateSyntaxTree(int idx_size, int ele_size, int tree_depth, bool for_init)
{
    TreeNode *dag_node = NULL;
    if (RandLessThan(10) < 2 && !for_init)
    {
        dag_node = node_manager_->GetNode(idx_size, ele_size, tree_depth);
        if (dag_node != NULL)
            return dag_node;
    }
    if (tree_depth == 1)
        return WhenLeafNode(idx_size, ele_size, for_init);

    assert(idx_size == -1 || idx_size > 0);
    assert(ele_size > 0);
    if (idx_size > 0)
    { // arr
        return ConstructNode_ARR_WRITE(idx_size, ele_size, tree_depth, for_init);
    }
    else if (ele_size == 1)
    { // bool / bv1
        /* op:
            slice
            not inc dec neg redand redor redxor
            iff implies
            eq neq
            sgt ugt sgte ugte slt ult slte ulte
            and nand nor or xnor xor rol ror sll sra srl add mul sdiv udiv smod srem urem sub
            saddo uaddo sdivo udivo smulo umulo ssubo usubo
            read
            ite
        */
        vector<TreeNode::oper> possible_ops;
        GetPossibleOps(possible_ops, true);
        while (!possible_ops.empty())
        {
            TreeNode *res_node = NULL;
            switch (possible_ops.back())
            {
            case TreeNode::SLICE:
            {
                res_node = ConstructNode_BOOL_SLICE(tree_depth, for_init);
                break;
            }
            case TreeNode::NOT:
            case TreeNode::INC:
            case TreeNode::DEC:
            case TreeNode::NEG:
            {
                res_node = ConstructNode_BOOL_TO_BOOL(possible_ops.back(), tree_depth, for_init);
                break;
            }
            case TreeNode::REDAND:
            case TreeNode::REDOR:
            case TreeNode::REDXOR:
            {
                res_node = ConstructNode_BVn_TO_BOOL(possible_ops.back(), tree_depth, for_init);
                break;
            }
            case TreeNode::IFF:
            case TreeNode::IMPLIES:
            case TreeNode::AND:
            case TreeNode::NAND:
            case TreeNode::NOR:
            case TreeNode::OR:
            case TreeNode::XNOR:
            case TreeNode::XOR:
            case TreeNode::ROL:
            case TreeNode::ROR:
            case TreeNode::SLL:
            case TreeNode::SRA:
            case TreeNode::SRL:
            case TreeNode::ADD:
            case TreeNode::MUL:
            case TreeNode::SDIV:
            case TreeNode::UDIV:
            case TreeNode::SMOD:
            case TreeNode::SREM:
            case TreeNode::UREM:
            case TreeNode::SUB:
            {
                res_node = ConstructNode_BOOL_x_BOOL_TO_BOOL(possible_ops.back(), tree_depth, for_init);
                break;
            }
            case TreeNode::EQ:
            case TreeNode::NEQ:
            {
                res_node = ConstructNode_S_x_S_TO_BOOL(possible_ops.back(), tree_depth, for_init);
                break;
            }
            case TreeNode::SGT:
            case TreeNode::UGT:
            case TreeNode::SGTE:
            case TreeNode::UGTE:
            case TreeNode::SLT:
            case TreeNode::ULT:
            case TreeNode::SLTE:
            case TreeNode::ULTE:
            case TreeNode::SADDO:
            case TreeNode::UADDO:
            case TreeNode::SDIVO:
            case TreeNode::UDIVO:
            case TreeNode::SMULO:
            case TreeNode::UMULO:
            case TreeNode::SSUBO:
            case TreeNode::USUBO:
            {
                res_node = ConstructNode_BVn_x_BVn_TO_BOOL(possible_ops.back(), tree_depth, for_init);
                break;
            }
            case TreeNode::READ:
            {
                res_node = ConstructNode_BOOL_READ(tree_depth, for_init);
                break;
            }
            case TreeNode::ITE:
            {
                res_node = ConstructNode_BOOL_ITE(tree_depth, for_init);
                break;
            }
            }
            if (res_node == NULL)
            {
                possible_ops.pop_back();
            }
            else
            {
                return res_node;
            }
        }
        return NULL;
    }
    else
    { // bv with size>1
        /* op:
            sext uext
            slice
            not inc dec neg
            and nand nor or xnor xor rol ror sll sra srl add mul sdiv udiv smod srem urem sub
            concat
            read
            ite
        */
        vector<TreeNode::oper> possible_ops;
        GetPossibleOps(possible_ops, false);
        while (!possible_ops.empty())
        {
            TreeNode *res_node = NULL;
            switch (possible_ops.back())
            {
            case TreeNode::SEXT:
            case TreeNode::UEXT:
            {
                res_node = ConstructNode_BVn_EXT(possible_ops.back(), ele_size, tree_depth, for_init);
                break;
            }
            case TreeNode::SLICE:
            {
                res_node = ConstructNode_BVn_SLICE(ele_size, tree_depth, for_init);
                break;
            }
            case TreeNode::NOT:
            case TreeNode::INC:
            case TreeNode::DEC:
            case TreeNode::NEG:
            {
                res_node = ConstructNode_BVn_TO_BVn(possible_ops.back(), ele_size, tree_depth, for_init);
                break;
            }
            case TreeNode::AND:
            case TreeNode::NAND:
            case TreeNode::NOR:
            case TreeNode::OR:
            case TreeNode::XNOR:
            case TreeNode::XOR:
            case TreeNode::ROL:
            case TreeNode::ROR:
            case TreeNode::SLL:
            case TreeNode::SRA:
            case TreeNode::SRL:
            case TreeNode::ADD:
            case TreeNode::MUL:
            case TreeNode::SDIV:
            case TreeNode::UDIV:
            case TreeNode::SMOD:
            case TreeNode::SREM:
            case TreeNode::UREM:
            case TreeNode::SUB:
            {
                res_node = ConstructNode_BVn_x_BVn_TO_BVn(possible_ops.back(), ele_size, tree_depth, for_init);
                break;
            }
            case TreeNode::CONCAT:
            {
                res_node = ConstructNode_BVn_CONCAT(ele_size, tree_depth, for_init);
                break;
            }
            case TreeNode::READ:
            {
                res_node = ConstructNode_BVn_READ(ele_size, tree_depth, for_init);
                break;
            }
            case TreeNode::ITE:
            {
                res_node = ConstructNode_BVn_ITE(ele_size, tree_depth, for_init);
                break;
            }
            }
            if (res_node == NULL)
            {
                possible_ops.pop_back();
            }
            else
            {
                return res_node;
            }
        }
        return NULL;
    }
}

TreeNode *Btor2Instance::WhenLeafNode(int idx_size, int ele_size, bool for_init)
{
    if (!for_init)
    {
        int dice_6 = RandLessThan(6);
        if (idx_size > 0) // arr sort
            dice_6 = 3;   // cannot be constant or input

        if (dice_6 < 1)
            // 0 constant
            return ConstructNode_CONSTANT(ele_size);
        if ((config_->IsCandidateSort(idx_size, ele_size)) == false)
            // sort is invalid
            return NULL;
        if (dice_6 < 3)
        { // 1 2 input var
            TreeNode *tmp_node = node_manager_->GetVarNode(idx_size, ele_size, false);
            assert(tmp_node == NULL || tmp_node->IsInputVariable());
            bool existing = (tmp_node != NULL);
            bool overage = (input_varibles_.size() < config_->max_input_num_);
            int dice_2 = RandLessThan(2);
            if ((!existing) && (!overage))
                return NULL;
            else if (overage && ((!existing) || (existing && dice_2 == 0)))
            { // new input var
                TreeNode *new_input = ConstructNode_VARIABLE(TreeNode::BITVEC, true, -1, ele_size);
                input_varibles_.push_back(new_input);
                return new_input;
            }
            else
            {
                assert(existing && ((!overage) || (overage && dice_2 == 1)));
                return tmp_node;
            }
        }
        else
        { // 3 4 5 state var
            TreeNode *tmp_node = node_manager_->GetVarNode(idx_size, ele_size, true);
            assert(tmp_node == NULL || !(tmp_node->IsInputVariable()));
            bool existing = (tmp_node != NULL);
            bool overage = false;
            if ((idx_size > 0 && arr_state_varibles_.size() < config_->arr_state_vars_num_) ||
                (idx_size == -1 && bv_state_varibles_.size() < config_->bv_state_vars_num_))
                overage = true;
            int dice_2 = RandLessThan(2);
            if ((!existing) && (!overage))
                return NULL;
            else if (overage && ((!existing) || (existing && dice_2 == 0)))
            { // new state var
                TreeNode *new_state = NULL;
                if (idx_size > 0)
                {
                    new_state = ConstructNode_VARIABLE(TreeNode::ARRAY, false, idx_size, ele_size);
                    arr_state_varibles_.push_back(new_state);
                }
                else
                {
                    new_state = ConstructNode_VARIABLE(TreeNode::BITVEC, false, -1, ele_size);
                    bv_state_varibles_.push_back(new_state);
                }
                return new_state;
            }
            else
            {
                assert(existing && ((!overage) || (overage && dice_2 == 1)));
                return tmp_node;
            }
        }
    }
    else
    { // init expr: only constants or state variables that do not depend on current target state
        TreeNode *tmp_node = node_manager_->GetVarNode(idx_size, ele_size, true, true);
        if (idx_size == -1 && (tmp_node == NULL || RandLessThan(10) < 5))
            return ConstructNode_CONSTANT(ele_size);
        if (tmp_node != NULL)
            state_depen_->AddDependedByCur(tmp_node);
        return tmp_node;
    }
}

void Btor2Instance::GetPossibleOps(vector<TreeNode::oper> &possible_ops, bool is_bool)
{
    possible_ops.clear();
    if (is_bool)
    {
        // TreeNode::oper candidate[] = {
        //     TreeNode::SLICE,
        //     TreeNode::NOT, TreeNode::INC, TreeNode::DEC, TreeNode::NEG,
        //     TreeNode::REDAND, TreeNode::REDOR, TreeNode::REDXOR,
        //     TreeNode::IFF, TreeNode::IMPLIES,
        //     TreeNode::EQ, TreeNode::NEQ,
        //     TreeNode::SGT, TreeNode::UGT, TreeNode::SGTE, TreeNode::UGTE, TreeNode::SLT, TreeNode::ULT, TreeNode::SLTE, TreeNode::ULTE, TreeNode::SADDO, TreeNode::UADDO, TreeNode::SDIVO, TreeNode::UDIVO, TreeNode::SMULO, TreeNode::UMULO, TreeNode::SSUBO, TreeNode::USUBO,
        //     TreeNode::AND, TreeNode::NAND, TreeNode::NOR, TreeNode::OR, TreeNode::XNOR, TreeNode::XOR, TreeNode::ROL, TreeNode::ROR, TreeNode::SLL, TreeNode::SRA, TreeNode::SRL, TreeNode::ADD, TreeNode::MUL, TreeNode::SDIV, TreeNode::UDIV, TreeNode::SMOD, TreeNode::SREM, TreeNode::UREM, TreeNode::SUB,
        //     TreeNode::READ,
        //     TreeNode::ITE};
        TreeNode::oper vmt_ops[] = {
            TreeNode::NOT, TreeNode::AND, TreeNode::OR,
            TreeNode::XNOR, TreeNode::SGT, TreeNode::SGTE,
            TreeNode::XOR, TreeNode::NEG, TreeNode::IMPLIES,
            TreeNode::SLICE, TreeNode::EQ, TreeNode::NEQ,
            TreeNode::UGT, TreeNode::UGTE, TreeNode::ADD,
            TreeNode::REDAND, TreeNode::REDOR, TreeNode::REDXOR,
            TreeNode::SUB, TreeNode::ULT, TreeNode::ITE,
            TreeNode::ULTE, TreeNode::SLT, TreeNode::SDIV,
            TreeNode::SLL, TreeNode::SRA, TreeNode::SRL,
            TreeNode::SREM, TreeNode::READ, TreeNode::MUL,
            TreeNode::UREM, TreeNode::UDIV};

        TreeNode::oper non_vmt_ops[] = {
            TreeNode::INC, TreeNode::DEC, TreeNode::IFF,
            TreeNode::SLTE, TreeNode::SADDO, TreeNode::UADDO,
            TreeNode::SDIVO, TreeNode::UDIVO, TreeNode::SMULO,
            TreeNode::UMULO, TreeNode::SSUBO, TreeNode::USUBO,
            TreeNode::NAND, TreeNode::NOR, TreeNode::ROL,
            TreeNode::ROR, TreeNode::SMOD};
        possible_ops = vector<TreeNode::oper>(std::begin(vmt_ops), std::end(vmt_ops));
        if (!(config_->to_vmt_))
        {
            vector<TreeNode::oper> tmp = vector<TreeNode::oper>(std::begin(non_vmt_ops), std::end(non_vmt_ops));
            possible_ops.insert(possible_ops.end(), tmp.begin(), tmp.end());
        }
        std::random_shuffle(possible_ops.begin(), possible_ops.end());
    }
    else
    {
        // TreeNode::oper candidate[] = {
        //     TreeNode::SEXT, TreeNode::UEXT,
        //     TreeNode::SLICE,
        //     TreeNode::NOT, TreeNode::INC, TreeNode::DEC, TreeNode::NEG,
        //     TreeNode::AND, TreeNode::NAND, TreeNode::NOR, TreeNode::OR, TreeNode::XNOR, TreeNode::XOR, TreeNode::ROL, TreeNode::ROR, TreeNode::SLL, TreeNode::SRA, TreeNode::SRL, TreeNode::ADD, TreeNode::MUL, TreeNode::SDIV, TreeNode::UDIV, TreeNode::SMOD, TreeNode::SREM, TreeNode::UREM, TreeNode::SUB,
        //     TreeNode::CONCAT,
        //     TreeNode::READ,
        //     TreeNode::ITE};
        TreeNode::oper vmt_ops[] = {
            TreeNode::SLICE, TreeNode::NOT, TreeNode::NEG,
            TreeNode::OR, TreeNode::XNOR, TreeNode::XOR,
            TreeNode::SEXT, TreeNode::SLL, TreeNode::SRA,
            TreeNode::SRL, TreeNode::UEXT, TreeNode::AND,
            TreeNode::READ, TreeNode::ADD, TreeNode::SDIV,
            TreeNode::UDIV, TreeNode::SREM, TreeNode::UREM,
            TreeNode::SUB, TreeNode::MUL, TreeNode::CONCAT,
            TreeNode::ITE};

        TreeNode::oper non_vmt_ops[] = {
            TreeNode::INC, TreeNode::DEC, TreeNode::NAND,
            TreeNode::NOR, TreeNode::ROL, TreeNode::ROR,
            TreeNode::SMOD};
        possible_ops = vector<TreeNode::oper>(std::begin(vmt_ops), std::end(vmt_ops));
        if (!(config_->to_vmt_))
        {
            vector<TreeNode::oper> tmp = vector<TreeNode::oper>(std::begin(non_vmt_ops), std::end(non_vmt_ops));
            possible_ops.insert(possible_ops.end(), tmp.begin(), tmp.end());
        }
        std::random_shuffle(possible_ops.begin(), possible_ops.end());
    }
}

void Btor2Instance::GetPossibleSizes(vector<int> &possible_sizes)
{
    possible_sizes = vector<int>(config_->candidate_sizes_); // candidate_sizes_ is sorted
    int tmp_max_size = possible_sizes.back();
    if (RandLessThan(8) < 1) // special case: a big size
        possible_sizes.push_back(RandLessThan(tmp_max_size) + tmp_max_size);
    int other_sizes_num = (possible_sizes.size() + 1) / 2;
    for (int i = 0; i < other_sizes_num; ++i)
    {
        possible_sizes.push_back(RandLessThan(tmp_max_size) + 1);
    }
    std::random_shuffle(possible_sizes.begin(), possible_sizes.end());
}

TreeNode *Btor2Instance::ConstructNode_VARIABLE(TreeNode::oper op, bool is_input_var, int idx_size, int ele_size)
{
    assert(op == TreeNode::BITVEC || op == TreeNode::ARRAY);
    TreeNode *res_node = NULL;
    if (ele_size == -1)
    { // default args, random sort
        idx_size = RandPick(config_->candidate_sizes_);
        ele_size = RandPick(config_->candidate_sizes_);
    }
    if (op == TreeNode::ARRAY)
    {
        string var_name = "arr" + std::to_string(arr_state_varibles_.size()) +
                          "_" + std::to_string(idx_size) + "_" + std::to_string(ele_size);
        res_node = new TreeNode(TreeNode::ARRAY, idx_size, ele_size, var_name);
    }
    else
    {
        string var_name;
        if (!is_input_var)
            var_name = "bv" + std::to_string(bv_state_varibles_.size()) +
                       "_" + std::to_string(ele_size);
        else
            var_name = "input" + std::to_string(input_varibles_.size()) +
                       "_" + std::to_string(ele_size);
        res_node = new TreeNode(TreeNode::BITVEC, ele_size, var_name, is_input_var);
    }
    if (res_node != NULL)
        res_node = node_manager_->InsertNode(res_node);
    return res_node;
}

TreeNode *Btor2Instance::ConstructNode_CONSTANT(int ele_size)
{
    TreeNode *res_node = NULL;
    switch (RandLessThan(6))
    {
    case 0:
        res_node = new TreeNode(0, ele_size);
        break;
    case 1:
        res_node = new TreeNode(1, ele_size);
        break;
    case 2:
        res_node = new TreeNode(-1, ele_size);
        break;
    default:
        res_node = new TreeNode(ele_size);
        break;
    }
    if (res_node != NULL)
        res_node = node_manager_->InsertNode(res_node);
    return res_node;
}

TreeNode *Btor2Instance::ConstructNode_ARR_WRITE(int idx_size, int ele_size, const int &tree_depth, bool for_init)
{
    TreeNode *src_arr = GenerateSyntaxTree(idx_size, ele_size, ((tree_depth + 1) * 5) / 8, for_init);
    if (src_arr == NULL)
        return NULL;
    vector<int> depthes(2);
    RandDepthes(depthes, tree_depth - 1);
    TreeNode *idx_node = GenerateSyntaxTree(-1, idx_size, depthes[0], for_init);
    if (idx_node == NULL)
        return NULL;
    TreeNode *ele_node = GenerateSyntaxTree(-1, ele_size, depthes[1], for_init);
    if (ele_node == NULL)
        return NULL;
    TreeNode *res_node = new TreeNode(TreeNode::WRITE, idx_size, ele_size, tree_depth);
    res_node->SetSubtree(src_arr, idx_node, ele_node);
    res_node = node_manager_->InsertNode(res_node);
    return res_node;
}

TreeNode *Btor2Instance::ConstructNode_BOOL_SLICE(const int &tree_depth, bool for_init)
{
    vector<int> possible_sizes;
    GetPossibleSizes(possible_sizes);
    for (auto it = possible_sizes.begin(); it != possible_sizes.end(); ++it)
    {
        TreeNode *sub_node = GenerateSyntaxTree(-1, *it, tree_depth - 1, for_init);
        if (sub_node == NULL)
            continue;
        TreeNode *res_node = new TreeNode(TreeNode::SLICE, -1, 1, tree_depth);
        res_node->SetSubtree(sub_node);
        int ul = RandLessThan(*it); // u==l
        res_node->SetArgs(ul, ul);
        res_node = node_manager_->InsertNode(res_node);
        return res_node;
    }
    return NULL;
}

TreeNode *Btor2Instance::ConstructNode_BOOL_TO_BOOL(TreeNode::oper op, const int &tree_depth, bool for_init)
{
    // std::random_shuffle(possible_sizes_.begin(), possible_sizes_.end());
    TreeNode *sub_node = GenerateSyntaxTree(-1, 1, tree_depth - 1, for_init);
    if (sub_node == NULL)
        return NULL;
    TreeNode *res_node = new TreeNode(op, -1, 1, tree_depth);
    res_node->SetSubtree(sub_node);
    res_node = node_manager_->InsertNode(res_node);
    return res_node;
}

TreeNode *Btor2Instance::ConstructNode_BVn_TO_BOOL(TreeNode::oper op, const int &tree_depth, bool for_init)
{
    vector<int> possible_sizes;
    GetPossibleSizes(possible_sizes);
    for (auto it = possible_sizes.begin(); it != possible_sizes.end(); ++it)
    {
        TreeNode *sub_node = GenerateSyntaxTree(-1, *it, tree_depth - 1, for_init);
        if (sub_node == NULL)
            continue;
        TreeNode *res_node = new TreeNode(op, -1, 1, tree_depth);
        res_node->SetSubtree(sub_node);
        res_node = node_manager_->InsertNode(res_node);
        return res_node;
    }
    return NULL;
}

TreeNode *Btor2Instance::ConstructNode_BOOL_x_BOOL_TO_BOOL(TreeNode::oper op, const int &tree_depth, bool for_init)
{
    vector<int> depthes(2);
    RandDepthes(depthes, tree_depth - 1);
    TreeNode *sub_node_0 = GenerateSyntaxTree(-1, 1, depthes[0], for_init);
    if (sub_node_0 == NULL)
        return NULL;
    TreeNode *sub_node_1 = GenerateSyntaxTree(-1, 1, depthes[1], for_init);
    if (sub_node_1 == NULL)
        return NULL;
    TreeNode *res_node = new TreeNode(op, -1, 1, tree_depth);
    res_node->SetSubtree(sub_node_0, sub_node_1);
    res_node = node_manager_->InsertNode(res_node);
    return res_node;
}

TreeNode *Btor2Instance::ConstructNode_S_x_S_TO_BOOL(TreeNode::oper op, const int &tree_depth, bool for_init)
{
    TreeNode *res_node = NULL;
    vector<int> patterns;
    // 0-bool, 1-bv, 2-arr
    patterns.push_back(0);
    patterns.push_back(1);
    if (!is_pure_bv_)
    {
        patterns.push_back(2);
    }
    std::random_shuffle(patterns.begin(), patterns.end());
    for (auto it = patterns.begin(); it != patterns.end(); ++it)
    {
        switch (*it)
        {
        case 0:
            res_node = ConstructNode_BOOL_x_BOOL_TO_BOOL(op, tree_depth, for_init);
            break;
        case 1:
            res_node = ConstructNode_BVn_x_BVn_TO_BOOL(op, tree_depth, for_init);
            break;
        case 2:
            res_node = ConstructNode_ARR_x_ARR_TO_BOOL(op, tree_depth, for_init);
            break;
        }
        if (res_node != NULL)
            break;
    }
    return res_node;
}

TreeNode *Btor2Instance::ConstructNode_BVn_x_BVn_TO_BOOL(TreeNode::oper op, const int &tree_depth, bool for_init)
{
    vector<int> depthes(2);
    RandDepthes(depthes, tree_depth - 1);
    vector<int> possible_sizes;
    GetPossibleSizes(possible_sizes);
    for (auto it = possible_sizes.begin(); it != possible_sizes.end(); ++it)
    {
        TreeNode *sub_node_0 = GenerateSyntaxTree(-1, *it, depthes[0], for_init);
        if (sub_node_0 == NULL)
            continue;
        TreeNode *sub_node_1 = GenerateSyntaxTree(-1, *it, depthes[1], for_init);
        if (sub_node_1 == NULL)
            continue;
        TreeNode *res_node = new TreeNode(op, -1, 1, tree_depth);
        res_node->SetSubtree(sub_node_0, sub_node_1);
        res_node = node_manager_->InsertNode(res_node);
        return res_node;
    }
    return NULL;
}

TreeNode *Btor2Instance::ConstructNode_ARR_x_ARR_TO_BOOL(TreeNode::oper op, const int &tree_depth, bool for_init)
{
    vector<int> depthes(2);
    RandDepthes(depthes, tree_depth - 1);
    vector<int> rand_indexes_i((config_->candidate_sizes_).size());
    RandIndexes(rand_indexes_i);
    vector<int> rand_indexes_e((config_->candidate_sizes_).size());
    RandIndexes(rand_indexes_e);
    const vector<int> &possible_sizes = config_->candidate_sizes_;
    for (auto it1 = rand_indexes_i.begin(); it1 != rand_indexes_i.end(); ++it1)
    {
        for (auto it2 = rand_indexes_e.begin(); it2 != rand_indexes_e.end(); ++it2)
        {
            TreeNode *sub_node_0 = GenerateSyntaxTree(possible_sizes[*it1], possible_sizes[*it2], depthes[0], for_init);
            if (sub_node_0 == NULL)
                continue;
            TreeNode *sub_node_1 = GenerateSyntaxTree(possible_sizes[*it1], possible_sizes[*it2], depthes[1], for_init);
            if (sub_node_1 == NULL)
                continue;
            TreeNode *res_node = new TreeNode(op, -1, 1, tree_depth);
            res_node->SetSubtree(sub_node_0, sub_node_1);
            res_node = node_manager_->InsertNode(res_node);
            return res_node;
        }
    }
    return NULL;
}

TreeNode *Btor2Instance::ConstructNode_BOOL_READ(const int &tree_depth, bool for_init)
{
    vector<int> rand_indexes((config_->candidate_sizes_).size());
    RandIndexes(rand_indexes);
    const vector<int> &possible_sizes = config_->candidate_sizes_;
    for (auto it = rand_indexes.begin(); it != rand_indexes.end(); ++it)
    {
        TreeNode *src_arr = GenerateSyntaxTree(possible_sizes[*it], 1, ((tree_depth + 1) * 5) / 8, for_init);
        if (src_arr == NULL)
            continue;
        TreeNode *idx_node = GenerateSyntaxTree(-1, possible_sizes[*it], tree_depth - 1, for_init);
        if (idx_node == NULL)
            continue;
        TreeNode *res_node = new TreeNode(TreeNode::READ, -1, 1, tree_depth);
        res_node->SetSubtree(src_arr, idx_node);
        res_node = node_manager_->InsertNode(res_node);
        return res_node;
    }
    return NULL;
}

TreeNode *Btor2Instance::ConstructNode_BOOL_ITE(const int &tree_depth, bool for_init)
{
    vector<int> depthes(3);
    RandDepthes(depthes, tree_depth - 1);
    TreeNode *i_node = GenerateSyntaxTree(-1, 1, depthes[0], for_init);
    if (i_node == NULL)
        return NULL;
    TreeNode *t_node = GenerateSyntaxTree(-1, 1, depthes[1], for_init);
    if (t_node == NULL)
        return NULL;
    TreeNode *e_node = GenerateSyntaxTree(-1, 1, depthes[2], for_init);
    if (e_node == NULL)
        return NULL;
    TreeNode *res_node = new TreeNode(TreeNode::ITE, -1, 1, tree_depth);
    res_node->SetSubtree(i_node, t_node, e_node);
    res_node = node_manager_->InsertNode(res_node);
    return res_node;
}

TreeNode *Btor2Instance::ConstructNode_BVn_EXT(TreeNode::oper op, int ele_size, const int &tree_depth, bool for_init)
{
    vector<int> possible_sizes;
    GetPossibleSizes(possible_sizes);
    for (auto it = possible_sizes.begin(); it != possible_sizes.end(); ++it)
    {
        if ((*it) >= ele_size)
            continue;
        TreeNode *sub_node = GenerateSyntaxTree(-1, *it, tree_depth - 1, for_init);
        if (sub_node == NULL)
            continue;
        TreeNode *res_node = new TreeNode(op, -1, ele_size, tree_depth);
        res_node->SetSubtree(sub_node);
        res_node->SetArgs(ele_size - (*it));
        res_node = node_manager_->InsertNode(res_node);
        return res_node;
    }
    return NULL;
}

TreeNode *Btor2Instance::ConstructNode_BVn_SLICE(int ele_size, const int &tree_depth, bool for_init)
{
    vector<int> possible_sizes;
    GetPossibleSizes(possible_sizes);
    for (auto it = possible_sizes.begin(); it != possible_sizes.end(); ++it)
    {
        if ((*it) <= ele_size)
            continue;
        TreeNode *sub_node = GenerateSyntaxTree(-1, *it, tree_depth - 1, for_init);
        if (sub_node == NULL)
            continue;
        TreeNode *res_node = new TreeNode(TreeNode::SLICE, -1, ele_size, tree_depth);
        res_node->SetSubtree(sub_node);
        int l = RandLessThan(*it - ele_size + 1);
        int u = ele_size + l - 1;
        res_node->SetArgs(u, l);
        res_node = node_manager_->InsertNode(res_node);
        return res_node;
    }
    return NULL;
}

TreeNode *Btor2Instance::ConstructNode_BVn_TO_BVn(TreeNode::oper op, int ele_size, const int &tree_depth, bool for_init)
{
    TreeNode *sub_node = GenerateSyntaxTree(-1, ele_size, tree_depth - 1, for_init);
    if (sub_node == NULL)
        return NULL;
    TreeNode *res_node = new TreeNode(op, -1, ele_size, tree_depth);
    res_node->SetSubtree(sub_node);
    res_node = node_manager_->InsertNode(res_node);
    return res_node;
}

TreeNode *Btor2Instance::ConstructNode_BVn_x_BVn_TO_BVn(TreeNode::oper op, int ele_size, const int &tree_depth, bool for_init)
{
    vector<int> depthes(2);
    RandDepthes(depthes, tree_depth - 1);
    TreeNode *sub_node_0 = GenerateSyntaxTree(-1, ele_size, depthes[0], for_init);
    if (sub_node_0 == NULL)
        return NULL;
    TreeNode *sub_node_1 = GenerateSyntaxTree(-1, ele_size, depthes[1], for_init);
    if (sub_node_1 == NULL)
        return NULL;
    TreeNode *res_node = new TreeNode(op, -1, ele_size, tree_depth);
    res_node->SetSubtree(sub_node_0, sub_node_1);
    res_node = node_manager_->InsertNode(res_node);
    return res_node;
}

TreeNode *Btor2Instance::ConstructNode_BVn_CONCAT(int ele_size, const int &tree_depth, bool for_init)
{
    vector<int> depthes(2);
    RandDepthes(depthes, tree_depth - 1);
    vector<int> possible_sizes;
    GetPossibleSizes(possible_sizes);
    for (auto it = possible_sizes.begin(); it != possible_sizes.end(); ++it)
    {
        if ((*it) >= ele_size)
            continue;
        int n = (*it);
        int m = ele_size - n;
        assert((n > 0) && (m > 0) && ((n + m) == ele_size));
        TreeNode *sub_node_0 = GenerateSyntaxTree(-1, n, depthes[0], for_init);
        if (sub_node_0 == NULL)
            return NULL;
        TreeNode *sub_node_1 = GenerateSyntaxTree(-1, m, depthes[1], for_init);
        if (sub_node_1 == NULL)
            return NULL;
        TreeNode *res_node = new TreeNode(TreeNode::CONCAT, -1, ele_size, tree_depth);
        res_node->SetSubtree(sub_node_0, sub_node_1);
        res_node = node_manager_->InsertNode(res_node);
        return res_node;
    }
    return NULL;
}

TreeNode *Btor2Instance::ConstructNode_BVn_READ(int ele_size, const int &tree_depth, bool for_init)
{
    vector<int> rand_indexes((config_->candidate_sizes_).size());
    RandIndexes(rand_indexes);
    const vector<int> &possible_sizes = config_->candidate_sizes_;
    for (auto it = rand_indexes.begin(); it != rand_indexes.end(); ++it)
    {
        TreeNode *src_arr = GenerateSyntaxTree(possible_sizes[*it], ele_size, ((tree_depth + 1) * 5) / 8, for_init);
        if (src_arr == NULL)
            continue;
        TreeNode *idx_node = GenerateSyntaxTree(-1, possible_sizes[*it], tree_depth - 1, for_init);
        if (idx_node == NULL)
            continue;
        TreeNode *res_node = new TreeNode(TreeNode::READ, -1, ele_size, tree_depth);
        res_node->SetSubtree(src_arr, idx_node);
        res_node = node_manager_->InsertNode(res_node);
        return res_node;
    }
    return NULL;
}

TreeNode *Btor2Instance::ConstructNode_BVn_ITE(int ele_size, const int &tree_depth, bool for_init)
{
    vector<int> depthes(3);
    RandDepthes(depthes, tree_depth - 1);
    TreeNode *i_node = GenerateSyntaxTree(-1, 1, depthes[0], for_init);
    if (i_node == NULL)
        return NULL;
    TreeNode *t_node = GenerateSyntaxTree(-1, ele_size, depthes[1], for_init);
    if (t_node == NULL)
        return NULL;
    TreeNode *e_node = GenerateSyntaxTree(-1, ele_size, depthes[2], for_init);
    if (e_node == NULL)
        return NULL;
    TreeNode *res_node = new TreeNode(TreeNode::ITE, -1, ele_size, tree_depth);
    res_node->SetSubtree(i_node, t_node, e_node);
    res_node = node_manager_->InsertNode(res_node);
    return res_node;
}

void Btor2Instance::Print()
{
    if (bad_properties_.empty() || TransitionAllNull())
    {
        cout << "*** FuzzBtor2: Construct completely Failed." << endl;
        return;
    }
    config_->Print();
    Btor2Instance::line_num_ = 1;
    // print init exprs by topo sort of var dependency
    for (auto it = init_topo_seq_.rbegin(); it != init_topo_seq_.rend(); ++it)
    {
        TreeNode *init_node;
        if ((*it)->GetIdxSize() == -1)
        {
            int idx = std::distance(bv_state_varibles_.begin(),
                                    std::find(bv_state_varibles_.begin(), bv_state_varibles_.end(), *it));
            init_node = bv_init_[idx];
        }
        else
        {
            int idx = std::distance(arr_state_varibles_.begin(),
                                    std::find(arr_state_varibles_.begin(), arr_state_varibles_.end(), *it));
            init_node = arr_init_[idx];
        }
        if (init_node != NULL)
            init_node->Print(node_manager_);
    }
    for (int i = 0; i < bv_state_varibles_.size(); ++i)
    {
        TreeNode *init_node = bv_init_[i];
        (bv_state_varibles_[i])->Print(node_manager_);
        if (init_node != NULL)
        {
            cout << line_num_ << " init "
                 << node_manager_->GetSortLineId(-1, bv_state_varibles_[i]->GetEleSize())
                 << ' ' << bv_state_varibles_[i]->GetLineId() << ' ' << init_node->GetLineId() << endl;
            (Btor2Instance::line_num_)++;
        }
        if (bv_transitions_[i] != NULL)
        {
            (bv_transitions_[i])->Print(node_manager_);
            cout << line_num_ << " next "
                 << node_manager_->GetSortLineId(-1, bv_state_varibles_[i]->GetEleSize())
                 << ' ' << bv_state_varibles_[i]->GetLineId() << ' ' << bv_transitions_[i]->GetLineId() << endl;
            (Btor2Instance::line_num_)++;
        }
    }
    for (int i = 0; i < arr_state_varibles_.size(); ++i)
    {
        TreeNode *init_node = arr_init_[i];
        if (init_node != NULL)
            init_node->Print(node_manager_);
        (arr_state_varibles_[i])->Print(node_manager_);
        if (init_node != NULL)
        {
            cout << line_num_ << " init "
                 << node_manager_->GetSortLineId(arr_state_varibles_[i]->GetIdxSize(), arr_state_varibles_[i]->GetEleSize())
                 << ' ' << arr_state_varibles_[i]->GetLineId() << ' ' << init_node->GetLineId() << endl;
            (Btor2Instance::line_num_)++;
        }
        if (arr_transitions_[i] != NULL)
        {
            (arr_transitions_[i])->Print(node_manager_);
            cout << line_num_ << " next "
                 << node_manager_->GetSortLineId(arr_state_varibles_[i]->GetIdxSize(), arr_state_varibles_[i]->GetEleSize())
                 << ' ' << arr_state_varibles_[i]->GetLineId() << ' ' << arr_transitions_[i]->GetLineId() << endl;
            (Btor2Instance::line_num_)++;
        }
    }
    for (int i = 0; i < bad_properties_.size(); ++i)
    {
        (bad_properties_[i])->Print(node_manager_);
        cout << line_num_ << " bad " << bad_properties_[i]->GetLineId() << endl;
        (Btor2Instance::line_num_)++;
    }
    for (int i = 0; i < constraints_.size(); ++i)
    {
        (constraints_[i])->Print(node_manager_);
        cout << line_num_ << " constraint " << constraints_[i]->GetLineId() << endl;
        (Btor2Instance::line_num_)++;
    }
}

void Configuration::Print()
{
    cout << "; seed for random number: " << seed_ << endl;
    cout << "; maximal depth of syntax trees: " << max_depth_ << endl;
    cout << "; number of bit-vector state variables: " << bv_state_vars_num_ << endl;
    cout << "; number of array state variables: " << arr_state_vars_num_ << endl;
    cout << "; maximum number of input variables: " << max_input_num_ << endl;
    cout << "; number of bad properties: " << bad_property_num_ << endl;
    cout << "; number of constraints: " << constraint_num_ << endl;
    cout << "; candidate sizes:";
    for (auto it = candidate_sizes_.begin(); it != candidate_sizes_.end(); ++it)
        cout << ' ' << *it;
    cout << endl;
}

void StateDependencies::AddDependedByCur(TreeNode *v)
{ // cur depends on v
    if (var2idx_.find(cur_target_state_) == var2idx_.end())
    {
        var2idx_[cur_target_state_] = depend_graph_.size();
        depend_graph_.push_back(vector<TreeNode *>());
    }
    vector<TreeNode *> &tmp_ref = depend_graph_[var2idx_[cur_target_state_]];
    if (std::find(tmp_ref.begin(), tmp_ref.end(), v) == tmp_ref.end())
        tmp_ref.push_back(v);
}

void StateDependencies::DestroyIntoTopoSort(vector<TreeNode *> &topo_sort)
{
    if (var2idx_.empty())
        return;
    auto it = var2idx_.begin();
    for (; it != var2idx_.end(); ++it)
    {
        if (IsNotDepended(it->first))
            break;
    }
    assert(it != var2idx_.end());
    topo_sort.push_back(it->first);
    (depend_graph_[it->second]).clear();
    var2idx_.erase(it);
    DestroyIntoTopoSort(topo_sort);
}

bool StateDependencies::DFS_SearchDepend(TreeNode *v0, TreeNode *v1)
{ // check whether v0 depends on v1
    if (var2idx_.find(v0) == var2idx_.end())
        return false;
    if (v0 == v1)
        return true;
    const vector<TreeNode *> &v_depend = depend_graph_[var2idx_[v0]];
    for (auto it = v_depend.begin(); it != v_depend.end(); ++it)
    {
        if ((*it) == v0)
            continue;
        if (DFS_SearchDepend(*it, v1))
            return true;
    }

    return false;
}

bool StateDependencies::IsNotDepended(TreeNode *v)
{
    for (auto it_0 = var2idx_.begin(); it_0 != var2idx_.end(); ++it_0)
    {
        TreeNode *src = it_0->first;
        const vector<TreeNode *> &dst_s = depend_graph_[it_0->second];
        for (auto it_1 = dst_s.begin(); it_1 != dst_s.end(); ++it_1)
            if ((*it_1) != src && (*it_1) == v)
                return false;
    }
    return true;
}