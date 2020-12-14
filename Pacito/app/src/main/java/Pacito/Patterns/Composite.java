package Pacito.Patterns;

import lombok.Getter;

import java.util.Objects;

@Getter
public class Composite extends Pattern {
    public String CompositeClass;
    public String Instance;
    public String ComponentClass;
    public String File;

    @Override
    public void cleanPaths(String base) {
        File = cleanPath(File, base);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Composite composite = (Composite) o;
        return Objects.equals(CompositeClass, composite.CompositeClass) && Objects.equals(Instance, composite.Instance) && Objects.equals(ComponentClass, composite.ComponentClass) && Objects.equals(File, composite.File);
    }

    @Override
    public int hashCode() {
        return Objects.hash(CompositeClass, Instance, ComponentClass, File);
    }
}
