#include <jni.h>
#include <jvmti.h>
#include <string.h>
#include <stdio.h>

jint hasZeroArguments(jvmtiEnv *jvmti, jmethodID method) {
  jint argument_count;
  jvmtiError err = (*jvmti)->GetArgumentsSize(jvmti, method, &argument_count);
  if (err == JVMTI_ERROR_NONE) {
    return argument_count == 0 ? JNI_TRUE : JNI_FALSE;
  } else {
    fprintf(stderr, "GetArgumentsSize (JVMTI) failed with error(%d)\n", err);
    return JNI_FALSE;
  }
}

jint isNamedFinalize(jvmtiEnv *jvmti, jmethodID method) {
  char* name;
  jvmtiError err = (*jvmti)->GetMethodName(jvmti, method, &name, NULL, NULL);
  if (err == JVMTI_ERROR_NONE) {
    jint result = strcmp(name, "finalize") == 0 ? JNI_TRUE : JNI_FALSE;
    (*jvmti)->Deallocate(jvmti, (void*)&name);
    return result;
  } else {
    fprintf(stderr, "GetMethodName (JVMTI) failed with error(%d)\n", err);
    return JNI_FALSE;
  }
}

jint isFinalizer(jvmtiEnv *jvmti, jmethodID method) {
  if (hasZeroArguments(jvmti, method) && isNamedFinalize(jvmti, method)) {
    return JNI_TRUE;
  } else {
    return JNI_FALSE;
  }
}

void printClassName(jvmtiEnv *jvmti, jclass klass) {
  char* name;
  jvmtiError err = (*jvmti)->GetClassSignature(jvmti, klass, &name, NULL);
  if (err == JVMTI_ERROR_NONE) {
    fprintf(stdout, "%s\n", name);
    (*jvmti)->Deallocate(jvmti, (void*)&name);
  } else {
    fprintf(stderr, "GetClassSignature (JVMTI) failed with error(%d)\n", err);
  }
}

void scanKlass(jvmtiEnv *jvmti, jclass klass) {
  jint method_count;
  jmethodID* methods;
  jvmtiError err = (*jvmti)->GetClassMethods(jvmti, klass, &method_count, &methods);
  if (err == JVMTI_ERROR_NONE) {
    for (int i = 0; i < method_count; i++) {
      jmethodID method = methods[i];
      if (isFinalizer(jvmti, method)) {
        printClassName(jvmti, klass);
      }
    }
    (*jvmti)->Deallocate(jvmti, (void*)&methods);
  } else {
    fprintf(stderr, "GetClassMethods (JVMTI) failed with error(%d)\n", err);
  }
}

jint findFinalizers(jvmtiEnv *jvmti, JNIEnv* env) {
  jint class_count;
  jclass* classes;

  jvmtiError err = (*jvmti)->GetLoadedClasses(jvmti, &class_count, &classes);
  if (err == JVMTI_ERROR_NONE) {
    for (int i = 0; i < class_count; i++) {
      // TODO release class
      jclass klass = classes[i];
      scanKlass(jvmti, klass);
      (*env)->DeleteLocalRef(env, klass);
    }
    (*jvmti)->Deallocate(jvmti, (void*)&classes);
    return JNI_OK;
  } else {
    fprintf(stderr, "GetLoadedClasses (JVMTI) failed with error(%d)\n", err);
    return err;
  }
}


JNIEXPORT jint JNICALL Agent_OnAttach(JavaVM* jvm, char *options, void *reserved) {

  jvmtiEnv *jvmti;
  JNIEnv* env;

  jint niErr = (*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_10);
  if (niErr != JNI_OK) {
    fprintf(stderr, "GetEnv (JNI) failed with error(%d)\n", niErr);
    return -1;
  }

  jint tiErr = (*jvm)->GetEnv(jvm, (void**) &jvmti, JVMTI_VERSION_11);
  if (tiErr == JNI_OK) {
    jint result = findFinalizers(jvmti, env);
    (*jvmti)->DisposeEnvironment(jvmti);
    // does JNIEnv need to be disposed?
    return result;
  } else {
    fprintf(stderr, "GetEnv (JVMTI) failed with error(%d)\n", tiErr);
    return JNI_ERR;
  }
}