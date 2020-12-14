package Pacito.Patterns;

import lombok.Getter;

import java.util.Objects;

@Getter
public class Decorator extends Pattern {
    public String Decorator;
    public String DecorateMethod;
    public String DecorateeName;
    public String DecorateeType;
    public String File;

    @Override
    public void cleanPaths(String base) {
        File = cleanPath(File, base);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Decorator decorator = (Decorator) o;
        return Objects.equals(Decorator, decorator.Decorator) && Objects.equals(DecorateMethod, decorator.DecorateMethod) && Objects.equals(DecorateeName, decorator.DecorateeName) && Objects.equals(DecorateeType, decorator.DecorateeType) && Objects.equals(File, decorator.File);
    }

    @Override
    public int hashCode() {
        return Objects.hash(Decorator, DecorateMethod, DecorateeName, DecorateeType, File);
    }
}
