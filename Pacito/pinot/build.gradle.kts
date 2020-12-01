import org.gradle.internal.jvm.Jvm

plugins {
    `cpp-library`
}

version = "1.2.1"

repositories {
    jcenter()
}

library {
    targetMachines.add(machines.linux.x86_64)
    targetMachines.add(machines.windows.x86_64)
    targetMachines.add(machines.macOS.x86_64)

    binaries.configureEach {
        compileTask.get().includes.from("${Jvm.current().javaHome}/include")

        val os = targetPlatform.targetMachine.operatingSystemFamily
        when {
            os.isMacOs -> compileTask.get().includes.from("${Jvm.current().javaHome}/include/darwin")
            os.isLinux -> compileTask.get().includes.from("${Jvm.current().javaHome}/include/linux")
            os.isWindows -> compileTask.get().includes.from("${Jvm.current().javaHome}/include/win32")
        }
    }
}

tasks.withType<CppCompile> {
    dependsOn(":app:copyHeaders")
}

// Copy output binary
tasks {
    task("copyBinary", Copy::class) {
        dependsOn("build")
        from("build/lib")
        include("**/*")
        eachFile {
            path = name
        }
        into("build/libs")
    }
}