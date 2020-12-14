#ifndef PACITO_FLYWEIGHTANALYSIS_H
#define PACITO_FLYWEIGHTANALYSIS_H
#include "../ast.h"
#include "Helpers.h"

class FlyweightAnalysis
{
private:
    MethodSymbol *GetFlyweight;
    TypeSymbol *flyweight;
    SymbolSet key;
    ContainerType *container_type;
    vector<Ast*> statements;
    vector<AstExpression*> conditions;
    vector<Snapshot*> summary;
    vector<Snapshot*> traces;
    //SymbolSet vcache;
    char bitmap[10];
    int n;
    // include some kind of summary for the resulting flow analysis
    void AssignRoles();
public:
    FlyweightAnalysis(MethodSymbol *GetMethod)
    {
        container_type = NULL;
        GetFlyweight = GetMethod;
        flyweight = GetFlyweight->Type();
        for (unsigned i = 0; i < GetFlyweight->NumFormalParameters(); i++)
            key.AddElement(GetFlyweight->FormalParameter(i));
        for (int i = 0; i < 10; i++) bitmap[i] = 'X';
        n = 0;
    }
    ~FlyweightAnalysis(){delete container_type;}
    bool IsFlyweightFactory();
    void UpdateSummary();
    void DumpSummary();
    VariableSymbol *GetFlyweightPool() { return (container_type) ? container_type->GetContainer(): NULL;}
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
    void visit(AstSynchronizedStatement*){}
    void visit(AstConditionalExpression*){}
};


#endif //PACITO_FLYWEIGHTANALYSIS_H
