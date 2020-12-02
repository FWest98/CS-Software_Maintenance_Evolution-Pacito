package Pacito;

// Class representing interaction with Pinot
public class Pinot {
    static {
        System.loadLibrary("pinot");
    }

    public native void test();
}
