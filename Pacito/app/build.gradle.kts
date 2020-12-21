/*
 * This file was generated by the Gradle 'init' task.
 *
 * This generated file contains a sample Java application project to get you started.
 * For more details take a look at the 'Building Java & JVM projects' chapter in the Gradle
 * User Manual available at https://docs.gradle.org/6.7.1/userguide/building_java_projects.html
 */

plugins {
    // Apply the application plugin to add support for building a CLI application in Java.
    application
    java
}

repositories {
    jcenter()
    mavenCentral()
}

java.sourceCompatibility = JavaVersion.VERSION_11
java.targetCompatibility = JavaVersion.VERSION_11

dependencies {
    // This dependency is used by the application.
    implementation("com.google.guava:guava:29.0-jre")
    implementation("org.eclipse.jgit:org.eclipse.jgit:5.9.0.202009080501-r")
    implementation("info.picocli:picocli:4.5.2")
    implementation("org.apache.maven:maven-model:3.6.3")
    implementation("org.apache.maven.shared:maven-invoker:3.0.1")
    implementation("org.json:json:20201115")

    compileOnly("org.projectlombok:lombok:1.18.16")

    annotationProcessor("org.projectlombok:lombok:1.18.16")
    annotationProcessor("info.picocli:picocli-codegen:4.5.2")
}

sourceSets {
    main {
        java {
            setSrcDirs(listOf("src/main/java"))
        }
        resources {
            setSrcDirs(listOf("src/main/resources"))
        }
    }
}

application {
    // Define the main class for the application.
    mainClass.set("Pacito.Pacito")
}

tasks.withType<JavaCompile>().configureEach {
    // Add picocli processing
    options.compilerArgs.addAll(listOf("-Aproject=${project.group}/${project.name}"))

    // Add JNI header compilation
    options.compilerArgs.addAll(listOf("-h", file("${buildDir}/headers").absolutePath))
}

// Copy headers
tasks.register("copyHeaders", Copy::class) {
    dependsOn(tasks.compileJava)
    from("${buildDir}/headers")
    into("${project(":pinot").projectDir}/src/main/headers")
}

tasks.register("fatJar", Jar::class) {
    //dependsOn(":pinot:copyBinary")
    from(configurations.runtimeClasspath.get().map { if (it.isDirectory) it else zipTree(it) })
    manifest {
        attributes("Main-Class" to application.mainClass)
    }
    with(tasks.jar.get() as CopySpec)
}

tasks.processResources.configure {
    dependsOn(":pinot:copyBinary")
}

tasks.withType<Jar> {
    //dependsOn(":pinot:copyBinary")
}

tasks.withType<JavaExec> {
    //jvmArgs("-Djava.library.path=${project(":pinot").buildDir}/libs")
}