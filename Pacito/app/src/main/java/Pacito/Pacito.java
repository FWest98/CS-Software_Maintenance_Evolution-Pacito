package Pacito;

import Pacito.Patterns.PatternResult;
import org.eclipse.jgit.api.Git;
import org.eclipse.jgit.revwalk.RevCommit;
import org.json.JSONArray;
import picocli.CommandLine;
import picocli.CommandLine.Model.CommandSpec;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.file.*;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.*;
import java.util.stream.Collectors;

import static java.nio.file.StandardCopyOption.REPLACE_EXISTING;

@CommandLine.Command(name = "Pacito", mixinStandardHelpOptions = true)
public class Pacito implements Callable<Integer> {
    @CommandLine.Spec CommandSpec spec;

    private Path source;
    @CommandLine.Option(names = {"-s", "--source"}, required = true, description = "Source directory (required)")
    private void setSource(Path source) {
        // Check whether value is valid
        if(!Files.isDirectory(source))
            throw new CommandLine.ParameterException(spec.commandLine(),
                    String.format("Invalid value '%s' for directory: path is not a directory.", source));

        this.source = source;
    }

    private Path workingDirectory;
    @CommandLine.Option(names = {"-wd", "--working-dir"}, description = "Working directory (default to /tmp)", defaultValue = "/tmp")
    private void setWorkingDirectory(Path directory) {
        // Check whether this is a directory we can write into
        if(!Files.isDirectory(directory))
            throw new CommandLine.ParameterException(spec.commandLine(),
                    String.format("Invalid value '%s' for working directory: path is not a directory.", directory));

        if(!Files.isWritable(directory))
            throw new CommandLine.ParameterException(spec.commandLine(),
                    String.format("No write access to working directory '%s'", directory));

        this.workingDirectory = directory;
    }

    @CommandLine.Option(names = {"-o", "--output"}, description = "Result output file (default to SD/pacito.out, relative to SD)", defaultValue = "pacito.out")
    private String outputFileName;
    private BufferedWriter output;
    private Path outputFile;

    @CommandLine.Option(names = {"-b", "--branch"}, description = "Branch to analyse (@ = current)")
    private String branch = "@";

    private int threads = 1;
    @CommandLine.Option(names = {"-n", "--threads"}, description = "Number of threads (default ${DEFAULT-VALUE}, 0 = all cores)")
    private void setThreads(int threads) {
        this.threads = threads == 0 ? Runtime.getRuntime().availableProcessors() : threads;
    }

    @CommandLine.Option(names = {"--start"}, description = "Starting commit number or name (default 0, first commit)")
    private String start = "";

    @CommandLine.Option(names = {"--end"}, description = "Ending commit number or name (default last commit, inclusive)")
    private String end = "";

    private List<PathMatcher> excludeFilters = new ArrayList<>();
    @CommandLine.Option(names = {"--exclude"}, description = "Filter for files to exclude (glob pattern)")
    private void setExcludeFilters(String[] excludes) {
        if(excludes == null) {
            excludeFilters = new ArrayList<>();
            return;
        }

        // Make path matcher from exclude filter
        excludeFilters = Arrays.stream(excludes).map(s -> FileSystems.getDefault().getPathMatcher("glob:"+s)).collect(Collectors.toList());
    }

    @CommandLine.Option(names = {"-v"}, description = "Whether to print verbose output")
    private boolean verbose = false;

    @CommandLine.Option(names = {"--mvn"}, description = "Custom Maven executable (default searches in PATH)")
    private Path mavenExecutable = null;

    @CommandLine.Option(names = {"--do-mvn"}, description = "Perform Maven package download or not", negatable = true)
    private boolean maven = false;

    private Git git;

    @Override
    public Integer call() throws Exception {
        // Test output file location is valid
        if(outputFileName.startsWith("/")) // absolute name
            outputFile = Path.of(outputFileName);
        else
            outputFile = source.resolve(outputFileName);

        try {
            output = new BufferedWriter(new FileWriter(outputFile.toString(), false));
        } catch(IOException ignored) {
            throw new CommandLine.ParameterException(spec.commandLine(),
                    "Specified output file is not writable!");
        }

        // Try and find a Maven executable
        if(verbose) System.out.println("Finding Maven executable");
        if(mavenExecutable != null) {
            if(!Files.isExecutable(mavenExecutable))
                throw new CommandLine.ParameterException(spec.commandLine(),
                        "Specified Maven executable is not executable!");
        } else if(maven) {
            // Find executable
            for(var dir : System.getenv("PATH").split(File.pathSeparator)) {
                var file = Path.of(dir, "mvn");
                if(Files.isExecutable(file)) mavenExecutable = file;
            }

            if(mavenExecutable == null)
                throw new CommandLine.ParameterException(spec.commandLine(),
                        "No Maven executable found in PATH!");
        }
        if(!maven) mavenExecutable = null;

        if(verbose) System.out.println("Finding Git commits");

        // First we go and test whether the directory is a git repo
        try {
            git = Git.open(source.toFile());
        } catch (IOException e) {
            throw new CommandLine.ParameterException(spec.commandLine(),
                    "Directory specified is not a valid git repository: " + e.getMessage());
        }

        // Set branch to current if necessary
        if(branch.equals("@"))
            branch = git.getRepository().getBranch();
        if(verbose) System.out.println("Working on branch " + branch);

        // Find all commits in reverse order (old to new)
        var commits = new ArrayList<RevCommit>();
        git.log().add(git.getRepository().resolve(branch)).call().forEach(commits::add);
        Collections.reverse(commits);

        // Find subset of commits we want
        var startOffset = findCommitOffset(start, 0, commits);
        var endOffset = findCommitOffset(end, commits.size() - 1, commits);

        if(verbose)
            System.out.println("Working on " + (endOffset - startOffset + 1) + " commits, " +
                "starting at " + commits.get(startOffset).getName() + ", " +
                "and ending at " + commits.get(endOffset).getName());

        if(verbose) System.out.println("Populating working directories for " + threads + " thread(s)");

        // Make working directory for every thread
        for(var i = 0; i < threads; i++) {
            var sourceDir = workingDirectory.resolve("pacito" + i);
            var mavenDir = workingDirectory.resolve("pacito-maven" + i);
            Files.createDirectories(sourceDir);
            Files.createDirectories(mavenDir);

            // Copy source (if thread > 0 copy from base thread since we assume this to be quick storage)
            var src = i == 0 ? source : sourceDir.resolve("../pacito0");
            copyFolder(src, sourceDir, REPLACE_EXISTING);
        }

        // Add rt.jar to working directory
        var rt = Pacito.class.getResourceAsStream("/rt.jar");
        Files.copy(rt, workingDirectory.resolve("rt.jar"), REPLACE_EXISTING);

        // Start threadpool and workers
        threads = Math.min(threads, (endOffset - startOffset + 1)); // set to commit count is too many threads
        if(verbose) System.out.println("Starting up threadpool with " + threads + " thread(s)");
        var executor = Executors.newFixedThreadPool(threads, new ThreadFactory() {
            private int currentThread = 0;

            @Override
            public Thread newThread(Runnable r) {
                var thread = new Thread(r, "pacito-" + currentThread);
                currentThread++;
                return thread;
            }
        });

        // Make tasks
        if(verbose) System.out.println("Creating tasks");
        var tasks = new ArrayList<Callable<Pinot>>();
        for(int i = startOffset; i < endOffset + 1; i++) {
            var commit = commits.get(i);
            var runner = new PacitoRunner(i, workingDirectory, commit, excludeFilters, mavenExecutable, verbose && (endOffset - startOffset == 0));
            tasks.add(runner);
        }

        if(verbose) System.out.println("Running " + tasks.size() + " tasks");
        var futureResults = executor.invokeAll(tasks);

        // Await all the results
        executor.shutdown();
        try {
            if(!executor.awaitTermination(10, TimeUnit.HOURS))
                executor.shutdownNow();
        } catch (InterruptedException ignored) {
            executor.shutdownNow();
        }

        if(verbose) System.out.println("Finished executing tasks, starting processing");

        // Process results
        var results = futureResults.stream().map(s -> {
            try {
                return s.get();
            } catch (InterruptedException | ExecutionException e) {
                e.printStackTrace();
                return null;
            }
        }).collect(Collectors.toList());

        // Cross-match patterns between commits
        var patternPool = new ArrayList<PatternResult>();
        for(var result : results) {
            // Per pattern check if pattern already exists
            for(var pattern : result.getPatterns()) {
                var existingResult = patternPool.stream().filter(s ->
                        s.pattern.equals(pattern)
                        && s.outroCommitNumber == result.getNumber() - 1
                ).findFirst();
                if(existingResult.isPresent()) {
                    // Update existing pattern
                    existingResult.get().updateOutroCommit(result.getNumber(), result.getCommit());
                } else {
                    // New pattern
                    patternPool.add(new PatternResult(pattern, result.getNumber(), result.getCommit()));
                }
            }
        }

        // Produce output file
        if(verbose) System.out.println("Finished processing, writing result file");

        var json = new JSONArray(patternPool);
        json.write(output, 4, 0);

        if(verbose) System.out.println("Analysis completed");
        output.close();

        return 0;
    }

    public static void main(String[] args) {
        int exitCode = new CommandLine(new Pacito()).execute(args);
        System.exit(exitCode);
    }

    private void copyFolder(Path source, Path dest, CopyOption... options) throws IOException {
        Files.walkFileTree(source, new SimpleFileVisitor<>() {
            @Override
            public FileVisitResult preVisitDirectory(Path dir, BasicFileAttributes attrs) throws IOException {
                Files.createDirectories(dest.resolve(source.relativize(dir)));
                return FileVisitResult.CONTINUE;
            }

            @Override
            public FileVisitResult visitFile(Path file, BasicFileAttributes attrs) throws IOException {
                Files.copy(file, dest.resolve(source.relativize(file)), options);
                return FileVisitResult.CONTINUE;
            }
        });
    }

    private int findCommitOffset(String commit, int fallback, List<RevCommit> commits) {
        if (commit.equals("")) return fallback;

        var startCommit = commits.stream().filter(s -> s.getName().equals(commit)).findFirst();
        if (startCommit.isPresent()) return commits.indexOf(startCommit.get());

        int offset;
        try {
            offset = Integer.parseInt(commit);
        } catch(NumberFormatException e) {
            offset = -1;
        }

        if(offset == -1 || offset > commits.size())
            throw new CommandLine.ParameterException(spec.commandLine(),
                    "Commit specified is not a valid commit!");

        return offset;
    }
}
