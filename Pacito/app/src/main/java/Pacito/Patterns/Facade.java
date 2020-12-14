package Pacito.Patterns;

import lombok.Getter;

import java.util.Arrays;
import java.util.Objects;

@Getter
public class Facade extends Pattern {
    public String Facade;
    public String[] Hidden;
    public String[] Access;
    public String File;

    @Override
    public void cleanPaths(String base) {
        File = cleanPath(File, base);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Facade facade = (Facade) o;
        return Objects.equals(Facade, facade.Facade) && Arrays.equals(Hidden, facade.Hidden) && Arrays.equals(Access, facade.Access) && Objects.equals(File, facade.File);
    }

    @Override
    public int hashCode() {
        int result = Objects.hash(Facade, File);
        result = 31 * result + Arrays.hashCode(Hidden);
        result = 31 * result + Arrays.hashCode(Access);
        return result;
    }
}
