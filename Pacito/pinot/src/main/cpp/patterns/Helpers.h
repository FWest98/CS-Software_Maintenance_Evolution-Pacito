#ifndef PACITO_HELPERS_H
#define PACITO_HELPERS_H
#include "../ast.h"

class Role
{
    friend class FlyweightAnalysis;
    friend class Snapshot;
    friend class Flatten;
private:
    enum RoleTag
    {
        CREATE,
        REGISTER,
        RETRIEVE,
        ALLOCATE,
        RETURN,
        NIL
    };
public:
    Role(AstArrayAccess *v, RoleTag t):array_access(v),tag(t){vsym = NULL;}
    Role(VariableSymbol *v, RoleTag t):vsym(v),tag(t){array_access = NULL;}
private:
    VariableSymbol *vsym;
    AstArrayAccess *array_access;
    RoleTag tag;
    const char *TagName();
};
class Snapshot
{
    friend class SingletonAnalysis;
    friend class FlyweightAnalysis;
    friend class FactoryAnalysis;
    friend class ChainAnalysis;
    friend class Flatten;
public:
    Snapshot():statements(NULL),conditions(NULL),condition(NULL),roles(NULL),index(0), number(-1){}
private:
    vector<Ast*> *statements;
    vector<AstExpression*> *conditions;
    AstExpression *condition;
    vector<Role*> *roles;
    signed index; // for IsFlyweightFactory
    signed number;
    set<signed> previous, next;
};
class Flatten
{
    friend class SingletonAnalysis;
    friend class FactoryAnalysis;
    friend class ChainAnalysis;
private:
    vector<Ast*> statements;
    vector<AstExpression*> conditions;
    AstExpression *condition;
    vector<Snapshot*> summary;
    vector<Snapshot*> traces;
    MethodSymbol *method;
    bool capture_trace;
    StoragePool *ast_pool;
    set<signed> pred;
    bool multi_if;
public:
    enum TransitionTag
    {
        UNCONDITIONAL,
        CONDITIONAL,
        NOTRANSITION
    };
    Flatten():condition(NULL), capture_trace(false), multi_if(false){ pred.insert(-1); }
    Flatten(MethodSymbol *msym, StoragePool *pool):condition(NULL),method(msym),capture_trace(false),ast_pool(pool), multi_if(false){ pred.insert(-1); }
    ~Flatten(){}
    void init(MethodSymbol *msym, StoragePool *pool){ method = msym; ast_pool = pool; }
    void BuildSummary();
    void UpdateSummary();
    void DumpSummary();
    void PushCondition(AstExpression*);
    void PopCondition();
    bool Compare(AstExpression *, AstExpression *);
    void FlattenBoolean(vector<AstExpression*>&, AstExpression*);
    TransitionTag TransitionFlow(AstExpression *, AstExpression *);
    void visit(AstBlock*);
    void visit(AstIfStatement*);
    void visit(AstWhileStatement*);
    void visit(AstForStatement*);
    void visit(AstTryStatement*);
    void visit(AstStatement*);
    void visit(AstExpression*);
    void visit(AstAssignmentExpression*);
    void visit(AstLocalVariableStatement*);
    void visit(AstVariableDeclarator*);
    void visit(AstReturnStatement*);
    void visit(AstMethodInvocation*);
    void visit(AstSynchronizedStatement*);
    void visit(AstConditionalExpression*);
};

#endif //PACITO_HELPERS_H
