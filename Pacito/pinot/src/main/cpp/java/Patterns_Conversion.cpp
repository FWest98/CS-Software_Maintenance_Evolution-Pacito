#include "../patterns/Patterns.h"
#include "helpers.h"

jobject CoR::ConvertToJava(JNIEnv *env) {
    auto obj = makeObject(env, "Pacito/Patterns/CoR");
    setString(env, obj, "Subject", this->subject->Utf8Name());
    setString(env, obj, "File", this->file->FileName());
    setString(env, obj, "Handler", this->handler->Utf8Name());
    setString(env, obj, "PropagatorName", this->propagator->Utf8Name());
    setString(env, obj, "PropagatorType", this->propagator->Type()->Utf8Name());

    return obj;
}

jobject Decorator::ConvertToJava(JNIEnv *env) {
    auto obj = makeObject(env, "Pacito/Patterns/Decorator");
    setString(env, obj, "Subject", this->subject->Utf8Name());
    setString(env, obj, "File", this->file->FileName());
    setString(env, obj, "DecorateOperation", this->decorateOp->Utf8Name());
    setString(env, obj, "DecorateeName", this->decoratee->Utf8Name());
    setString(env, obj, "DecorateeType", this->decoratee->Type()->Utf8Name());

    return obj;
}