#ifndef PACITO_SINGLETONANALYSIS_H
#define PACITO_SINGLETONANALYSIS_H

#include "set.h"
#include "symbol.h"
#include "Helpers.h"

class SingletonAnalysis
{
private:
    VariableSymbol *variable;
    MethodSymbol *method;
    Flatten flatten;
    thread_local inline static SymbolSet visited; // visited methods, avoid analysis on recursion methods.
    StoragePool *ast_pool;
    vector<vector<signed> > exec_paths;
    vector<signed> path;
    vector<signed> footprints; // point of creating the Singleton instance
    vector<signed> fingerprints; // point of returning the Singleton instance
public:
    SingletonAnalysis(VariableSymbol *vsym, MethodSymbol *msym, StoragePool *pool)
            :variable(vsym),method(msym), ast_pool(pool){ flatten.init(msym, pool); }
    void TracePath(Snapshot*);
    bool ReturnsSingleton();
    bool ReturnsSingleton1();
    void CleanUp() { visited.SetEmpty(); }
    ~SingletonAnalysis()= default;
};


#endif //PACITO_SINGLETONANALYSIS_H
