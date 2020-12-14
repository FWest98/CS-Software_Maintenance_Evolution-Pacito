import org.gradle.internal.jvm.Jvm

plugins {
    `cpp-library`
}

version = "1.2.1"

repositories {
    jcenter()
}

library {
    source.from(file("src/main/cpp"))
    privateHeaders.from(file("src/main/cpp"))
    publicHeaders.from(file("src/main/headers"))

    targetMachines.add(machines.linux.x86_64)
    targetMachines.add(machines.windows.x86_64)
    targetMachines.add(machines.macOS.x86_64)

    binaries.configureEach {
        compileTask.get().includes.from("${Jvm.current().javaHome}/include")
        compileTask.get().includes.from(".")

        val os = targetPlatform.targetMachine.operatingSystemFamily
        when {
            os.isMacOs -> compileTask.get().includes.from("${Jvm.current().javaHome}/include/darwin")
            os.isLinux -> compileTask.get().includes.from("${Jvm.current().javaHome}/include/linux")
            os.isWindows -> compileTask.get().includes.from("${Jvm.current().javaHome}/include/win32")
        }

        compileTask.get().compilerArgs.addAll(listOf("-Wl,-rpath,/usr/local/lib", "-Wl,-z,defs", "-L/usr/local/lib"))
        compileTask.get().compilerArgs.addAll(listOf("-std=c++20"))
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