#include <jni.h>
#include <jvmti.h>
#include <classfile_constants.h>
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
  jboolean isNative;
  jvmtiError err = (*jvmti)->IsMethodNative(jvmti, method, &isNative);
  if (err == JVMTI_ERROR_NONE) {
    return isNative;
  } else {
    printJvmtiError(jvmti, err, "IsMethodNative");
    return JNI_FALSE;
  }
}

jboolean isStatic(jvmtiEnv *jvmti, jmethodID method) {
  jint modifiers;
  jvmtiError err = (*jvmti)->GetMethodModifiers(jvmti, method, &modifiers);
  if (err == JVMTI_ERROR_NONE) {
    return (modifiers & JVM_ACC_STATIC) ? JNI_TRUE : JNI_FALSE;
  } else {
    printJvmtiError(jvmti, err, "GetMethodModifiers");
    return JNI_FALSE;
  }
}

jint getArgumentCount(jvmtiEnv *jvmti, jmethodID method) {
  jint argumentCount;
  jvmtiError err = (*jvmti)->GetArgumentsSize(jvmti, method, &argumentCount);
  if (err == JVMTI_ERROR_NONE) {
    return argumentCount;
  } else {
    printJvmtiError(jvmti, err, "GetArgumentsSize");
    return -1;
  }
}

jboolean isNamedFinalize(jvmtiEnv *jvmti, jmethodID method) {
  char* methodName;
  jvmtiError err = (*jvmti)->GetMethodName(jvmti, method, &methodName, NULL, NULL);
  if (err == JVMTI_ERROR_NONE) {
    jboolean result = strcmp(methodName, "finalize") == 0 ? JNI_TRUE : JNI_FALSE;
    (*jvmti)->Deallocate(jvmti, (void*)methodName);
    return result;
  } else {
    printJvmtiError(jvmti, err, "GetMethodName");
    return JNI_FALSE;
  }
}

jint isFinalizer(jvmtiEnv *jvmti, jmethodID method) {
  if (isNative(jvmti, method) == JNI_FALSE
      && isStatic(jvmti, method) == JNI_FALSE
      // "this" reference is first argument
      && getArgumentCount(jvmti, method) == 1
      && isNamedFinalize(jvmti, method)) {
    return JNI_TRUE;
  } else {
    return JNI_FALSE;
  }
}

jboolean isInitialized(jvmtiEnv *jvmti, jclass klass) {
  jint status;
  jvmtiError err = (*jvmti)->GetClassStatus(jvmti, klass, &status);
  if (err == JVMTI_ERROR_NONE) {
    return (status & JVMTI_CLASS_STATUS_INITIALIZED) != 0 ? JNI_TRUE : JNI_FALSE;
  } else {
    printJvmtiError(jvmti, err, "GetClassStatus");
    return JNI_FALSE;
  }
}

void replaceChar(char *str, char original, char replacement) {
  char *i = str;
  while ((i = strchr(i, original)) != NULL) {
    *i++ = replacement;
  }
}

char* toJavaName(char *jniName) {
  // Lcom/ibm/icu/impl/coll/SharedObject$Reference; -> com.ibm.icu.impl.coll.SharedObject$Reference
  replaceChar(jniName, '/', '.');
  size_t len = strlen(jniName);
  jniName[len - 1] = '\0';
  return &jniName[1];
}

void printClassName(jvmtiEnv *jvmti, jclass klass) {
  char* className;
  char* javaName;
  jvmtiError err = (*jvmti)->GetClassSignature(jvmti, klass, &className, NULL);
  if (err == JVMTI_ERROR_NONE) {
    javaName = toJavaName(className);
    fprintf(stdout, "%s\n", javaName);
    (*jvmti)->Deallocate(jvmti, (void*)className);
  } else {
    printJvmtiError(jvmti, err, "GetClassSignature");
  }
}

void scanKlass(jvmtiEnv *jvmti, jclass klass) {
  jint methodCount;
  jmethodID* methods;
  jvmtiError err = (*jvmti)->GetClassMethods(jvmti, klass, &methodCount, &methods);
  if (err == JVMTI_ERROR_NONE) {
    for (int i = 0; i < methodCount; i++) {
      jmethodID method = methods[i];
      if (isFinalizer(jvmti, method)) {
        printClassName(jvmti, klass);
      }
    }
    (*jvmti)->Deallocate(jvmti, (void*)methods);
  } else if (err == JVMTI_ERROR_CLASS_NOT_PREPARED) {
    // ignore, not prepared means not initialized means no instantiated
  } else {
    printJvmtiError(jvmti, err, "GetClassMethods");
  }
}

jint findFinalizers(jvmtiEnv *jvmti, JNIEnv* env) {
  jint classCount;
  jclass* classes;

  jvmtiError err = (*jvmti)->GetLoadedClasses(jvmti, &classCount, &classes);
  if (err == JVMTI_ERROR_NONE) {
    for (int i = 0; i < classCount; i++) {
      jclass klass = classes[i];
      if (isInitialized(jvmti, klass) == JNI_TRUE) {
        scanKlass(jvmti, klass);
      }
      (*env)->DeleteLocalRef(env, klass);
    }
    (*jvmti)->Deallocate(jvmti, (void*)classes);
    return JNI_OK;
  } else {
    printJvmtiError(jvmti, err, "GetLoadedClasses");
    return JNI_ERR;
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

