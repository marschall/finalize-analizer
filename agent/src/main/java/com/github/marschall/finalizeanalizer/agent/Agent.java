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
        searchForFinalizers(clazz);
      }
    } finally {
      if (needsClose) {
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

  private static void searchForFinalizers(Class<?> clazz) {
    if (clazz == Object.class) {
      return;
    }
    if (clazz.isArray() || clazz.isAnnotation() || clazz.isEnum() || clazz.isPrimitive() || clazz.isRecord()) {
      return;
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
