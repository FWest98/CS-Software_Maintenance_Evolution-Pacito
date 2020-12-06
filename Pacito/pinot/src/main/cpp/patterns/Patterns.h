#ifndef PACITO_PATTERNS_H
#define PACITO_PATTERNS_H

#include <memory>
#include <ast.h>
#include <jni.h>

class Pattern {
public:
    typedef std::shared_ptr<Pattern> Ptr;

    Pattern() = default;

    static vector<Ptr> FindChainOfResponsibility(Control *);
    static vector<Ptr> FindBridge(Control *);
    static vector<Ptr> FindStrategy(Control *);
    static vector<Ptr> FindFlyweight(Control *);

    virtual void Print() = 0;
    virtual jobject ConvertToJava(JNIEnv *) = 0;
    virtual ~Pattern() = default;
};

class Bridge : public Pattern {
public:
    TypeSymbol *delegator;
    FileSymbol *delegatorFile;
    TypeSymbol *delegated;
    FileSymbol *delegatedFile;

    jobject ConvertToJava(JNIEnv *) override;
    void Print() override {
        Coutput << "Bridge Pattern" << endl;
        Coutput << delegator->Utf8Name() << " is abstract" << endl;
        Coutput << delegated->Utf8Name() << " is an interface" << endl;
        Coutput << delegator->Utf8Name() << " delegates " << delegated->Utf8Name() << endl;
        Coutput << "File location: " << delegatorFile->FileName() << endl << delegatedFile->FileName() << endl << endl;
    }
};

class CoR : public Pattern {
public:
    TypeSymbol *handler;
    MethodSymbol *handlerMethod;
    VariableSymbol *propagator;
    FileSymbol *file;

    jobject ConvertToJava(JNIEnv *) override;
    void Print() override {
        Coutput << "Chain of Responsibility Pattern" << endl;
        Coutput << handler->Utf8Name() << " is a Chain of Responsibility Handler class" << endl;
        Coutput << handlerMethod->Utf8Name() << " is a handle operation" << endl;
        Coutput << propagator->Utf8Name() << " of type " << propagator->Type()->Utf8Name() << " propagates the request"
                << endl;
        Coutput << "File location: " << file->FileName() << endl << endl;
    }
};

class Decorator : public Pattern {
public:
    TypeSymbol *decorator;
    MethodSymbol *decorateMethod;
    VariableSymbol *decoratee;
    FileSymbol *file;

    jobject ConvertToJava(JNIEnv *) override;
    void Print() override {
        Coutput << "Decorator Pattern" << endl;
        Coutput << decorator->Utf8Name() << " is a Decorator class" << endl;
        Coutput << decorateMethod->Utf8Name() << " is a decorate operation" << endl;
        Coutput << decoratee->Utf8Name() << " of type " << decoratee->Type()->Utf8Name() << " is the Decoratee class"
                << endl;
        Coutput << "File location: " << file->FileName() << endl << endl;
    }
};

class Flyweight : public Pattern {
public:
    TypeSymbol *objectType = nullptr;
    TypeSymbol *factory = nullptr;
    MethodSymbol *factoryMethod = nullptr;

    VariableSymbol *pool = nullptr;
    VariableSymbol *object = nullptr;

    FileSymbol *file = nullptr;
    bool isImmutable = false;

    jobject ConvertToJava(JNIEnv *) override;
    void Print() override {
        Coutput << "Flyweight Pattern" << endl;

        if(isImmutable) {
            Coutput << factory->Utf8Name() << " is immutable" << endl;
        } else {
            Coutput << factory->Utf8Name() << " is a flyweight factory" << endl;

            if(pool) {
                Coutput << pool->Utf8Name() << " is the flyweight pool" << endl;
                Coutput << factoryMethod->Utf8Name() << " is the factory methods, producing flyweight objects of type " << objectType->Utf8Name() << endl;
            } else if (object && factoryMethod) {
                Coutput << object->Utf8Name() << " is a flyweight object" << endl;
                Coutput << factoryMethod->Utf8Name() << " is the getFlyweight method" << endl;
            } else if(object) {
                Coutput << object->Utf8Name() << " is a flyweight object (declared public/static/final)" << endl;
            }
        }

        Coutput << "File location: " << file->FileName() << endl << endl;
    }
};

class StatePattern : public Pattern {
public:
    TypeSymbol *context;
    TypeSymbol *state;
    vector<TypeSymbol *> stateImplementations;

    VariableSymbol *delegator;
    MethodSymbol *stateChanger;
    vector<MethodSymbol *> changeInvokers;

    FileSymbol *contextFile;
    FileSymbol *stateFile;

    jobject ConvertToJava(JNIEnv *) override;
    void Print() override {
        Coutput << "State Pattern" << endl;
        Coutput << context->Utf8Name() << " is the Context class" << endl;
        Coutput << state->Utf8Name() << " is the State interface with the following concrete implementations:" << endl;
        for(auto impl : stateImplementations)
            Coutput << impl->Utf8Name() << ", ";
        Coutput << endl;

        Coutput << "Delegation is through " << delegator->Utf8Name() << ", being changed by " << stateChanger->Utf8Name() << endl;
        Coutput << "Changer is invoked by:" << endl;
        for(auto inv : changeInvokers)
            Coutput << inv->Utf8Name() << ", ";
        Coutput << endl;

        Coutput << "File location: " << contextFile->FileName() << endl << stateFile->FileName() << endl << endl;
    }
};

class Strategy : public Pattern {
public:
    TypeSymbol *context;
    TypeSymbol *strategy;
    vector<TypeSymbol *> strategyImplementations;
    VariableSymbol *delegator;

    FileSymbol *contextFile;
    FileSymbol *strategyFile;

    jobject ConvertToJava(JNIEnv *) override;
    void Print() override {
        Coutput << "Strategy Pattern" << endl;
        Coutput << context->Utf8Name() << " is the Context class" << endl;
        Coutput << strategy->Utf8Name() << " is the Strategy interface with the following concrete implementations:" << endl;
        for(auto impl : strategyImplementations)
            Coutput << impl->Utf8Name() << ", ";
        Coutput << endl;

        Coutput << "Delegation is through " << delegator->Utf8Name() << endl;
        Coutput << "File location: " << contextFile->FileName() << endl << strategyFile->FileName() << endl << endl;
    }
};

#endif //PACITO_PATTERNS_H
