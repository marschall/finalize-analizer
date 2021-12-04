package com.github.marschall.finalizeanalizer.plugin;

import java.io.File;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import io.takari.maven.testing.TestResources;
import io.takari.maven.testing.executor.MavenExecution;
import io.takari.maven.testing.executor.MavenExecutionResult;
import io.takari.maven.testing.executor.MavenRuntime;
import io.takari.maven.testing.executor.MavenRuntime.MavenRuntimeBuilder;
import io.takari.maven.testing.executor.MavenVersions;
import io.takari.maven.testing.executor.junit.MavenJUnitTestRunner;

@RunWith(MavenJUnitTestRunner.class)
@MavenVersions("3.8.4")
public class AnalizeMojoTests {

  @Rule
  public final TestResources resources = new TestResources();

  private final MavenRuntime mavenRuntime;

  public AnalizeMojoTests(MavenRuntimeBuilder builder) throws Exception {
    this.mavenRuntime = builder
            .withCliOptions("--batch-mode")
            .build();
  }

  @Test
  public void commonsCompress() throws Exception {
    File basedir = this.resources.getBasedir("commons-compress");
    MavenExecution execution = this.mavenRuntime.forProject(basedir);

    MavenExecutionResult result = execution.execute("clean", "verify");
    result.assertLogText("[WARNING] The dependency: org.apache.commons:commons-compress:jar:1.21 (compile) has a finalizer in the calss: org/apache/commons/compress/archivers/zip/ZipFile.class");
    result.assertLogText("[WARNING] The dependency: org.apache.commons:commons-compress:jar:1.21 (compile) has a finalizer in the calss: org/apache/commons/compress/compressors/bzip2/BZip2CompressorOutputStream.class");
    result.assertLogText("[INFO] BUILD FAILURE");
  }

}
