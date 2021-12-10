

Running the Java Agent
----------------------

    jcmd <pid> JVMTI.agent_load $(pwd)/agent/target/finalize-analizer-agent-1.0.1-SNAPSHOT.jar <path>
    jcmd 3029 JVMTI.agent_load $(pwd)/agent/target/finalize-analizer-agent-1.0.1-SNAPSHOT.jar $(pwd)/eclipse.txt


Running the Native Agent
------------------------

    jcmd <pid> JVMTI.agent_load $(pwd)/agent/target/nar/finalize-analizer-agent-1.0.1-SNAPSHOT-amd64-Linux-gpp-jni/lib/amd64-Linux-gpp/jni/libfinalize-analizer-agent-1.0.1-SNAPSHOT.so
    jcmd 3291 JVMTI.agent_load $(pwd)/agent/target/nar/finalize-analizer-agent-1.0.1-SNAPSHOT-amd64-Linux-gpp-jni/lib/amd64-Linux-gpp/jni/libfinalize-analizer-agent-1.0.1-SNAPSHOT.so

Ubuntu core dump location `/var/lib/apport/coredump/`
