package com.github.marschall.finalizeanalizer;

import java.lang.instrument.Instrumentation;

public class Agent {

  public static void agentmain(String args, Instrumentation instrumentation) {
    for (Class<?> clazz : instrumentation.getAllLoadedClasses()) {
      if (clazz == Object.class) {
        continue;
      }
      if (clazz.isArray() || clazz.isAnnotation() || clazz.isEnum() || clazz.isPrimitive() || clazz.isRecord()) {
        continue;
      }
      Thread currentThread = Thread.currentThread();
      ClassLoader previous = currentThread.getContextClassLoader();
      currentThread.setContextClassLoader(clazz.getClassLoader());
      try {
        if (hasFinalizer(clazz)) {
          System.out.println(clazz.getName());
        }
      } finally {
        currentThread.setContextClassLoader(previous);
      }
    }
  }

  private static boolean hasFinalizer(Class<?> clazz) {
    try {
      // no need to check superclass as it is loaded as well
      clazz.getDeclaredMethod("finalize");
      return true;
    } catch (NoClassDefFoundError e) {
      // can unfortunately happen if some of the type of methods are not available to this class
      return false;
    } catch (NoSuchMethodException e) {
      return false;
    }
  }

}
