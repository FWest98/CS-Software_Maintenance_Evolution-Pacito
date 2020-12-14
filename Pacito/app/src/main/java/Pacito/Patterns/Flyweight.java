package Pacito.Patterns;

import lombok.Getter;

import java.util.Objects;

@Getter
public class Flyweight extends Pattern {
    public String ObjectType;
    public String Factory;
    public String FactoryMethod;
    public String Pool;
    public String Object;
    public String File;
    public boolean IsImmutable;

    @Override
    public void cleanPaths(String base) {
        File = cleanPath(File, base);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Flyweight flyweight = (Flyweight) o;
        return IsImmutable == flyweight.IsImmutable && Objects.equals(ObjectType, flyweight.ObjectType) && Objects.equals(Factory, flyweight.Factory) && Objects.equals(FactoryMethod, flyweight.FactoryMethod) && Objects.equals(Pool, flyweight.Pool) && Objects.equals(Object, flyweight.Object) && Objects.equals(File, flyweight.File);
    }

    @Override
    public int hashCode() {
        return Objects.hash(ObjectType, Factory, FactoryMethod, Pool, Object, File, IsImmutable);
    }
}
