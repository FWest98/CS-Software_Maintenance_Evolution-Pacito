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
    public native int run(String[] files);
    public native Pattern[] findCoR();
    public native Pattern[] findBridge();
    public native Pattern[] findStrategy();
    public native Pattern[] findFlyweight();

    public Pinot(String classPath) {
        initialize(classPath);
    }
}
