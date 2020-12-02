package Pacito;

import org.eclipse.jgit.api.Git;
import org.eclipse.jgit.revwalk.RevCommit;

import java.io.IOException;
import java.nio.file.Path;
import java.util.ArrayList;

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
        pinot = new Pinot();


    }
}
