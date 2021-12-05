#include <jni.h>
#include <jvmti.h>
#include <string.h>
#include <stdio.h>

static void printJvmtiError(jvmtiEnv *jvmti, jvmtiError error, char *message) {
  char *name;
  jvmtiError err = (*jvmti)->GetErrorName(jvmti, error, &name);
  if (err != JVMTI_ERROR_NONE) {
    (*jvmti)->Deallocate(jvmti, (void*)name);
    fprintf(stderr, "ERROR: JVMTI: %s failed with error(%d): %s\n", message, error, name);
  } else {
    fprintf(stderr, "ERROR: JVMTI: %s failed with error(%d)\n", message, error);
  }
}

jboolean isNative(jvmtiEnv *jvmti, jmethodID method) {
  jboolean is_native;
  jvmtiError err = (*jvmti)->IsMethodNative(jvmti, method, &is_native);
  if (err == JVMTI_ERROR_NONE) {
    return is_native;
  } else {
    printJvmtiError(jvmti, err, "IsMethodNative");
    return JNI_FALSE;
  }
}

jint hasZeroArguments(jvmtiEnv *jvmti, jmethodID method) {
  jint argument_count;
  jvmtiError err = (*jvmti)->GetArgumentsSize(jvmti, method, &argument_count);
  if (err == JVMTI_ERROR_NONE) {
    return argument_count == 0 ? JNI_TRUE : JNI_FALSE;
  } else {
    printJvmtiError(jvmti, err, "GetArgumentsSize");
    return JNI_FALSE;
  }
}

jint isNamedFinalize(jvmtiEnv *jvmti, jmethodID method) {
  char* name;
  jvmtiError err = (*jvmti)->GetMethodName(jvmti, method, &name, NULL, NULL);
  if (err == JVMTI_ERROR_NONE) {
    jint result = strcmp(name, "finalize") == 0 ? JNI_TRUE : JNI_FALSE;
    (*jvmti)->Deallocate(jvmti, (void*)name);
    return result;
  } else {
    printJvmtiError(jvmti, err, "GetMethodName");
    return JNI_FALSE;
  }
}

jint isFinalizer(jvmtiEnv *jvmti, jmethodID method) {
  if (isNative(jvmti, method) == JNI_FALSE && hasZeroArguments(jvmti, method) && isNamedFinalize(jvmti, method)) {
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
    (*jvmti)->Deallocate(jvmti, (void*)name);
  } else {
    printJvmtiError(jvmti, err, "GetClassSignature");
  }
}

void printClassNameNotPrepared(jvmtiEnv *jvmti, jclass klass) {
  char* name;
  jvmtiError err = (*jvmti)->GetClassSignature(jvmti, klass, &name, NULL);
  if (err == JVMTI_ERROR_NONE) {
    fprintf(stdout, "not prepared %s\n", name);
    (*jvmti)->Deallocate(jvmti, (void*)name);
  } else {
    printJvmtiError(jvmti, err, "GetClassSignature");
  }
}

void printClassNamePrepared(jvmtiEnv *jvmti, jclass klass) {
  char* name;
  jvmtiError err = (*jvmti)->GetClassSignature(jvmti, klass, &name, NULL);
  if (err == JVMTI_ERROR_NONE) {
    fprintf(stdout, "prepared %s\n", name);
    (*jvmti)->Deallocate(jvmti, (void*)name);
  } else {
    printJvmtiError(jvmti, err, "GetClassSignature");
  }
}

void scanKlass(jvmtiEnv *jvmti, jclass klass) {
  jint method_count;
  jmethodID* methods;
  // (*jvmti)->GetClassSignature(jvmti, klass, NULL, NULL);
  jvmtiError err = (*jvmti)->GetClassMethods(jvmti, klass, &method_count, &methods);
  if (err == JVMTI_ERROR_NONE) {
    for (int i = 0; i < method_count; i++) {
      jmethodID method = methods[i];
      if (isFinalizer(jvmti, method)) {
        printClassName(jvmti, klass);
      }
    }
    (*jvmti)->Deallocate(jvmti, (void*)methods);
  } else if (err == JVMTI_ERROR_CLASS_NOT_PREPARED) {
    printClassNameNotPrepared(jvmti, klass);
    // ignore for now
  } else {
    printJvmtiError(jvmti, err, "GetClassMethods");
  }
}

jint findFinalizers(jvmtiEnv *jvmti, JNIEnv* env) {
  jint class_count;
  jclass* classes;

  jvmtiError err = (*jvmti)->GetLoadedClasses(jvmti, &class_count, &classes);
  if (err == JVMTI_ERROR_NONE) {
    for (int i = 0; i < class_count; i++) {
      jclass klass = classes[i];
      scanKlass(jvmti, klass);
      (*env)->DeleteLocalRef(env, klass);
    }
    (*jvmti)->Deallocate(jvmti, (void*)classes);
    return JNI_OK;
  } else {
    printJvmtiError(jvmti, err, "GetLoadedClasses");
    return err;
  }
}

void JNICALL callbackClassPrepare(jvmtiEnv *jvmti, JNIEnv* env, jthread thread, jclass klass) {
  printClassNamePrepared(jvmti, klass);
}


JNIEXPORT jint JNICALL Agent_OnAttach(JavaVM* jvm, char *options, void *reserved) {

  jvmtiEnv *jvmti;
  JNIEnv* env;
  jvmtiEventCallbacks callbacks;
  (void)memset(&callbacks, 0, sizeof(callbacks));

  jint niErr = (*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_10);
  if (niErr != JNI_OK) {
    fprintf(stderr, "GetEnv (JNI) failed with error(%d)\n", niErr);
    return -1;
  }

  jint tiErr = (*jvm)->GetEnv(jvm, (void**) &jvmti, JVMTI_VERSION_11);
  if (tiErr == JNI_OK) {
    callbacks.ClassPrepare = &callbackClassPrepare;
  
    (*jvmti)->SetEventCallbacks(jvmti, &callbacks, (jint) sizeof(callbacks));
    (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_CLASS_PREPARE, (jthread) NULL);
    //(*jvmti)->GenerateEvents(jvmti, (void*)classes);
    jint result = findFinalizers(jvmti, env);
    //(*jvmti)->DisposeEnvironment(jvmti);
    // does JNIEnv need to be disposed?
    return result;
  } else {
    fprintf(stderr, "GetEnv (JVMTI) failed with error(%d)\n", tiErr);
    return JNI_ERR;
  }
}