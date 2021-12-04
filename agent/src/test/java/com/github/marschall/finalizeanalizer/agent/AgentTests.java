package com.github.marschall.finalizeanalizer.agent;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import java.io.IOException;
import java.lang.instrument.Instrumentation;
import java.nio.file.Files;
import java.nio.file.Path;

import org.junit.jupiter.api.Test;

class AgentTests {

  @Test
  void agentmain() throws IOException {
    Path tempFile = Files.createTempFile("finalizeanalizer", null);
    Files.delete(tempFile);
    Instrumentation instrumentation = mock(Instrumentation.class);
    when(instrumentation.getAllLoadedClasses()).thenReturn(new Class[] {Object.class, Enum.class, WithFinalizer.class, WithoutFinalizer.class});

    Agent.agentmain(tempFile.toAbsolutePath().toString(), instrumentation);

    assertTrue(Files.exists(tempFile));
    String content = Files.readString(tempFile);
    assertEquals(WithFinalizer.class.getName() + "\n", content);
  }

  static final class WithoutFinalizer {

  }

  static final class WithFinalizer {

    @Override
    protected void finalize() throws Throwable {
      // empty
    }

  }

}
