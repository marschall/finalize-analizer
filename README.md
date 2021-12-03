

Running the Java Agent
----------------------

jcmd <pid> JVMTI.agent_load $(pwd)/target/finalize-analizer-1.0.0-SNAPSHOT.jar


Running the Native Agent
------------------------

jcmd <pid> JVMTI.agent_load $(pwd)/target/nar/finalize-analizer-1.0.0-SNAPSHOT-amd64-Linux-gpp-jni/lib/amd64-Linux-gpp/jni/libfinalize-analizer-1.0.0-SNAPSHOT.so
