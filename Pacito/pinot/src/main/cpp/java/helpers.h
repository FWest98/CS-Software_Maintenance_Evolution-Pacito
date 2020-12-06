#include <jni.h>
#include <vector>

#ifndef PACITO_HELPERS_H
#define PACITO_HELPERS_H

jfieldID getPointerField(JNIEnv *, jobject);
jobject makeObject(JNIEnv*, const char*);
void setBool(JNIEnv *, jobject, const char *, bool);
void setString(JNIEnv *, jobject, const char *, const char *);
void setStringArray(JNIEnv *, jobject, const char *, std::vector<const char *>);

template <class T> void setInstance(JNIEnv *env, jobject obj, T *instance) {
    env->SetLongField(obj, getPointerField(env, obj), (jlong) instance);
}

template <class T> T* getInstance(JNIEnv *env, jobject obj) {
    return (T*) env->GetLongField(obj, getPointerField(env, obj));
}

#endif //PACITO_HELPERS_H
