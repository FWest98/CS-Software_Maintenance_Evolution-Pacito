package Pacito;

import Pacito.Patterns.Pattern;
import lombok.Getter;
import org.eclipse.jgit.revwalk.RevCommit;

import java.io.IOException;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

// Class representing interaction with Pinot
public class Pinot extends JniObject {
    static {
        try {
            NativeUtils.loadLibraryFromJar("/libpinot.so");
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    // Lifecycle methods
    private boolean isInitialized = false;
    private boolean isClean = true;
    private void ensureInitialized() throws UnsupportedOperationException {
        if(!isInitialized || isClean)
            throw new UnsupportedOperationException("Pinot is not yet initialized!");
    }
    private void ensureClean() throws UnsupportedOperationException {
        if(isInitialized || !isClean)
            throw new UnsupportedOperationException("Pinot has already been initialized!");
    }

    private boolean hasRun = false;
    private void ensureHasRun() throws UnsupportedOperationException {
        if(!hasRun)
            throw new UnsupportedOperationException("Pinot has not yet run!");
    }

    private native void initialize(String classPath);
    private native void clean();

    // Operations methods
    @Getter private RunStats result = null;
    private native RunStats run(String[] files);

    @Getter private List<Pattern> patterns = null;
    private native Pattern[] findAdapter();
    private native Pattern[] findBridge();
    private native Pattern[] findComposite();
    private native Pattern[] findCoR();
    private native Pattern[] findFacade();
    private native Pattern[] findFactory();
    private native Pattern[] findFlyweight();
    private native Pattern[] findMediator();
    private native Pattern[] findObserver();
    private native Pattern[] findProxy();
    private native Pattern[] findSingleton();
    private native Pattern[] findStrategy();
    private native Pattern[] findTemplateMethod();
    private native Pattern[] findVisitor();

    @Getter private RevCommit commit;
    @Getter private int number;

    // Public methods
    public Pinot(RevCommit commit, int number) {
        this.commit = commit;
        this.number = number;
    }

    public void initialize(List<String> classPath) throws UnsupportedOperationException {
        ensureClean();

        initialize(classPath.stream().reduce("", (s1, s2) -> s1 + ":" + s2));
        isInitialized = true;
        isClean = false;
    }

    public RunStats run(List<Path> files) throws UnsupportedOperationException {
        ensureInitialized();

        result = run(files.stream().map(Path::toString).toArray(String[]::new));
        hasRun = true;

        return result;
    }

    public void cleanUp() throws UnsupportedOperationException {
        ensureInitialized();
        clean();

        isClean = true;
        isInitialized = false;
    }

    public List<Pattern> findPatterns() throws UnsupportedOperationException {
        ensureHasRun();

        patterns = new ArrayList<>();
        patterns.addAll(Arrays.asList(findAdapter()));
        patterns.addAll(Arrays.asList(findBridge()));
        patterns.addAll(Arrays.asList(findComposite()));
        patterns.addAll(Arrays.asList(findCoR()));
        patterns.addAll(Arrays.asList(findFacade()));
        patterns.addAll(Arrays.asList(findFactory()));
        patterns.addAll(Arrays.asList(findFlyweight()));
        patterns.addAll(Arrays.asList(findMediator()));
        patterns.addAll(Arrays.asList(findObserver()));
        patterns.addAll(Arrays.asList(findProxy()));
        patterns.addAll(Arrays.asList(findSingleton()));
        patterns.addAll(Arrays.asList(findStrategy()));
        patterns.addAll(Arrays.asList(findTemplateMethod()));
        patterns.addAll(Arrays.asList(findVisitor()));

        return patterns;
    }

    public void cleanPaths(Path baseDirectory) {
        ensureHasRun();

        patterns.forEach(s -> s.cleanPaths(baseDirectory.toString()));
    }

    public static class RunStats {
        public int returnCode;
        public int numClasses;
        public int numFiles;
        public int numDelegations;
        public int numConcreteClassNodes;
        public int numUndirectedInvocationEdges;
    }
}
