#include "syntax_tree.h"
#include <cstring>
#include <cassert>
#include <vector>
#include <algorithm>
#include <iostream>
#include "random.h"
#include "generator.h"
#include "util.h"

using std::cout;
using std::endl;

TreeNode::TreeNode(int val, int ele_size)
    : op_(CONSTANT), idx_size_(-1), ele_size_(ele_size), depth_(1), line_id_(-1), var_name_(""), is_input_var_(false), init_value_(NULL)
{
    const_val_ = new bool[ele_size];
    if (val < 0)
    {
        for (int i = 0; i < ele_size; ++i)
            const_val_[i] = true;
    }
    else
    {
        memset(const_val_, 0, sizeof(bool) * ele_size_);
        for (int i = 0; i < std::min((unsigned long)(ele_size), sizeof(int)); i++)
        {
            if ((val % 2) != 0)
                const_val_[i] = true;
            val = (val >> 2);
        }
    }
}

TreeNode::TreeNode(int ele_size)
    : op_(CONSTANT), idx_size_(-1), ele_size_(ele_size), depth_(1), line_id_(-1), var_name_(""), is_input_var_(false), init_value_(NULL)
{
    const_val_ = new bool[ele_size];
    for (int i = 0; i < ele_size; i++)
        const_val_[i] = (RandLessThan(2) != 0);
}

bool Identical(TreeNode *node_0, TreeNode *node_1)
{
    TreeNode::oper op = node_0->GetOper();
    if (op != (node_1->GetOper()))
        return false;
    if (((node_0->GetIdxSize()) != (node_1->GetIdxSize())) || ((node_0->GetEleSize()) != (node_1->GetEleSize())))
        return false;
    int depth = (node_0->GetDepth());
    if (depth != (node_1->GetDepth()))
        return false;
    if (depth == 1)
    {
        if (op == TreeNode::CONSTANT)
        {
            int ele_size = node_0->GetEleSize();
            for (int i = 0; i < ele_size; i++)
            {
                if (node_0->IthValueOfConstant(i) != node_1->IthValueOfConstant(i))
                    return false;
            }
        }
        else
        {
            if ((node_0->GetVarName()) != (node_1->GetVarName()))
                return false;
        }
    }
    int subtrees_num = node_0->NumOfSubtrees();
    assert(subtrees_num == (node_1->NumOfSubtrees()));
    int args_num = node_0->NumOfArgs();
    for (int i = 0; i < args_num; ++i)
    {
        if ((node_0->IthArg(i)) != (node_1->IthArg(i)))
            return false;
    }
    for (int i = 0; i < subtrees_num; ++i)
    {
        if (!Identical(node_0->IthSubtree(i), node_1->IthSubtree(i)))
            return false;
    }
    return true;
}

TreeNode *NodeManager::InsertNode(TreeNode *node)
{
    if (node == NULL)
        return NULL;
    if ((node->GetIdxSize()) < 0)
    { // bv
        int pos = 0;
        for (; pos < bv_size_.size(); ++pos)
        {
            if (bv_size_[pos] == (node->GetEleSize()))
                break;
        }
        if (pos == bv_size_.size())
        {
            bv_size_.push_back(node->GetEleSize());
            bv_nodes_.push_back(vector<TreeNode *>());
            bv_sort_line_id_.push_back(-1);
        }
        for (auto it = (bv_nodes_[pos]).begin(); it != (bv_nodes_[pos]).end(); ++it)
        {
            if (Identical(node, *it))
            {
                delete node;
                return (*it);
            }
        }
        bv_nodes_[pos].push_back(node);
        return node;
    }
    else
    { // arr
        int pos = 0;
        for (; pos < arr_sort_.size(); ++pos)
        {
            if (((arr_sort_[pos]).first == (node->GetIdxSize())) && (((arr_sort_[pos]).second) == ((node->GetEleSize()))))
                break;
        }
        if (pos == arr_sort_.size())
        {
            arr_sort_.push_back(std::make_pair(node->GetIdxSize(), node->GetEleSize()));
            arr_nodes_.push_back(vector<TreeNode *>());
            arr_sort_line_id_.push_back(-1);
        }
        for (auto it = (arr_nodes_[pos]).begin(); it != (arr_nodes_[pos]).end(); ++it)
        {
            if (Identical(node, *it))
            {
                delete node;
                return (*it);
            }
        }
        arr_nodes_[pos].push_back(node);
        return node;
    }
}

TreeNode *NodeManager::GetNode(int idx_size, int ele_size, int tree_depth)
{
    if (idx_size < 0)
    { // bv
        for (int i = 0; i < bv_size_.size(); ++i)
        {
            if (bv_size_[i] == ele_size)
            {
                if (tree_depth == -1)
                    return bv_nodes_[i][RandLessThan((bv_nodes_[i]).size())];
                else
                {
                    std::random_shuffle((bv_nodes_[i]).begin(), (bv_nodes_[i]).end());
                    for (int j = 0; j < (bv_nodes_[i]).size(); ++j)
                    {
                        if ((bv_nodes_[i][j])->GetDepth() == tree_depth)
                            return bv_nodes_[i][j];
                    }
                }
            }
        }
    }
    else
    { // arr
        for (int i = 0; i < arr_sort_.size(); ++i)
        {
            if (arr_sort_[i].first == idx_size && arr_sort_[i].second == ele_size)
            {
                if (tree_depth == -1)
                    return (arr_nodes_[i])[RandLessThan((arr_nodes_[i]).size())];
                else
                {
                    std::random_shuffle((arr_nodes_[i]).begin(), (arr_nodes_[i]).end());
                    for (int j = 0; j < (arr_nodes_[i]).size(); ++j)
                    {
                        if ((arr_nodes_[i][j])->GetDepth() == tree_depth)
                            return arr_nodes_[i][j];
                    }
                }
            }
        }
    }
    return NULL;
}

int NodeManager::GetSortLineId(int idx_size, int ele_size)
{
    if (idx_size == -1)
    {
        auto target_it = find(bv_size_.begin(), bv_size_.end(), ele_size);
        if (target_it != bv_size_.end())
            return bv_sort_line_id_[std::distance(bv_size_.begin(), target_it)];
    }
    else
    {
        for (int i = 0; i < arr_sort_.size(); i++)
        {
            if (arr_sort_[i].first == idx_size && arr_sort_[i].second == ele_size)
                return arr_sort_line_id_[i];
        }
    }
    return -1;
}

void NodeManager::SetSortLineId(int idx_size, int ele_size, int line_id)
{
    if (idx_size == -1)
    {
        auto target_it = find(bv_size_.begin(), bv_size_.end(), ele_size);
        if (target_it != bv_size_.end())
            bv_sort_line_id_[std::distance(bv_size_.begin(), target_it)] = line_id;
    }
    else
    {
        for (int i = 0; i < arr_sort_.size(); i++)
        {
            if (arr_sort_[i].first == idx_size && arr_sort_[i].second == ele_size)
                arr_sort_line_id_[i] = line_id;
        }
    }
}

void NodeManager::PrintSort(int idx_size, int ele_size)
{
    if (this->IsPrintedSort(idx_size, ele_size))
        return;
    cout << Btor2Instance::line_num_ << " sort ";
    if (idx_size == -1)
    { // bv
        cout << "bitvec " << ele_size << endl;
        this->SetSortLineId(idx_size, ele_size, Btor2Instance::line_num_);
        ++(Btor2Instance::line_num_);
    }
    else
    { // arr
        this->PrintSort(-1, idx_size);
        this->PrintSort(-1, ele_size);
        cout << "array " << this->GetSortLineId(-1, idx_size) << ' ' << this->GetSortLineId(-1, ele_size) << endl;
        this->SetSortLineId(idx_size, ele_size, Btor2Instance::line_num_);
        ++(Btor2Instance::line_num_);
    }
}

void TreeNode::Print(NodeManager *node_manager)
{
    if (line_id_ != -1) // has been printed
        return;

    node_manager->PrintSort(idx_size_, ele_size_);
    if (this->IsVariable())
    {
        cout << Btor2Instance::line_num_ << ((this->IsInput()) ? " input " : " state ")
             << node_manager->GetSortLineId(idx_size_, ele_size_);
        if (RandLessThan(10) < 9) // may skip output of var name
            cout << ' ' << var_name_;
        cout << endl;
        this->SetLineId(Btor2Instance::line_num_);
        ++(Btor2Instance::line_num_);
        return;
    }
    else if (this->IsConstant())
    {
        cout << Btor2Instance::line_num_;
        int flag_num = RandLessThan(3);
        if (this->IsOne() && flag_num < 2)
        {
            cout << " one " << node_manager->GetSortLineId(idx_size_, ele_size_) << endl;
        }
        else if (this->IsOnes() && flag_num < 2)
        {
            cout << " ones " << node_manager->GetSortLineId(idx_size_, ele_size_) << endl;
        }
        else if (this->IsZero() && flag_num < 2)
        {
            cout << " zero " << node_manager->GetSortLineId(idx_size_, ele_size_) << endl;
        }

        else
        {
            switch (RandLessThan(3))
            {
            case 0:
            {
                cout << " const " << node_manager->GetSortLineId(idx_size_, ele_size_);
                PrintBoolArr(this->const_val_, ele_size_, BINARY);
                cout<<endl;
                break;
            }
            case 1:
            {
                cout << " constd " << node_manager->GetSortLineId(idx_size_, ele_size_);
                PrintBoolArr(this->const_val_, ele_size_, DECIMAL);
                cout<<endl;
                break;
            }
            case 2:
            {
                cout << " consth " << node_manager->GetSortLineId(idx_size_, ele_size_);
                PrintBoolArr(this->const_val_, ele_size_, HEXIMAL);
                cout<<endl;
                break;
            }
            }
        }
        this->SetLineId(Btor2Instance::line_num_);
        ++(Btor2Instance::line_num_);
        return;
    }
    for (int i = 0; i < this->NumOfSubtrees(); ++i)
        (this->IthSubtree(i))->Print(node_manager);
    cout << Btor2Instance::line_num_ << ' ';
    PrintOpName();
    cout << ' ' << node_manager->GetSortLineId(idx_size_, ele_size_) << ' ';
    for (int i = 0; i < this->NumOfSubtrees(); ++i)
        (this->IthSubtree(i))->GetLineId();
    for (int i = 0; i < this->NumOfArgs(); ++i)
        (this->IthSubtree(i))->IthArg(i);
    cout << endl;
    this->SetLineId(Btor2Instance::line_num_);
    ++(Btor2Instance::line_num_);
}

bool TreeNode::IsZero()
{
    if (const_val_ == NULL)
        return false;
    for (int i = 0; i < ele_size_; ++i)
        if (const_val_[i] != false)
            return false;
    return true;
}

bool TreeNode::IsOne()
{
    if (const_val_ == NULL)
        return false;
    if (const_val_[0] != true)
        return false;
    for (int i = 1; i < ele_size_; ++i)
        if (const_val_[i] != false)
            return false;
    return true;
}

bool TreeNode::IsOnes()
{
    if (const_val_ == NULL)
        return false;
    for (int i = 0; i < ele_size_; ++i)
        if (const_val_[i] != true)
            return false;
    return true;
}

void TreeNode::PrintOpName()
{
    switch (op_)
    {
    case BITVEC:
        cout << "bitvec";
        break;
    case ARRAY:
        cout << "array";
        break;
    case CONSTANT:
        cout << "constant";
        break;
    case WRITE:
        cout << "write";
        break;
    case SLICE:
        cout << "slice";
        break;
    case NOT:
        cout << "not";
        break;
    case INC:
        cout << "inc";
        break;
    case DEC:
        cout << "dec";
        break;
    case NEG:
        cout << "neg";
        break;
    case REDAND:
        cout << "redand";
        break;
    case REDOR:
        cout << "redor";
        break;
    case REDXOR:
        cout << "redxor";
        break;
    case IFF:
        cout << "iff";
        break;
    case IMPLIES:
        cout << "implies";
        break;
    case EQ:
        cout << "eq";
        break;
    case NEQ:
        cout << "neq";
        break;
    case SGT:
        cout << "sgt";
        break;
    case UGT:
        cout << "ugt";
        break;
    case SGTE:
        cout << "sgte";
        break;
    case UGTE:
        cout << "ugte";
        break;
    case SLT:
        cout << "slt";
        break;
    case ULT:
        cout << "ult";
        break;
    case SLTE:
        cout << "slte";
        break;
    case ULTE:
        cout << "ulte";
        break;
    case AND:
        cout << "and";
        break;
    case NAND:
        cout << "nand";
        break;
    case NOR:
        cout << "nor";
        break;
    case OR:
        cout << "or";
        break;
    case XNOR:
        cout << "xnor";
        break;
    case XOR:
        cout << "xor";
        break;
    case ROL:
        cout << "rol";
        break;
    case ROR:
        cout << "ror";
        break;
    case SLL:
        cout << "sll";
        break;
    case SRA:
        cout << "sra";
        break;
    case SRL:
        cout << "srl";
        break;
    case ADD:
        cout << "add";
        break;
    case MUL:
        cout << "mul";
        break;
    case SDIV:
        cout << "sdiv";
        break;
    case UDIV:
        cout << "udiv";
        break;
    case SMOD:
        cout << "smod";
        break;
    case SREM:
        cout << "srem";
        break;
    case UREM:
        cout << "urem";
        break;
    case SUB:
        cout << "sub";
        break;
    case SADDO:
        cout << "saddo";
        break;
    case UADDO:
        cout << "uaddo";
        break;
    case SDIVO:
        cout << "sdivo";
        break;
    case UDIVO:
        cout << "udivo";
        break;
    case SMULO:
        cout << "smulo";
        break;
    case UMULO:
        cout << "umulo";
        break;
    case SSUBO:
        cout << "ssubo";
        break;
    case USUBO:
        cout << "usubo";
        break;
    case READ:
        cout << "read";
        break;
    case ITE:
        cout << "ite";
        break;
    case SEXT:
        cout << "sext";
        break;
    case UEXT:
        cout << "uext";
        break;
    case CONCAT:
        cout << "concat";
        break;
    }
}