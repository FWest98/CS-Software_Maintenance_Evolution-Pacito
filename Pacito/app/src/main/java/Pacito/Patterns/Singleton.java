package Pacito.Patterns;

import lombok.Getter;

import java.util.Objects;

@Getter
public class Singleton extends Pattern {
    public String Singleton;
    public String Instance;
    public String Creator;
    public String File;

    @Override
    public void cleanPaths(String base) {
        File = cleanPath(File, base);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Singleton singleton = (Singleton) o;
        return IsMultithreaded == singleton.IsMultithreaded && Objects.equals(Singleton, singleton.Singleton) && Objects.equals(Instance, singleton.Instance) && Objects.equals(Creator, singleton.Creator) && Objects.equals(File, singleton.File);
    }

    @Override
    public int hashCode() {
        return Objects.hash(Singleton, Instance, Creator, File, IsMultithreaded);
    }

    public boolean IsMultithreaded;
}
