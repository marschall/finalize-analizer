package com.github.marschall.finalizeanalizer.agent;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.PrintStream;
import java.lang.instrument.Instrumentation;

public class Agent {

  public static void agentmain(String argString, Instrumentation instrumentation) {
    String[] args = parseArgs(argString);
    PrintStream out;
    boolean needsClose;
    if (args.length > 0) {
      needsClose = false;
      try {
        out = new PrintStream(new File(args[0]));
      } catch (FileNotFoundException e) {
        e.printStackTrace(System.err);
        return;
      }
    } else {
      needsClose = false;
      out = System.out;
    }
    try {
      for (Class<?> clazz : instrumentation.getAllLoadedClasses()) {
        hasFinalizer(clazz, out);
      }
    } finally {
      if (needsClose) {
        // we don't want to close System.out
        out.close();
      }
    }
  }

  private static String[] parseArgs(String argString) {
    if ((argString == null) || argString.isBlank()) {
      return new String[0];
    }
    return argString.split(" ");
  }

  private static boolean shouldSkip(Class<?> clazz) {
    if ((clazz == Object.class) || (clazz == Enum.class)) {
      return true;
    }
    // maybe  clazz.isRecord()
    if (clazz.isArray() || clazz.isAnnotation() || clazz.isEnum() || clazz.isPrimitive()) {
      return true;
    }
    String className = clazz.getName();
    if (className.startsWith("java.") || className.startsWith("jdk.")) {
      return true;
    }
    return false;
  }

  private static void hasFinalizer(Class<?> clazz, PrintStream out) {
    if (shouldSkip(clazz)) {
      return;
    }
    Thread currentThread = Thread.currentThread();
    ClassLoader previous = currentThread.getContextClassLoader();
    // hopefully with the TCCL set all types referenced by method signatures can be loaded
    currentThread.setContextClassLoader(clazz.getClassLoader());
    try {
      if (hasFinalizerMethod(clazz)) {
        out.println(clazz.getName());
      }
    } finally {
      currentThread.setContextClassLoader(previous);
    }
  }

  private static boolean hasFinalizerMethod(Class<?> clazz) {
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
