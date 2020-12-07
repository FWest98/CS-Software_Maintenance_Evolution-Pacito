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
    static vector<Ptr> FindTemplateMethod(Control *);
    static vector<Ptr> FindFactory(Control *);
    static vector<Ptr> FindVisitor(Control *);
    static vector<Ptr> FindObserver(Control *);
    static vector<Ptr> FindMediator(Control *);
    static vector<Ptr> FindProxy(Control *);
    static vector<Ptr> FindAdapter(Control *);
    static vector<Ptr> FindFacade(Control *);

    virtual void Print() = 0;
    virtual jobject ConvertToJava(JNIEnv *) = 0;
    virtual ~Pattern() = default;
};

class Adapter : public Pattern {
public:
    vector<TypeSymbol *> adapting;
    TypeSymbol *adapter;
    TypeSymbol *adaptee;
    FileSymbol *adapterFile;
    FileSymbol *adapteeFile;

    jobject ConvertToJava(JNIEnv *) override;
    void Print() override {
        Coutput << "Adapter Pattern" << endl;
        Coutput << "Adapting classes: ";
        for(auto adapt : adapting) Coutput << adapt->Utf8Name() << ", ";
        Coutput << endl;
        Coutput << adapter->Utf8Name() << " is an adapter class" << endl;
        Coutput << adaptee->Utf8Name() << " is an adaptee class" << endl;
        Coutput << "File locations: " << adapterFile->FileName() << endl << adapteeFile->FileName() << endl << endl;
    }
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

class Facade : public Pattern {
public:
    TypeSymbol *facade;
    vector<TypeSymbol *> hidden;
    vector<TypeSymbol *> access;
    FileSymbol *file;

    jobject ConvertToJava(JNIEnv *) override;
    void Print() override {
        Coutput << "Facade Pattern" << endl;
        Coutput << facade->Utf8Name() << " is a facade class" << endl;
        Coutput << "Hidden types: ";
        for(auto type : hidden) Coutput << type->Utf8Name() << ", ";
        Coutput << endl << "Facade access types: ";
        for(auto type : access) Coutput << type->Utf8Name() << ", ";
        Coutput << endl << "File location: " << file->FileName() << endl << endl;
    }
};

class Factory : public Pattern {
public:
    TypeSymbol *factoryMethodClass;
    TypeSymbol *factoryMethodImpl;
    MethodSymbol *factoryMethod;
    vector<TypeSymbol *> factoryMethodResults;
    TypeSymbol *factoryMethodResultBase;
    FileSymbol *file;

    jobject ConvertToJava(JNIEnv *) override;
    void Print() override {
        Coutput << "Factory Method Pattern" << endl;
        Coutput << factoryMethodClass->Utf8Name() << " is a Factory Method class." << endl;
        Coutput << factoryMethodImpl->Utf8Name() << " is a concrete Factory Method class." << endl;
        Coutput << factoryMethod->Utf8Name() << " is a factory method returns ";
        for(auto res : factoryMethodResults)
            Coutput << res->Utf8Name() << ", ";
        Coutput << " which extends " << factoryMethodResultBase->Utf8Name() << endl;
        Coutput << "File location: " << file->FileName() << endl;
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

class Mediator : public Pattern {
public:
    TypeSymbol *mediator;
    vector<TypeSymbol *> colleagues;
    FileSymbol *file;

    jobject ConvertToJava(JNIEnv *) override;
    void Print() override {
        Coutput << "Mediator Pattern" << endl;
        Coutput << "Mediator: " << mediator->Utf8Name() << endl;
        Coutput << "Colleagues: ";
        for(auto colleague : colleagues) Coutput << colleague->Utf8Name() << ", ";
        Coutput << endl << "File location: " << file->FileName() << endl << endl;
    }
};

class Observer : public Pattern {
public:
    TypeSymbol *iterator;
    TypeSymbol *listenerType;
    MethodSymbol *notify;
    MethodSymbol *update;
    vector<TypeSymbol *> subjects;
    FileSymbol *file;

    jobject ConvertToJava(JNIEnv *) override;
    void Print() override {
        Coutput << "Observer Pattern" << endl;
        Coutput << iterator->Utf8Name() << " is an observer iterator" << endl;
        Coutput << listenerType->Utf8Name() << " is the generic type for the listeners." << endl;
        Coutput << notify->Utf8Name() << " is the notify method, and " << update->Utf8Name() << " is the update method." << endl;
        Coutput << "Subject classes: ";
        for(auto subject : subjects)
            Coutput << subject->Utf8Name() << ", ";
        Coutput << endl;
        Coutput << "File location: " << file->FileName() << endl << endl;
    }
};

class Proxy : public Pattern {
public:
    TypeSymbol *proxy;
    TypeSymbol *interface;
    vector<TypeSymbol *> reals;
    FileSymbol *file;

    jobject ConvertToJava(JNIEnv *) override;
    void Print() override {
        Coutput << "Proxy Pattern" << endl;
        Coutput << proxy->Utf8Name() << " is a proxy" << endl;
        Coutput << interface->Utf8Name() << " is a proxy interface" << endl;
        Coutput << "The real object(s): ";
        for(auto real : reals) Coutput << real->Utf8Name() << ", ";
        Coutput << endl << "File location: " << file->FileName() << endl << endl;
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

class Template : public Pattern {
public:
    TypeSymbol *templateClass;
    MethodSymbol *templateMethod;
    MethodSymbol *templateSource;
    FileSymbol *file;

    jobject ConvertToJava(JNIEnv *) override;
    void Print() override {
        Coutput << "Template Method" << endl;
        Coutput << templateClass->Utf8Name() << " is the template class" << endl;
        Coutput << templateMethod->Utf8Name() << " is the template method" << endl;
        Coutput << templateSource->Utf8Name() << " is a primitive method" << endl;
        Coutput << "File location: " << file->FileName() << endl << endl;
    }
};

class Visitor : public Pattern {
public:
    TypeSymbol *visitor;
    TypeSymbol *visitee;

    MethodSymbol *accept;
    MethodSymbol *visit;
    bool isThisExposed = false; // or
    VariableSymbol *exposed = nullptr;

    FileSymbol *file;

    // Optional
    TypeSymbol *abstractVisitee = nullptr;
    vector<TypeSymbol *> visiteeImplementations;

    jobject ConvertToJava(JNIEnv *) override;
    void Print() override {
        Coutput << "Visitor Pattern" << endl;
        Coutput << visitor->Utf8Name() << " is an abstract Visitor class." << endl;
        Coutput << visitee->Utf8Name() << " is a Visitee class" << endl;
        if(abstractVisitee) {
            Coutput << abstractVisitee->Utf8Name() << " is the abstract Visitee superclass with subtypes:" << endl;
            for(auto impl : visiteeImplementations)
                Coutput << impl->Utf8Name() << ", ";
            Coutput << endl;
        }
        Coutput << accept->Utf8Name() << " is the accept method." << endl;
        Coutput << visit->Utf8Name() << " is the visit method." << endl;
        if(isThisExposed)
            Coutput << "Pointer to <this> is exposed to the abstract visitor class" << endl;
        else
            Coutput << "Variable " << exposed->Utf8Name() << " is exposed to the abstract visitor class" << endl;

        Coutput << "File location: " << file->FileName() << endl << endl;
    }
};

#endif //PACITO_PATTERNS_H
