package Pacito.Patterns;

import lombok.Getter;

import java.util.Arrays;
import java.util.Objects;

@Getter
public class Factory extends Pattern {
    public String FactoryMethodClass;
    public String FactoryMethodImplementation;
    public String FactoryMethod;
    public String[] FactoryMethodResults;
    public String FactoryMethodResultBase;
    public String File;

    @Override
    public void cleanPaths(String base) {
        File = cleanPath(File, base);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Factory factory = (Factory) o;
        return Objects.equals(FactoryMethodClass, factory.FactoryMethodClass) && Objects.equals(FactoryMethodImplementation, factory.FactoryMethodImplementation) && Objects.equals(FactoryMethod, factory.FactoryMethod) && Arrays.equals(FactoryMethodResults, factory.FactoryMethodResults) && Objects.equals(FactoryMethodResultBase, factory.FactoryMethodResultBase) && Objects.equals(File, factory.File);
    }

    @Override
    public int hashCode() {
        int result = Objects.hash(FactoryMethodClass, FactoryMethodImplementation, FactoryMethod, FactoryMethodResultBase, File);
        result = 31 * result + Arrays.hashCode(FactoryMethodResults);
        return result;
    }
}
