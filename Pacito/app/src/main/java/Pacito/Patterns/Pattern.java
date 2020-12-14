package Pacito.Patterns;

public abstract class Pattern {
    @Override
    public abstract boolean equals(Object obj);

    public abstract void cleanPaths(String base);
    protected String cleanPath(String path, String base) {
        if(path.startsWith(base))
            return "." + path.substring(base.length());
        return path;
    }
}