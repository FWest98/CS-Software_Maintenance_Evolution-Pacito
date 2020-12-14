package Pacito.Patterns;

import lombok.Getter;

import java.util.Arrays;
import java.util.Objects;

@Getter
public class Mediator extends Pattern {
    public String Mediator;
    public String[] Colleagues;
    public String File;

    @Override
    public void cleanPaths(String base) {
        File = cleanPath(File, base);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Mediator mediator = (Mediator) o;
        return Objects.equals(Mediator, mediator.Mediator) && Arrays.equals(Colleagues, mediator.Colleagues) && Objects.equals(File, mediator.File);
    }

    @Override
    public int hashCode() {
        int result = Objects.hash(Mediator, File);
        result = 31 * result + Arrays.hashCode(Colleagues);
        return result;
    }
}
