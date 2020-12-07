#include "helpers.h"
#include <vector>

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

void setBool(JNIEnv *env, jobject obj, const char *field, bool value) {
    auto cls = env->GetObjectClass(obj);
    if(cls == nullptr) return;

    auto fieldId = env->GetFieldID(cls, field, "Z");
    if(fieldId == nullptr) return;

    env->SetBooleanField(obj, fieldId, value);
}

void setInt(JNIEnv *env, jobject obj, const char *field, int value) {
    auto cls = env->GetObjectClass(obj);
    if(cls == nullptr) return;

    auto fieldId = env->GetFieldID(cls, field, "I");
    if(fieldId == nullptr) return;

    env->SetIntField(obj, fieldId, value);
}

void setString(JNIEnv *env, jobject obj, const char *field, const char *value) {
    auto cls = env->GetObjectClass(obj);
    if(cls == nullptr) return;

    auto fieldId = env->GetFieldID(cls, field, "Ljava/lang/String;");
    if(fieldId == nullptr) return;

    auto content = env->NewStringUTF(value);
    env->SetObjectField(obj, fieldId, content);
}

void setStringArray(JNIEnv *env, jobject obj, const char *field, std::vector<const char *> arr) {
    auto cls = env->GetObjectClass(obj);
    if(cls == nullptr) return;

    auto fieldId = env->GetFieldID(cls, field, "[Ljava/lang/String;");
    if(fieldId == nullptr) return;

    auto stringCls = env->FindClass("java/lang/String");
    auto array = env->NewObjectArray(arr.size(), stringCls, nullptr);

    for(auto i = 0; i < arr.size(); i++) {
        auto content = env->NewStringUTF(arr.at(i));
        env->SetObjectArrayElement(array, i, content);
    }

    env->SetObjectField(obj, fieldId, array);
}