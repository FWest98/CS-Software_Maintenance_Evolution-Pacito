import org.gradle.internal.jvm.Jvm

// Base setup
plugins {
    `cpp-library`
}

version = "1.2.1"

repositories {
    jcenter()
}

// Add JNI Binary configuration for output of the library to Java
val jniBinary: Configuration by configurations.creating {
    isCanBeConsumed = true
    isCanBeResolved = false

    // Selection attributes
    attributes {
        attribute(Usage.USAGE_ATTRIBUTE, objects.named(Usage.NATIVE_RUNTIME))
    }
}

// Add JNI Headers configuration for import of the generated headers from Java
val jniHeaders: Configuration by configurations.creating {
    isCanBeResolved = true
    isCanBeConsumed = false

    // Selection attributes
    attributes {
        attribute(Usage.USAGE_ATTRIBUTE, objects.named(Usage.C_PLUS_PLUS_API))
    }

    // Define task that processes the header files from Java
    val jniHeaderTask by tasks.registering(DefaultTask::class) {
        dependsOn(jniHeaders) // resolves the dependencies
        doLast {
            library.publicHeaders.from(jniHeaders) // set the header files to compile
        }
    }

    // Indicate that some public headers are built by the processing task so that it gets executed in time
    library.publicHeaders.builtBy(jniHeaderTask)
}

// Add dependencies for this project, just the jni headers from Java
dependencies {
    jniHeaders(project(":app"))
}

// C++ specific configuration
library {
    // Set source files
    source.from(file("src/main/cpp"))
    privateHeaders.from(file("src/main/cpp"))
    publicHeaders.from(file("src/main/headers"))

    // Set supported target OSs
    targetMachines.add(machines.linux.x86_64)
    targetMachines.add(machines.windows.x86_64) // not tested as Jikes does not support it
    targetMachines.add(machines.macOS.x86_64)

    // Configure the resulting binaries
    binaries.configureEach {
        // Include Java headers for compilation
        compileTask.get().includes.from("${Jvm.current().javaHome}/include")
        compileTask.get().includes.from(".")

        val os = targetPlatform.targetMachine.operatingSystemFamily
        when {
            os.isMacOs -> compileTask.get().includes.from("${Jvm.current().javaHome}/include/darwin")
            os.isLinux -> compileTask.get().includes.from("${Jvm.current().javaHome}/include/linux")
            os.isWindows -> compileTask.get().includes.from("${Jvm.current().javaHome}/include/win32")
        }

        // Set some flags
        compileTask.get().compilerArgs.addAll(listOf("-Wl,-rpath,/usr/local/lib", "-Wl,-z,defs", "-L/usr/local/lib"))
        compileTask.get().compilerArgs.addAll(listOf("-std=c++20"))

        // Add artifact for the compiled debug-shared lib
        if(this is CppSharedLibrary && isDebuggable && !isOptimized) {
            artifacts.add(jniBinary.name, linkFile) {
                builtBy(linkTask)
            }
        }
    }
}