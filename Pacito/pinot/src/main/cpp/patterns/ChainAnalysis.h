#ifndef PACITO_CHAINANALYSIS_H
#define PACITO_CHAINANALYSIS_H

#include "set.h"
#include "symbol.h"
#include "Helpers.h"

class ChainAnalysis
{
private:
    VariableSymbol *variable;
    MethodSymbol *method;
    Flatten flatten;
    //static SymbolSet visited; // visited methods, avoid analysis on recursion methods.
    StoragePool *ast_pool;
    vector<vector<signed> > paths;
    vector<signed> path;
    vector<signed> footprints;
public:
    enum ResultTag
    {
        CoR,
        DECORATOR,
        NONE
    };
    ChainAnalysis(VariableSymbol *vsym, MethodSymbol *msym, StoragePool *pool)
            :variable(vsym),method(msym), ast_pool(pool){ flatten.init(msym, pool); }
    void TracePath(Snapshot*);
    void TraceBinaryExpression(AstBinaryExpression*, Snapshot*);
    ResultTag AnalyzeCallChain();
    void CleanUp() { paths.clear(); /*visited.SetEmpty();*/ }
    ~ChainAnalysis()= default;
};


#endif //PACITO_CHAINANALYSIS_H
