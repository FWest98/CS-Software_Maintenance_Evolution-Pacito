package Pacito;

// Class representing interaction with Pinot
public class Pinot extends JniObject {
    static {
        System.loadLibrary("pinot");
    }

    private native void initialize(String classPath);
    public native void clean();

    public native void run(String[] files);

    public Pinot(String classPath) {
        initialize(classPath);
    }
}
