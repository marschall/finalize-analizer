package com.github.marschall.finalizeanalizer.plugin;

import static java.util.stream.Collectors.toList;
import static org.apache.maven.plugins.annotations.LifecyclePhase.VERIFY;
import static org.apache.maven.plugins.annotations.ResolutionScope.RUNTIME;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;
import java.util.stream.Stream;

import org.apache.maven.plugin.AbstractMojo;
import org.apache.maven.plugin.MojoExecutionException;
import org.apache.maven.plugin.MojoFailureException;
import org.apache.maven.plugins.annotations.Component;
import org.apache.maven.plugins.annotations.Mojo;
import org.apache.maven.plugins.annotations.Parameter;
import org.apache.maven.project.DefaultDependencyResolutionRequest;
import org.apache.maven.project.DependencyResolutionException;
import org.apache.maven.project.DependencyResolutionRequest;
import org.apache.maven.project.DependencyResolutionResult;
import org.apache.maven.project.MavenProject;
import org.apache.maven.project.ProjectDependenciesResolver;
import org.eclipse.aether.RepositorySystemSession;
import org.eclipse.aether.artifact.Artifact;
import org.eclipse.aether.graph.Dependency;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;
import org.sonatype.plexus.build.incremental.BuildContext;

/**
 * Searches for dependencies with finalizers.
 */
@Mojo(
        name = "analize",
        threadSafe = true,
        defaultPhase = VERIFY,
        requiresDependencyResolution = RUNTIME
      )
public class AnalizeMojo extends AbstractMojo {

  @Component
  private ProjectDependenciesResolver resolver;

  @Parameter(defaultValue = "${project}", readonly = true)
  private MavenProject project;

  @Parameter(defaultValue = "${repositorySystemSession}", readonly = true)
  private RepositorySystemSession repositorySession;

  @Component
  private BuildContext buildContext;

  /**
   * Whether plugin execution should be skipped.
   */
  @Parameter(defaultValue = "false", property = "finalize.analize.skip")
  private boolean skip;

  @Override
  public void execute() throws MojoExecutionException, MojoFailureException {
    if (!this.buildContext.hasDelta("pom.xml")) {
      return;
    }

    List<Dependency> dependencies = this.resolveDependencies();
    List<FoundFinalizer> foundFinalizers = new ArrayList<>();
    for (Dependency dependency : dependencies) {
      foundFinalizers.addAll(this.scanDependency(dependency));
    }
    if (!foundFinalizers.isEmpty()) {
      for (FoundFinalizer foundFinalizer : foundFinalizers) {
        this.getLog().warn("The dependency: " + foundFinalizer.getDependency() + " has a finalizer in the calss: " + foundFinalizer.getClassFile());
      }

      MojoFailureException exception = new MojoFailureException("classes with finalizers dependencies detected");
      this.buildContext.addMessage(new File("pom.xml"), 0, 0, ROLE, BuildContext.SEVERITY_ERROR, exception);
      throw exception;
    }
  }

  private List<FoundFinalizer> scanDependency(Dependency dependency) throws MojoExecutionException {
    Artifact artifact = dependency.getArtifact();
    if (artifact.getExtension().equals("jar")) {
      List<String> filesWithFinalizers = this.scanArtifact(artifact);
      //formatter:off
      return filesWithFinalizers.stream()
                                .map(fileName -> new FoundFinalizer(dependency, fileName))
                                .collect(toList());
      //formatter:on
    } else {
      return List.of();
    }
  }

  private List<String> scanArtifact(Artifact artifact) throws MojoExecutionException {
    try (JarFile jarFile = new JarFile(artifact.getFile())) {
      try (Stream<JarEntry> entryStream = jarFile.versionedStream()) {
        List<String> filesWithFinalizers = new ArrayList<>();
        //formatter:off
        Iterator<JarEntry> entryIterator = entryStream.filter(entry -> !entry.isDirectory())
                                                      .filter(entry -> !entry.getName().equals("module-info.class"))
                                                      .filter(entry -> entry.getName().endsWith(".class"))
                                                      .iterator();
        //formatter:on
        while (entryIterator.hasNext()) {
          JarEntry entry = entryIterator.next();
          boolean hasFinalizer = this.scanEntryForFinalizer(jarFile, entry);
          if (hasFinalizer) {
            filesWithFinalizers.add(entry.getName());
          }
        }
        return filesWithFinalizers;
      }
    } catch (IOException e) {
      throw new MojoExecutionException("could not open jar of: " + artifact, e);
    }
  }

  private boolean scanEntryForFinalizer(JarFile jarFile, JarEntry entry) throws IOException {
    try (InputStream inputStream = jarFile.getInputStream(entry)) {
      ClassReader classReader = new ClassReader(inputStream);
      FinalizerClassVisitor visitor = new FinalizerClassVisitor();
      classReader.accept(visitor, ClassReader.SKIP_CODE | ClassReader.SKIP_DEBUG | ClassReader.SKIP_FRAMES);
      return visitor.hasFinalizer();
    }
  }

  private List<Dependency> resolveDependencies() throws MojoExecutionException {
    DependencyResolutionRequest request = this.newDependencyResolutionRequest();

    DependencyResolutionResult result;
    try {
      result = this.resolver.resolve(request);
    } catch (DependencyResolutionException e) {
      throw new MojoExecutionException("could not resolve dependencies", e);
    }

    return result.getDependencies();
  }

  private DependencyResolutionRequest newDependencyResolutionRequest() {
    DefaultDependencyResolutionRequest request = new DefaultDependencyResolutionRequest();
    request.setMavenProject(this.project);
    request.setRepositorySession(this.repositorySession);
    return request;
  }

  static final class FoundFinalizer {

    private final Dependency dependency;
    private final String classFile;

    FoundFinalizer(Dependency dependency, String classFile) {
      this.dependency = dependency;
      this.classFile = classFile;
    }

    Dependency getDependency() {
      return this.dependency;
    }

    String getClassFile() {
      return this.classFile;
    }

  }

  static final class FinalizerClassVisitor extends ClassVisitor {

    private boolean hasFinalizer;

    FinalizerClassVisitor() {
      super(Opcodes.ASM9);
      this.hasFinalizer = false;
    }

    @Override
    public MethodVisitor visitMethod(int access, String name, String descriptor, String signature, String[] exceptions) {
      if (name.equals("finalize")) {
        this.hasFinalizer = "()V".equals(descriptor);
      }
      return null;
    }

    boolean hasFinalizer() {
      return this.hasFinalizer;
    }

  }

}
