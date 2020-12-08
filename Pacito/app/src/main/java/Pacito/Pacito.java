package Pacito;

import org.eclipse.jgit.api.Git;
import org.eclipse.jgit.revwalk.RevCommit;
import picocli.CommandLine;
import picocli.CommandLine.Model.CommandSpec;

import java.io.IOException;
import java.nio.file.*;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.ArrayList;
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

    private Git git;

    @Override
    public Integer call() throws Exception {
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

        // Find all commits in reverse order (old to new)
        var commits = new ArrayList<RevCommit>();
        git.log().add(git.getRepository().resolve(branch)).call().forEach(commits::add);
        Collections.reverse(commits);

        // Find subset of commits we want
        var startOffset = findCommitOffset(start, 0, commits);
        var endOffset = findCommitOffset(end, commits.size() - 1, commits);

        // Make working directory for every thread
        for(var i = 0; i < threads; i++) {
            var dir = workingDirectory.resolve("pacito" + i);
            Files.createDirectories(dir);

            // Copy source (if thread > 0 copy from base thread since we assume this to be quick storage)
            var src = i == 0 ? source : dir.resolve("../pacito0");
            copyFolder(src, dir, REPLACE_EXISTING);
        }

        System.out.println("Finished copying");

        // Start threadpool and workers
        var executor = Executors.newFixedThreadPool(threads, new ThreadFactory() {
            private int currentThread = 0;

            @Override
            public Thread newThread(Runnable r) {
                var thread = new Thread(r, "pacito-" + currentThread);
                currentThread++;
                return thread;
            }
        });
        var tasks = new ArrayList<Callable<Pinot>>();
        for(int i = startOffset; i < endOffset + 1; i++) {
            var commit = commits.get(i);
            var runner = new PacitoRunner(i, workingDirectory, commit);
            tasks.add(runner);
        }
        var futureResults = executor.invokeAll(tasks);

        // Await all the results
        executor.shutdown();
        try {
            if(!executor.awaitTermination(10, TimeUnit.HOURS))
                executor.shutdownNow();
        } catch (InterruptedException ignored) {
            executor.shutdownNow();
        }

        // Process results
        var results = futureResults.stream().map(s -> {
            try {
                return s.get();
            } catch (InterruptedException | ExecutionException e) {
                e.printStackTrace();
                return null;
            }
        }).collect(Collectors.toList());

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
