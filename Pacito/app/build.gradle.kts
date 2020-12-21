// Base setup
plugins {
    application
    java
}

repositories {
    jcenter()
    mavenCentral()
}

// Add JNI Headers configuration for output to C++ Pinot code
val jniHeaders: Configuration by configurations.creating {
    isCanBeConsumed = true
    isCanBeResolved = false

    // Selection attributes
    attributes {
        attribute(Usage.USAGE_ATTRIBUTE, objects.named(Usage.C_PLUS_PLUS_API))
    }
}

// Add JNI Binary configuration for import of the Pinot binary
val jniBinary: Configuration by configurations.creating {
    isCanBeResolved = true
    isCanBeConsumed = false

    // Selection attributes
    attributes {
        attribute(Usage.USAGE_ATTRIBUTE, objects.named(Usage.NATIVE_RUNTIME))
    }

    // Define task that processes the binary from C++
    val jniBinaryTask by tasks.registering(DefaultTask::class) {
        dependsOn(jniBinary) // resolves the dependencies
        doLast {
            sourceSets.configureEach { resources.srcDir(jniBinary.first().parentFile) } // adds the lib as resource
        }
    }

    // Indicate that resources can only be processed after obtaining the binary
    tasks.processResources.configure { dependsOn(jniBinaryTask) }
}

dependencies {
    implementation("com.google.guava:guava:29.0-jre")
    implementation("org.eclipse.jgit:org.eclipse.jgit:5.9.0.202009080501-r")
    implementation("info.picocli:picocli:4.5.2")
    implementation("org.apache.maven:maven-model:3.6.3")
    implementation("org.apache.maven.shared:maven-invoker:3.0.1")
    implementation("org.json:json:20201115")

    compileOnly("org.projectlombok:lombok:1.18.16")

    annotationProcessor("org.projectlombok:lombok:1.18.16")
    annotationProcessor("info.picocli:picocli-codegen:4.5.2")

    runtimeOnly("org.slf4j:slf4j-simple:1.7.2") // match version to that of JGit

    jniBinary(project(":pinot"))
}

// Java specific configuration
java.sourceCompatibility = JavaVersion.VERSION_11
java.targetCompatibility = JavaVersion.VERSION_11
application {
    // Define the main class for the application.
    mainClass.set("Pacito.Pacito")
}

sourceSets {
    main {
        java {
            srcDirs(listOf("src/main/java"))
        }
        resources {
            srcDirs(listOf("src/main/resources"))
        }
    }
}

// Configure compilation
tasks.compileJava.configure {
    // Add picocli processing
    options.compilerArgs.addAll(listOf("-Aproject=${project.group}/${project.name}"))

    // Add JNI header compilation
    options.headerOutputDirectory.set(layout.buildDirectory.dir("jniHeaders"))
}

// Add artifact for exporting the JNI headers
artifacts {
    add(jniHeaders.name, tasks.compileJava.get().options.headerOutputDirectory) {
        builtBy(tasks.compileJava)
    }
}

// Task for making a fat jar with all dependencies included
val fatJar by tasks.registering(Jar::class) {
    // Include all entries in the classpath
    from(configurations.runtimeClasspath.get().map { if (it.isDirectory) it else zipTree(it) })
    // But exclude all signatures
    exclude("META-INF/*.RSA", "META-INF/*.SF", "META-INF/*.DSA")

    // Add a manifest
    manifest {
        attributes("Main-Class" to application.mainClass)
    }

    // Inherit properties from normal jar task
    with(tasks.jar.get() as CopySpec)
}