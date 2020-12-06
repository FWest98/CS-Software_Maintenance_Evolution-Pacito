#include "helpers.h"

jfieldID getPointerField(JNIEnv *env, jobject obj) {
    // Get JniObject java class
    auto javaJniObject = env->FindClass("Pacito/JniObject");
    if(javaJniObject == nullptr) return nullptr; // Then the object is not a jni object so we can't manage it
    if(!env->IsInstanceOf(obj, javaJniObject)) return nullptr;

    // Get objPtr field id
    auto result = env->GetFieldID(javaJniObject, "objPtr", "J");
    env->DeleteLocalRef(javaJniObject);

    return result;
}

jobject makeObject(JNIEnv* env, const char* name) {
    auto cls = env->FindClass(name);
    if(cls == nullptr) return nullptr;

    auto constructor = env->GetMethodID(cls, "<init>", "()V");
    if(constructor == nullptr) return nullptr;

    return env->NewObject(cls, constructor);
}

void setString(JNIEnv *env, jobject obj, const char *field, const char *value) {
    auto cls = env->GetObjectClass(obj);
    if(cls == nullptr) return;

    auto fieldId = env->GetFieldID(cls, field, "Ljava/lang/String;");
    if(fieldId == nullptr) return;

    auto content = env->NewStringUTF(value);
    env->SetObjectField(obj, fieldId, content);
}