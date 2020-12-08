package Pacito;

import lombok.SneakyThrows;
import org.eclipse.jgit.api.Git;
import org.eclipse.jgit.api.ResetCommand;
import org.eclipse.jgit.api.errors.GitAPIException;
import org.eclipse.jgit.revwalk.RevCommit;

import java.io.IOException;
import java.nio.file.*;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.Callable;

public class PacitoRunner implements Callable<Pinot> {
    private Path directory;
    private Git git;
    private final RevCommit commit;
    private Pinot pinot;
    private final int number;

    public PacitoRunner(int number, Path directory, RevCommit commit) {
        this.directory = directory;
        this.number = number;
        this.commit = commit;
    }

    @SneakyThrows(UnsupportedOperationException.class)
    @Override
    public Pinot call() {
        // Find directory for this task based on thread number
        var threadName = Thread.currentThread().getName();
        var threadNumber = Integer.parseInt(threadName.substring("pacito-".length()));

        directory = directory.resolve("pacito" + threadNumber);

        // Git work
        try {
            // Open repository
            git = Git.open(directory.toFile());

            // Checkout commit
            git.reset().setRef(commit.getName()).setMode(ResetCommand.ResetType.HARD).call();

            // Clean dir
            git.clean().setCleanDirectories(true).setForce(true).call();
        } catch (IOException | GitAPIException e) {
            e.printStackTrace();
            return null;
        }

        // Call Pinot
        pinot = new Pinot();
        pinot.initialize(Collections.singletonList("/d/Documents/Studie/Informatica/Software Maintenance and Evolution/Pinot/lib/rt-1.7.jar"));

        var files = findFiles("*.java", directory);
        pinot.run(files);
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
                    if (matcher.matches(file.getFileName())) {
                        paths.add(file);
                    }

                    return FileVisitResult.CONTINUE;
                }
            });
        } catch(IOException ignored) {}

        return paths;
    }
}
