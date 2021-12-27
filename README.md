

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

