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

Btor2Instance::Btor2Instance(int argc, char **argv)
    : config_(NULL), node_manager_(NULL)
{
    config_ = new Configuration();
    config_completed_ = GetConfigInfo(argc, argv, config_);
    if (config_completed_ == false)
        return;
    node_manager_ = new NodeManager();
    for (int i = 0; i < ((config_->bv_vars_size_).size()); i++)
    {
        TreeNode *node = new TreeNode(TreeNode::BITVEC, (config_->bv_vars_size_)[i], (config_->bv_vars_name_)[i]);
        node_manager_->InsertNode(node);
        if (RandLessThan(11) < 3)
        {
            input_varibles_.push_back(node);
            node->SetAsInput();
        }
        else
        {
            state_varibles_.push_back(node);
            if (RandLessThan(5) < 4)
            {
                TreeNode *init_val = ConstructNode_CONSTANT(node->GetEleSize());
                node->SetInitValue(init_val);
            }
        }
    }
    for (int i = 0; i < ((config_->arr_vars_idx_size_).size()); i++)
    {
        TreeNode *node = new TreeNode(TreeNode::ARRAY,
                                      (config_->arr_vars_idx_size_)[i], (config_->arr_vars_ele_size_)[i], (config_->arr_vars_name_)[i]);
        node_manager_->InsertNode(node);
        if (RandLessThan(16) < 1)
        {
            input_varibles_.push_back(node);
            node->SetAsInput();
        }
        else
            state_varibles_.push_back(node);
    }
    is_pure_bv_ = config_->arr_vars_name_.empty();
    SetRandSeed(config_->seed_);

    for (auto it = state_varibles_.begin(); it != state_varibles_.end(); ++it)
    {
        if ((*it)->IsArr())
        {
            base_possible_sizes_.push_back((*it)->GetIdxSize());
            base_possible_sizes_.push_back((*it)->GetEleSize());
        }
        else
        {
            base_possible_sizes_.push_back((*it)->GetEleSize());
            base_possible_sizes_.push_back((*it)->GetEleSize());
        }
    }
    for (auto it = input_varibles_.begin(); it != input_varibles_.end(); ++it)
    {
        if ((*it)->IsArr())
        {
            base_possible_sizes_.push_back((*it)->GetIdxSize());
            base_possible_sizes_.push_back((*it)->GetEleSize());
        }
        else
        {
            base_possible_sizes_.push_back((*it)->GetEleSize());
            base_possible_sizes_.push_back((*it)->GetEleSize());
        }
    }
    std::sort(base_possible_sizes_.begin(), base_possible_sizes_.end());

    // construct syntax trees for transition
    for (auto it = state_varibles_.begin(); it != state_varibles_.end();)
    {
        TreeNode *tran = GenerateSyntaxTree((*it)->GetIdxSize(), (*it)->GetEleSize(), config_->tree_depth_);
        if (tran != NULL)
        {
            transitions_.push_back(tran);
            ++it;
        }
        else
        {
            input_varibles_.push_back(*it);
            (*it)->SetAsInput();
            it = state_varibles_.erase(it);
        }
    }

    // construct syntax trees for bad properties
    for (int i = 0; i < (config_->bad_property_num_); ++i)
    {
        TreeNode *bad_p = GenerateSyntaxTree(-1, 1, config_->tree_depth_);
        if (bad_p != NULL)
            bad_properties_.push_back(bad_p);
    }

    // construct syntax trees for constraints
    for (int i = 0; i < (config_->constraint_num_); ++i)
    {
        TreeNode *constraint = GenerateSyntaxTree(-1, 1, config_->tree_depth_);
        if (constraint != NULL)
            constraints_.push_back(constraint);
    }
}

Btor2Instance::~Btor2Instance()
{
    if (config_ != NULL)
        delete config_;
    if (node_manager_ != NULL)
        delete node_manager_;
}

TreeNode *Btor2Instance::GenerateSyntaxTree(int idx_size, int ele_size, int tree_depth)
{
    TreeNode *dag_node = NULL;
    if (RandLessThan(10) < 3)
    {
        dag_node = node_manager_->GetNode(idx_size, ele_size, tree_depth);
        if (dag_node != NULL)
            return dag_node;
    }
    if (tree_depth == 1)
    {
        if (RandLessThan(10) > 2 || idx_size != -1)
        { // variables
            while (true)
            {
                TreeNode *tmp_node = node_manager_->GetNode(idx_size, ele_size, 1);
                if (tmp_node->IsVariable())
                    return tmp_node;
            }
        }
        else
        { // new constant
            return ConstructNode_CONSTANT(ele_size);
        }
    }
    assert(idx_size == -1 || idx_size > 0);
    assert(ele_size > 0);
    if (idx_size > 0)
    { // arr
        return ConstructNode_ARR_WRITE(idx_size, ele_size, tree_depth);
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
                res_node = ConstructNode_BOOL_SLICE(tree_depth);
                break;
            }
            case TreeNode::NOT:
            case TreeNode::INC:
            case TreeNode::DEC:
            case TreeNode::NEG:
            {
                res_node = ConstructNode_BOOL_TO_BOOL(possible_ops.back(), tree_depth);
                break;
            }
            case TreeNode::REDAND:
            case TreeNode::REDOR:
            case TreeNode::REDXOR:
            {
                res_node = ConstructNode_BVn_TO_BOOL(possible_ops.back(), tree_depth);
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
                res_node = ConstructNode_BOOL_x_BOOL_TO_BOOL(possible_ops.back(), tree_depth);
                break;
            }
            case TreeNode::EQ:
            case TreeNode::NEQ:
            {
                res_node = ConstructNode_S_x_S_TO_BOOL(possible_ops.back(), tree_depth);
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
                res_node = ConstructNode_BVn_x_BVn_TO_BOOL(possible_ops.back(), tree_depth);
                break;
            }
            case TreeNode::READ:
            {
                res_node = ConstructNode_BOOL_READ(tree_depth);
                break;
            }
            case TreeNode::ITE:
            {
                res_node = ConstructNode_BOOL_ITE(tree_depth);
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
                res_node = ConstructNode_BVn_EXT(possible_ops.back(), ele_size, tree_depth);
                break;
            }
            case TreeNode::SLICE:
            {
                res_node = ConstructNode_BVn_SLICE(ele_size, tree_depth);
                break;
            }
            case TreeNode::NOT:
            case TreeNode::INC:
            case TreeNode::DEC:
            case TreeNode::NEG:
            {
                res_node = ConstructNode_BVn_TO_BVn(possible_ops.back(), ele_size, tree_depth);
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
                res_node = ConstructNode_BVn_x_BVn_TO_BVn(possible_ops.back(), ele_size, tree_depth);
                break;
            }
            case TreeNode::CONCAT:
            {
                res_node = ConstructNode_BVn_CONCAT(ele_size, tree_depth);
                break;
            }
            case TreeNode::READ:
            {
                res_node = ConstructNode_BVn_READ(ele_size, tree_depth);
                break;
            }
            case TreeNode::ITE:
            {
                res_node = ConstructNode_BVn_ITE(ele_size, tree_depth);
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

void Btor2Instance::GetPossibleOps(vector<TreeNode::oper> &possible_ops, bool is_bool)
{
    possible_ops.clear();
    if (is_bool)
    {
        TreeNode::oper candidate[] = {
            TreeNode::SLICE,
            TreeNode::NOT, TreeNode::INC, TreeNode::DEC, TreeNode::NEG,
            TreeNode::REDAND, TreeNode::REDOR, TreeNode::REDXOR,
            TreeNode::IFF, TreeNode::IMPLIES,
            TreeNode::EQ, TreeNode::NEQ,
            TreeNode::SGT, TreeNode::UGT, TreeNode::SGTE, TreeNode::UGTE, TreeNode::SLT, TreeNode::ULT, TreeNode::SLTE, TreeNode::ULTE, TreeNode::SADDO, TreeNode::UADDO, TreeNode::SDIVO, TreeNode::UDIVO, TreeNode::SMULO, TreeNode::UMULO, TreeNode::SSUBO, TreeNode::USUBO,
            TreeNode::AND, TreeNode::NAND, TreeNode::NOR, TreeNode::OR, TreeNode::XNOR, TreeNode::XOR, TreeNode::ROL, TreeNode::ROR, TreeNode::SLL, TreeNode::SRA, TreeNode::SRL, TreeNode::ADD, TreeNode::MUL, TreeNode::SDIV, TreeNode::UDIV, TreeNode::SMOD, TreeNode::SREM, TreeNode::UREM, TreeNode::SUB,
            TreeNode::READ,
            TreeNode::ITE};
        possible_ops = vector<TreeNode::oper>(std::begin(candidate), std::end(candidate));
        std::random_shuffle(possible_ops.begin(), possible_ops.end());
    }
    else
    {
        TreeNode::oper candidate[] = {
            TreeNode::SEXT, TreeNode::UEXT,
            TreeNode::SLICE,
            TreeNode::NOT, TreeNode::INC, TreeNode::DEC, TreeNode::NEG,
            TreeNode::AND, TreeNode::NAND, TreeNode::NOR, TreeNode::OR, TreeNode::XNOR, TreeNode::XOR, TreeNode::ROL, TreeNode::ROR, TreeNode::SLL, TreeNode::SRA, TreeNode::SRL, TreeNode::ADD, TreeNode::MUL, TreeNode::SDIV, TreeNode::UDIV, TreeNode::SMOD, TreeNode::SREM, TreeNode::UREM, TreeNode::SUB,
            TreeNode::CONCAT,
            TreeNode::READ,
            TreeNode::ITE};
        possible_ops = vector<TreeNode::oper>(std::begin(candidate), std::end(candidate));
        std::random_shuffle(possible_ops.begin(), possible_ops.end());
    }
}

void Btor2Instance::GetPossibleSizes(vector<int> &possible_sizes)
{
    possible_sizes = vector<int>(base_possible_sizes_); // base_possible_sizes_ is sorted
    int tmp_max_size = possible_sizes.back();
    int other_sizes_num = (possible_sizes.size() + 1) / 2;
    for (int i = 0; i < other_sizes_num; ++i)
    {
        possible_sizes.push_back(RandLessThan(tmp_max_size) + 1);
    }
    std::random_shuffle(possible_sizes.begin(), possible_sizes.end());
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

TreeNode *Btor2Instance::ConstructNode_ARR_WRITE(int idx_size, int ele_size, const int &tree_depth)
{
    TreeNode *src_arr = GenerateSyntaxTree(idx_size, ele_size, ((tree_depth + 1) * 5) / 8);
    if (src_arr == NULL)
        return NULL;
    vector<int> depthes(2);
    RandDepthes(depthes, tree_depth - 1);
    TreeNode *idx_node = GenerateSyntaxTree(-1, idx_size, depthes[0]);
    if (idx_node == NULL)
    {
        delete src_arr;
        return NULL;
    }
    TreeNode *ele_node = GenerateSyntaxTree(-1, ele_size, depthes[1]);
    if (ele_node == NULL)
    {
        delete src_arr;
        delete idx_node;
        return NULL;
    }
    TreeNode *res_node = new TreeNode(TreeNode::WRITE, idx_size, ele_size, tree_depth);
    res_node->SetSubtree(src_arr, idx_node, ele_node);
    res_node = node_manager_->InsertNode(res_node);
    return res_node;
}

TreeNode *Btor2Instance::ConstructNode_BOOL_SLICE(const int &tree_depth)
{
    vector<int> possible_sizes;
    GetPossibleSizes(possible_sizes);
    for (auto it = possible_sizes.begin(); it != possible_sizes.end(); ++it)
    {
        TreeNode *sub_node = GenerateSyntaxTree(-1, *it, tree_depth - 1);
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

TreeNode *Btor2Instance::ConstructNode_BOOL_TO_BOOL(TreeNode::oper op, const int &tree_depth)
{
    // std::random_shuffle(possible_sizes_.begin(), possible_sizes_.end());
    TreeNode *sub_node = GenerateSyntaxTree(-1, 1, tree_depth - 1);
    if (sub_node == NULL)
        return NULL;
    TreeNode *res_node = new TreeNode(op, -1, 1, tree_depth);
    res_node->SetSubtree(sub_node);
    res_node = node_manager_->InsertNode(res_node);
    return res_node;
}

TreeNode *Btor2Instance::ConstructNode_BVn_TO_BOOL(TreeNode::oper op, const int &tree_depth)
{
    vector<int> possible_sizes;
    GetPossibleSizes(possible_sizes);
    for (auto it = possible_sizes.begin(); it != possible_sizes.end(); ++it)
    {
        TreeNode *sub_node = GenerateSyntaxTree(-1, *it, tree_depth - 1);
        if (sub_node == NULL)
            continue;
        TreeNode *res_node = new TreeNode(op, -1, 1, tree_depth);
        res_node->SetSubtree(sub_node);
        res_node = node_manager_->InsertNode(res_node);
        return res_node;
    }
    return NULL;
}

TreeNode *Btor2Instance::ConstructNode_BOOL_x_BOOL_TO_BOOL(TreeNode::oper op, const int &tree_depth)
{
    vector<int> depthes(2);
    RandDepthes(depthes, tree_depth - 1);
    TreeNode *sub_node_0 = GenerateSyntaxTree(-1, 1, depthes[0]);
    if (sub_node_0 == NULL)
        return NULL;
    TreeNode *sub_node_1 = GenerateSyntaxTree(-1, 1, depthes[1]);
    if (sub_node_1 == NULL)
    {
        delete sub_node_0;
        return NULL;
    }
    TreeNode *res_node = new TreeNode(op, -1, 1, tree_depth);
    res_node->SetSubtree(sub_node_0, sub_node_1);
    res_node = node_manager_->InsertNode(res_node);
    return res_node;
}

TreeNode *Btor2Instance::ConstructNode_S_x_S_TO_BOOL(TreeNode::oper op, const int &tree_depth)
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
            res_node = ConstructNode_BOOL_x_BOOL_TO_BOOL(op, tree_depth);
            break;
        case 1:
            res_node = ConstructNode_BVn_x_BVn_TO_BOOL(op, tree_depth);
            break;
        case 2:
            res_node = ConstructNode_ARR_x_ARR_TO_BOOL(op, tree_depth);
            break;
        }
        if (res_node != NULL)
            break;
    }
    return res_node;
}

TreeNode *Btor2Instance::ConstructNode_BVn_x_BVn_TO_BOOL(TreeNode::oper op, const int &tree_depth)
{
    vector<int> depthes(2);
    RandDepthes(depthes, tree_depth - 1);
    vector<int> possible_sizes;
    GetPossibleSizes(possible_sizes);
    for (auto it = possible_sizes.begin(); it != possible_sizes.end(); ++it)
    {
        TreeNode *sub_node_0 = GenerateSyntaxTree(-1, *it, depthes[0]);
        if (sub_node_0 == NULL)
            continue;
        TreeNode *sub_node_1 = GenerateSyntaxTree(-1, *it, depthes[1]);
        if (sub_node_1 == NULL)
        {
            delete sub_node_0;
            continue;
        }
        TreeNode *res_node = new TreeNode(op, -1, 1, tree_depth);
        res_node->SetSubtree(sub_node_0, sub_node_1);
        res_node = node_manager_->InsertNode(res_node);
        return res_node;
    }
    return NULL;
}

TreeNode *Btor2Instance::ConstructNode_ARR_x_ARR_TO_BOOL(TreeNode::oper op, const int &tree_depth)
{
    vector<int> depthes(2);
    RandDepthes(depthes, tree_depth - 1);
    vector<int> indexes(config_->arr_vars_idx_size_.size());
    RandIndexes(indexes);
    for (auto it = indexes.begin(); it != indexes.end(); ++it)
    {
        TreeNode *sub_node_0 = GenerateSyntaxTree((config_->arr_vars_idx_size_)[*it], (config_->arr_vars_ele_size_)[*it], depthes[0]);
        if (sub_node_0 == NULL)
            continue;
        TreeNode *sub_node_1 = GenerateSyntaxTree((config_->arr_vars_idx_size_)[*it], (config_->arr_vars_ele_size_)[*it], depthes[1]);
        if (sub_node_1 == NULL)
        {
            delete sub_node_0;
            continue;
        }
        TreeNode *res_node = new TreeNode(op, -1, 1, tree_depth);
        res_node->SetSubtree(sub_node_0, sub_node_1);
        res_node = node_manager_->InsertNode(res_node);
        return res_node;
    }
    return NULL;
}

TreeNode *Btor2Instance::ConstructNode_BOOL_READ(const int &tree_depth)
{
    vector<int> possible_idx_size;
    for (int i = 0; i < config_->arr_vars_ele_size_.size(); ++i)
    {
        if ((config_->arr_vars_ele_size_)[i] == 1)
            possible_idx_size.push_back((config_->arr_vars_idx_size_)[i]);
    }
    if (possible_idx_size.empty())
        return NULL;
    std::random_shuffle(possible_idx_size.begin(), possible_idx_size.end());
    for (auto it = possible_idx_size.begin(); it != possible_idx_size.end(); ++it)
    {
        TreeNode *src_arr = GenerateSyntaxTree(*it, 1, ((tree_depth + 1) * 5) / 8);
        if (src_arr == NULL)
            continue;
        TreeNode *idx_node = GenerateSyntaxTree(-1, *it, tree_depth - 1);
        if (idx_node == NULL)
        {
            delete src_arr;
            continue;
        }
        TreeNode *res_node = new TreeNode(TreeNode::READ, *it, 1, tree_depth);
        res_node->SetSubtree(src_arr, idx_node);
        res_node = node_manager_->InsertNode(res_node);
        return res_node;
    }
    return NULL;
}

TreeNode *Btor2Instance::ConstructNode_BOOL_ITE(const int &tree_depth)
{
    vector<int> depthes(3);
    RandDepthes(depthes, tree_depth - 1);
    TreeNode *i_node = GenerateSyntaxTree(-1, 1, depthes[0]);
    if (i_node == NULL)
        return NULL;
    TreeNode *t_node = GenerateSyntaxTree(-1, 1, depthes[1]);
    if (t_node == NULL)
    {
        delete i_node;
        return NULL;
    }
    TreeNode *e_node = GenerateSyntaxTree(-1, 1, depthes[2]);
    if (e_node == NULL)
    {
        delete i_node;
        delete t_node;
        return NULL;
    }
    TreeNode *res_node = new TreeNode(TreeNode::ITE, -1, 1, tree_depth);
    res_node->SetSubtree(i_node, t_node, e_node);
    res_node = node_manager_->InsertNode(res_node);
    return res_node;
}

TreeNode *Btor2Instance::ConstructNode_BVn_EXT(TreeNode::oper op, int ele_size, const int &tree_depth)
{
    vector<int> possible_sizes;
    GetPossibleSizes(possible_sizes);
    for (auto it = possible_sizes.begin(); it != possible_sizes.end(); ++it)
    {
        if ((*it) >= ele_size)
            continue;
        TreeNode *sub_node = GenerateSyntaxTree(-1, *it, tree_depth - 1);
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

TreeNode *Btor2Instance::ConstructNode_BVn_SLICE(int ele_size, const int &tree_depth)
{
    vector<int> possible_sizes;
    GetPossibleSizes(possible_sizes);
    for (auto it = possible_sizes.begin(); it != possible_sizes.end(); ++it)
    {
        if ((*it) <= ele_size)
            continue;
        TreeNode *sub_node = GenerateSyntaxTree(-1, *it, tree_depth - 1);
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

TreeNode *Btor2Instance::ConstructNode_BVn_TO_BVn(TreeNode::oper op, int ele_size, const int &tree_depth)
{
    TreeNode *sub_node = GenerateSyntaxTree(-1, ele_size, tree_depth - 1);
    if (sub_node == NULL)
        return NULL;
    TreeNode *res_node = new TreeNode(op, -1, ele_size, tree_depth);
    res_node->SetSubtree(sub_node);
    res_node = node_manager_->InsertNode(res_node);
    return res_node;
}

TreeNode *Btor2Instance::ConstructNode_BVn_x_BVn_TO_BVn(TreeNode::oper op, int ele_size, const int &tree_depth)
{
    vector<int> depthes(2);
    RandDepthes(depthes, tree_depth - 1);
    TreeNode *sub_node_0 = GenerateSyntaxTree(-1, ele_size, depthes[0]);
    if (sub_node_0 == NULL)
        return NULL;
    TreeNode *sub_node_1 = GenerateSyntaxTree(-1, ele_size, depthes[1]);
    if (sub_node_1 == NULL)
    {
        delete sub_node_0;
        return NULL;
    }
    TreeNode *res_node = new TreeNode(op, -1, ele_size, tree_depth);
    res_node->SetSubtree(sub_node_0, sub_node_1);
    res_node = node_manager_->InsertNode(res_node);
    return res_node;
}

TreeNode *Btor2Instance::ConstructNode_BVn_CONCAT(int ele_size, const int &tree_depth)
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
        int m = ele_size - m;
        assert((n > 0) && (m > 0) && ((n + m) == ele_size));
        TreeNode *sub_node_0 = GenerateSyntaxTree(-1, n, depthes[0]);
        if (sub_node_0 == NULL)
            return NULL;
        TreeNode *sub_node_1 = GenerateSyntaxTree(-1, m, depthes[1]);
        if (sub_node_1 == NULL)
        {
            delete sub_node_0;
            return NULL;
        }
        TreeNode *res_node = new TreeNode(TreeNode::CONCAT, -1, ele_size, tree_depth);
        res_node->SetSubtree(sub_node_0, sub_node_1);
        res_node = node_manager_->InsertNode(res_node);
        return res_node;
    }
    return NULL;
}

TreeNode *Btor2Instance::ConstructNode_BVn_READ(int ele_size, const int &tree_depth)
{
    vector<int> possible_idx_size;
    for (int i = 0; i < config_->arr_vars_ele_size_.size(); ++i)
    {
        if ((config_->arr_vars_ele_size_)[i] == ele_size)
            possible_idx_size.push_back((config_->arr_vars_idx_size_)[i]);
    }
    if (possible_idx_size.empty())
        return NULL;
    std::random_shuffle(possible_idx_size.begin(), possible_idx_size.end());
    for (auto it = possible_idx_size.begin(); it != possible_idx_size.end(); ++it)
    {
        TreeNode *src_arr = GenerateSyntaxTree(*it, ele_size, ((tree_depth + 1) * 5) / 8);
        if (src_arr == NULL)
            continue;
        TreeNode *idx_node = GenerateSyntaxTree(-1, *it, tree_depth - 1);
        if (idx_node == NULL)
        {
            delete src_arr;
            continue;
        }
        TreeNode *res_node = new TreeNode(TreeNode::READ, *it, ele_size, tree_depth);
        res_node->SetSubtree(src_arr, idx_node);
        res_node = node_manager_->InsertNode(res_node);
        return res_node;
    }
    return NULL;
}

TreeNode *Btor2Instance::ConstructNode_BVn_ITE(int ele_size, const int &tree_depth)
{
    vector<int> depthes(3);
    RandDepthes(depthes, tree_depth - 1);
    TreeNode *i_node = GenerateSyntaxTree(-1, 1, depthes[0]);
    if (i_node == NULL)
        return NULL;
    TreeNode *t_node = GenerateSyntaxTree(-1, ele_size, depthes[1]);
    if (t_node == NULL)
    {
        delete i_node;
        return NULL;
    }
    TreeNode *e_node = GenerateSyntaxTree(-1, ele_size, depthes[2]);
    if (e_node == NULL)
    {
        delete i_node;
        delete t_node;
        return NULL;
    }
    TreeNode *res_node = new TreeNode(TreeNode::ITE, -1, ele_size, tree_depth);
    res_node->SetSubtree(i_node, t_node, e_node);
    res_node = node_manager_->InsertNode(res_node);
    return res_node;
}

void Btor2Instance::Print()
{
    assert(!(state_varibles_.empty() || bad_properties_.empty()));
    Btor2Instance::line_num_ = 1;
    for (int i = 0; i < state_varibles_.size(); ++i)
    {
        TreeNode *init_val_node = state_varibles_[i]->GetInitValue();
        if (init_val_node != NULL)
            init_val_node->Print(node_manager_);
        (state_varibles_[i])->Print(node_manager_);
        if (init_val_node != NULL)
        {
            cout << line_num_ << " init "
                 << node_manager_->GetSortLineId(state_varibles_[i]->GetIdxSize(), state_varibles_[i]->GetEleSize())
                 << ' ' << state_varibles_[i]->GetLineId() << ' ' << init_val_node->GetLineId() << endl;
            (Btor2Instance::line_num_)++;
        }
        (transitions_[i])->Print(node_manager_);
        cout << line_num_ << " next "
             << node_manager_->GetSortLineId(state_varibles_[i]->GetIdxSize(), state_varibles_[i]->GetEleSize())
             << ' ' << state_varibles_[i]->GetLineId() << ' ' << transitions_[i]->GetLineId() << endl;
        (Btor2Instance::line_num_)++;
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