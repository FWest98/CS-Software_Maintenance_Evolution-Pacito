#ifndef PACITO_CONTROLANALYSIS_H
#define PACITO_CONTROLANALYSIS_H
#include "../ast.h"

class ControlAnalysis
{
private:
    AstExpression *expression;
public:
    ControlAnalysis(AstExpression *e)
            : expression (e), flag(false), cond(0), containing_stmt(0), result (false) {}
    void visit(AstBlock*);
    void visit(AstIfStatement*);
    void visit(AstWhileStatement*);
    void visit(AstForStatement*);
    void visit(AstStatement*);
    void visit(AstExpression*);
    void visit(AstSynchronizedStatement*);
    void visit(AstConditionalExpression*);
    bool IsConditional();
    bool IsRepeated();
    bool IsSynchronized();
    ~ControlAnalysis(){}
    bool flag;
    AstExpression* cond;
    Ast* containing_stmt;
    bool result;
    vector<Ast*> rt_stack;
};


#endif //PACITO_CONTROLANALYSIS_H
