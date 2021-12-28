Finalize Analizer
=================

Tools for figuring out where you use finalizers in your application, see [JEP 421: Deprecate Finalization for Removal](https://openjdk.java.net/jeps/421). It currently comes with the following tools:

* a Maven plugin that searches for finalizers in dependencies
* a Java agent that lists all loaded classes with finalizers
* a native agent that lists all loaded classes with finalizers

Pros and Cons of the Agents
---------------------------

* The Java agent is easily portable. You can just download the JAR and run it on any architecture or operating system. However it has the following disadvantages:
  * It struggles to analyze some classes where not all types of all methods can be loaded. This should not be an issue as such classes should not have any instances.
* The native agent requires compilation for your specific architecture or operating system. However it has the following advantages:
  * It ignores loaded classes who have not yet been initialized. Such classes never had an instance created.
  * It does not allocate any Java heap memory.


Running the Java Agent
----------------------

    jcmd <pid> JVMTI.agent_load $(pwd)/agent/target/finalize-analizer-agent-1.0.1-SNAPSHOT.jar <path>
    jcmd 143830 JVMTI.agent_load $(pwd)/agent/target/finalize-analizer-agent-1.0.1-SNAPSHOT.jar $(pwd)/eclipse.txt


Running the Native Agent
------------------------

    jcmd <pid> JVMTI.agent_load $(pwd)/agent/target/nar/finalize-analizer-agent-1.0.1-SNAPSHOT-amd64-Linux-gpp-jni/lib/amd64-Linux-gpp/jni/libfinalize-analizer-agent-1.0.1-SNAPSHOT.so
    jcmd 3052 JVMTI.agent_load $(pwd)/agent/target/nar/finalize-analizer-agent-1.0.1-SNAPSHOT-amd64-Linux-gpp-jni/lib/amd64-Linux-gpp/jni/libfinalize-analizer-agent-1.0.1-SNAPSHOT.so


Running the Maven Plugin
------------------------

```xml
<build>
  <plugins>
    <plugin>
      <groupId>com.github.marschall</groupId>
      <artifactId>finalize-analizer-maven-plugin</artifactId>
      <version>1.0.0</version>
      <executions>
        <execution>
          <id>find-finalizers</id>
          <goals>
            <goal>analize</goal>
          </goals>
        </execution>
      </executions>
    </plugin>
  </plugins>
</build>
```

Per default the plugin will run in the `verify` phase. The plugin will cause the build to fail should a dependency with a finalizer be found.

