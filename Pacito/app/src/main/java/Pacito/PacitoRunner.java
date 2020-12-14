package Pacito;

import lombok.SneakyThrows;
import org.apache.maven.model.io.xpp3.MavenXpp3Reader;
import org.apache.maven.model.io.xpp3.MavenXpp3Writer;
import org.apache.maven.shared.invoker.*;
import org.codehaus.plexus.util.xml.pull.XmlPullParserException;
import org.eclipse.jgit.api.Git;
import org.eclipse.jgit.api.ResetCommand;
import org.eclipse.jgit.api.errors.GitAPIException;
import org.eclipse.jgit.revwalk.RevCommit;

import java.io.*;
import java.nio.file.*;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Properties;
import java.util.concurrent.Callable;
import java.util.stream.Collectors;

public class PacitoRunner implements Callable<Pinot> {
    private final RevCommit commit;
    private final int number;
    private int threadNumber;
    private final boolean verbose;
    private final Path workingDirectory;
    private Path sourceDirectory;
    private Path mavenDirectory;
    private List<PathMatcher> excludes;

    private Git git;
    private Pinot pinot;
    private final MavenXpp3Writer mavenWriter = new MavenXpp3Writer();
    private final MavenXpp3Reader mavenReader = new MavenXpp3Reader();
    private Invoker mavenInvoker = null;
    private final ByteArrayOutputStream mavenOutput = new ByteArrayOutputStream();

    public PacitoRunner(int number, Path workingDirectory, RevCommit commit, List<PathMatcher> excludes, Path mavenExecutable, boolean verbose) {
        this.number = number;
        this.commit = commit;
        this.excludes = excludes;
        this.verbose = verbose;
        this.workingDirectory = workingDirectory;

        if(mavenExecutable != null) {
            var outputHandler = new PrintStreamHandler(new PrintStream(mavenOutput), false);

            mavenInvoker = new DefaultInvoker();
            mavenInvoker.setMavenExecutable(mavenExecutable.toFile());
            mavenInvoker.setMavenHome(mavenExecutable.getParent().toFile());
            mavenInvoker.setOutputHandler(outputHandler);
            mavenInvoker.setErrorHandler(outputHandler);
        }
    }

    @SneakyThrows(UnsupportedOperationException.class)
    @Override
    public Pinot call() {
        if(verbose) System.out.println("Initializing runner");
        // Find directory for this task based on thread number
        var threadName = Thread.currentThread().getName();
        threadNumber = Integer.parseInt(threadName.substring("pacito-".length()));
        this.sourceDirectory = workingDirectory.resolve("pacito" + threadNumber);
        this.mavenDirectory = workingDirectory.resolve("pacito-maven" + threadNumber);

        if(mavenInvoker != null) {
            mavenInvoker.setLocalRepositoryDirectory(mavenDirectory.toFile());
            mavenInvoker.setWorkingDirectory(sourceDirectory.toFile());
        }

        // Git work
        if(verbose) System.out.println("Checking out commit " + commit.getName() + " and cleaning repository");
        try {
            // Open repository
            git = Git.open(sourceDirectory.toFile());

            // Checkout commit
            git.reset().setRef(commit.getName()).setMode(ResetCommand.ResetType.HARD).call();

            // Clean dir
            git.clean().setCleanDirectories(true).setForce(true).call();
        } catch (IOException | GitAPIException e) {
            e.printStackTrace();
            return null;
        }

        // Maven work
        var classpath = new ArrayList<String>();
        classpath.add(workingDirectory.resolve("rt.jar").toString());
        if(mavenInvoker != null) {
            if (verbose) System.out.println("Adapting Maven POM files");
            var modules = findModules();
            var pomFiles = findFiles("**pom.xml", sourceDirectory);
            for (var pomFile : pomFiles) clearModuleDependencies(pomFile, modules);

            if (verbose) System.out.println("Downloading and gathering dependent packages");
            var dependencyFolder = sourceDirectory.resolve("dependencies");
            copyDependencies(dependencyFolder);
            classpath.addAll(findFiles("*.jar", dependencyFolder).stream().map(Path::toString).collect(Collectors.toList()));
        }

        // Call Pinot
        if(verbose) System.out.println("Initializing Pinot");
        pinot = new Pinot(commit, number);
        pinot.initialize(classpath);

        if(verbose) System.out.println("Running Pinot");
        var files = findFiles("*.java", sourceDirectory);
        pinot.run(files);

        if(verbose) System.out.println("Finding Patterns");
        pinot.findPatterns();
        pinot.cleanPaths(sourceDirectory);
        pinot.cleanUp();

        // Print output
        System.out.println(number + ": found " + pinot.getResult().numClasses + " classes in commit " + commit.getName());

        return pinot;
    }

    private List<Path> findFiles(String pattern, Path directory) {
        var paths = new ArrayList<Path>();
        var matcher = FileSystems.getDefault().getPathMatcher("glob:"+pattern);

        try {
            Files.walkFileTree(directory, new SimpleFileVisitor<>() {
                @Override
                public FileVisitResult visitFile(Path file, BasicFileAttributes attrs) {
                    // Return all files fitting the pattern, but not matching an exclude
                    if (matcher.matches(file.getFileName())
                            && excludes.stream().noneMatch(s -> s.matches(file))) {
                        paths.add(file);
                    }

                    return FileVisitResult.CONTINUE;
                }
            });
        } catch(IOException ignored) {}

        return paths;
    }

    private List<String> findModules() {
        try {
            var rootPom = sourceDirectory.resolve("pom.xml").toFile();
            var reader = new FileReader(rootPom);
            var model = mavenReader.read(reader);
            reader.close();

            return model.getModules();
        } catch (XmlPullParserException ignored) {
            System.out.println("Could not read POM file");
        } catch(IOException ignored) {
            // This should never happen
        }
        return new ArrayList<>();
    }

    private void clearModuleDependencies(Path file, List<String> modules) {
        try {
            var reader = new FileReader(file.toFile());
            var model = mavenReader.read(reader);
            reader.close();

            model.getDependencies().removeIf(s -> modules.stream().anyMatch(a -> s.getArtifactId().contains(a)));

            var writer = new FileWriter(file.toFile());
            mavenWriter.write(writer, model);
            writer.close();
        } catch (XmlPullParserException ignored) {
            System.out.println("Could not read POM file");
        } catch (IOException ignored) {
            // this should never happen
        }
    }

    private void copyDependencies(Path output) {
        try {
            Files.createDirectories(output);

            var properties = new Properties();
            properties.put("outputDirectory", output.toAbsolutePath().toString());

            var request = new DefaultInvocationRequest();
            request.setPomFile(sourceDirectory.resolve("pom.xml").toFile());
            request.setGoals(Collections.singletonList("dependency:copy-dependencies"));
            request.setBaseDirectory(sourceDirectory.toFile());
            request.setProperties(properties);
            request.setBatchMode(true);
            request.setReactorFailureBehavior(InvocationRequest.ReactorFailureBehavior.FailNever);

            var result = mavenInvoker.execute(request);

            if(result.getExitCode() != 0 && verbose) System.out.println("Could not copy all dependencies");
        } catch (IOException | MavenInvocationException e) {
            e.printStackTrace();
        }
    }
}
