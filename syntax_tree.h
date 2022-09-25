#ifndef __SYNTAX_TREE_H__
#define __SYNTAX_TREE_H__

#include <cassert>
#include <vector>
#include <string>
#include <map>

using std::string;
using std::vector;

struct CompareSort
{
    bool operator()(const std::pair<int, int> &sort_0, const std::pair<int, int> &sort_1)
    {
        return (sort_0.first == sort_1.first) ? (sort_0.second < sort_1.second) : (sort_0.first < sort_1.first);
    }
};

class NodeManager;

class TreeNode
{
public:
    typedef enum
    {
        BITVEC,
        ARRAY,
        CONSTANT,
        WRITE,
        SLICE,
        NOT,
        INC,
        DEC,
        NEG,
        REDAND,
        REDOR,
        REDXOR,
        IFF,
        IMPLIES,
        EQ,
        NEQ,
        SGT,
        UGT,
        SGTE,
        UGTE,
        SLT,
        ULT,
        SLTE,
        ULTE,
        AND,
        NAND,
        NOR,
        OR,
        XNOR,
        XOR,
        ROL,
        ROR,
        SLL,
        SRA,
        SRL,
        ADD,
        MUL,
        SDIV,
        UDIV,
        SMOD,
        SREM,
        UREM,
        SUB,
        SADDO,
        UADDO,
        SDIVO,
        UDIVO,
        SMULO,
        UMULO,
        SSUBO,
        USUBO,
        READ,
        ITE,
        SEXT,
        UEXT,
        CONCAT
    } oper;

    TreeNode(oper op, int ele_size, string v_name)
        : op_(op), idx_size_(-1), ele_size_(ele_size), depth_(1), line_id_(-1), var_name_(v_name), const_val_(NULL), is_input_var_(false), init_value_(NULL) {}

    TreeNode(oper op, int idx_size, int ele_size, string v_name)
        : op_(op), idx_size_(idx_size), ele_size_(ele_size), depth_(1), line_id_(-1), var_name_(v_name), const_val_(NULL), is_input_var_(false), init_value_(NULL) {}

    TreeNode(oper op, int idx_size, int ele_size, int depth)
        : op_(op), idx_size_(idx_size), ele_size_(ele_size), depth_(depth), line_id_(-1), var_name_(""), const_val_(NULL), is_input_var_(false), init_value_(NULL) {}

    // constant node
    TreeNode(int val, int ele_size);
    TreeNode(int ele_size);

    ~TreeNode();

    inline int GetIdxSize() { return idx_size_; }
    inline int GetEleSize() { return ele_size_; }
    inline oper GetOper() { return op_; }
    inline int GetDepth() { return depth_; }
    inline string GetVarName() { return var_name_; }
    inline int NumOfSubtrees() { return sub_nodes_.size(); }
    inline int NumOfArgs() { return indexed_args_.size(); }
    inline TreeNode *IthSubtree(int idx)
    {
        return (idx >= 0 && idx < sub_nodes_.size()) ? (sub_nodes_[idx]) : NULL;
    }
    inline int IthArg(int idx)
    {
        return indexed_args_[idx];
    }
    inline bool IthValueOfConstant(int idx)
    {
        assert(idx >= 0 && idx < ele_size_);
        return const_val_[idx];
    }

    inline void SetSubtree(TreeNode *tn) { sub_nodes_.push_back(tn); }
    inline void SetSubtree(TreeNode *tn0, TreeNode *tn1)
    {
        sub_nodes_.push_back(tn0);
        sub_nodes_.push_back(tn1);
    }
    inline void SetSubtree(TreeNode *tn0, TreeNode *tn1, TreeNode *tn2)
    {
        sub_nodes_.push_back(tn0);
        sub_nodes_.push_back(tn1);
        sub_nodes_.push_back(tn2);
    }

    inline void SetArgs(int u, int l)
    {
        indexed_args_.push_back(u);
        indexed_args_.push_back(l);
    }

    inline void SetArgs(int w)
    {
        indexed_args_.push_back(w);
    }

    inline bool IsArr() { return idx_size_ > 0; }
    inline bool IsConstant() { return const_val_ != NULL; }
    inline bool IsVariable() { return var_name_.size() > 0; }
    inline bool IsLeafNode() { return this->IsConstant() || this->IsVariable(); }

    void Print(NodeManager *node_manager);
    inline bool BeenPrinted() { return line_id_ != -1; }
    inline void SetLineId(int line_id) { line_id_ = line_id; }
    inline int GetLineId() { return line_id_; }

    inline void SetAsInput()
    {
        assert(this->IsVariable());
        init_value_ = NULL;
        is_input_var_ = true;
    }
    inline bool IsInput()
    {
        return is_input_var_;
    }
    bool IsZero(); // constant 0
    bool IsOne();  // constant 1
    bool IsOnes(); // constant ffffff

    inline void SetInitValue(TreeNode *init_val) { init_value_ = init_val; }
    inline TreeNode *GetInitValue() { return init_value_; }

private:
    oper op_;

    int idx_size_; // for bv, idx_size_ = -1
    int ele_size_;

    int depth_;

    int line_id_;

    string var_name_; // for variables leaf node

    bool *const_val_; // for constant leaf node

    bool is_input_var_;

    TreeNode *init_value_; // only for bv state variables

    vector<TreeNode *> sub_nodes_;
    vector<int> indexed_args_;

    void PrintOpName();
};

bool Identical(TreeNode *node_0, TreeNode *node_1);

class NodeManager
{
public:
    ~NodeManager();

    TreeNode *InsertNode(TreeNode *);
    TreeNode *GetNode(int idx_size, int ele_size, int tree_depth = -1);

    int GetSortLineId(int idx_size, int ele_size);
    inline bool IsPrintedSort(int idx_size, int ele_size)
    {
        return GetSortLineId(idx_size, ele_size) != -1;
    }
    void SetSortLineId(int idx_size, int ele_size, int line_id);
    void PrintSort(int idx_size, int ele_size);

private:
    vector<int> bv_size_;
    vector<vector<TreeNode *>> bv_nodes_;
    vector<int> bv_sort_line_id_;

    vector<std::pair<int, int>> arr_sort_; // idx-ele
    vector<vector<TreeNode *>> arr_nodes_;
    vector<int> arr_sort_line_id_;
};

#endif