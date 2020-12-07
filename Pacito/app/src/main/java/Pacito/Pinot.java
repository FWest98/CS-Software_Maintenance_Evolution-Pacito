package Pacito;

import Pacito.Patterns.Pattern;

// Class representing interaction with Pinot
public class Pinot extends JniObject {
    static {
        System.loadLibrary("pinot");
    }

    // Lifecycle methods
    private native void initialize(String classPath);
    public native void clean();

    // Operations methods
    public native RunStats run(String[] files);
    public native Pattern[] findCoR();
    public native Pattern[] findBridge();
    public native Pattern[] findStrategy();
    public native Pattern[] findFlyweight();
    public native Pattern[] findTemplateMethod();
    public native Pattern[] findFactory();
    public native Pattern[] findVisitor();
    public native Pattern[] findObserver();
    public native Pattern[] findMediator();
    public native Pattern[] findProxy();
    public native Pattern[] findAdapter();
    public native Pattern[] findFacade();
    public native Pattern[] findSingleton();
    public native Pattern[] findComposite();

    public Pinot(String classPath) {
        initialize(classPath);
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
