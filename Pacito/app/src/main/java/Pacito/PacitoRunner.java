package Pacito;

import org.eclipse.jgit.api.Git;
import org.eclipse.jgit.api.ResetCommand;
import org.eclipse.jgit.api.errors.GitAPIException;
import org.eclipse.jgit.revwalk.RevCommit;

import java.io.IOException;
import java.nio.file.*;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.ArrayList;
import java.util.List;

public class PacitoRunner implements Runnable {
    private Path directory;
    private Git git;
    private ArrayList<RevCommit> commits;
    private Pinot pinot;

    public PacitoRunner(Path directory, ArrayList<RevCommit> commits) {
        this.directory = directory;
        this.commits = commits;
    }

    @Override
    public void run() {
        try {
            git = Git.open(directory.toFile());
        } catch (IOException e) {
            e.printStackTrace();
        }
        pinot = new Pinot("/d/Documents/Studie/Informatica/Software Maintenance and Evolution/Pinot/lib/rt-1.7.jar");

        var commit = commits.get(2000);
        // Checkout commit
        try {
            git.reset().setRef(commit.getName()).setMode(ResetCommand.ResetType.HARD).call();
        } catch (GitAPIException e) {
            e.printStackTrace();
        }

        try {
            var files = findFiles("*.java", directory);
            pinot.run(files.stream().map(Path::toString).toArray(String[]::new));
            var cor = pinot.findCoR();
            var bridge = pinot.findBridge();
            var strategy = pinot.findStrategy();
            var flyweight = pinot.findFlyweight();
            var template = pinot.findTemplateMethod();
            var factory = pinot.findFactory();
            var visitor = pinot.findVisitor();
            var observer = pinot.findObserver();
            var mediator = pinot.findMediator();
            var proxy = pinot.findProxy();
            var adapter = pinot.findAdapter();
            var facade = pinot.findFacade();
            pinot.clean();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private List<Path> findFiles(String pattern, Path directory) throws IOException {
        var paths = new ArrayList<Path>();
        var matcher = FileSystems.getDefault().getPathMatcher("glob:"+pattern);

        Files.walkFileTree(directory, new SimpleFileVisitor<>() {
            @Override
            public FileVisitResult visitFile(Path file, BasicFileAttributes attrs) throws IOException {
                if(matcher.matches(file.getFileName())) {
                    paths.add(file);
                }

                return FileVisitResult.CONTINUE;
            }
        });

        return paths;
    }
}
