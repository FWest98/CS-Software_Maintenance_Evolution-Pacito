package Pacito.Patterns;

import lombok.Getter;

import java.util.Objects;

@Getter
public class CoR extends Pattern {
    public String Handler;
    public String HandlerMethod;
    public String PropagatorName;
    public String PropagatorType;
    public String File;

    @Override
    public void cleanPaths(String base) {
        File = cleanPath(File, base);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        CoR coR = (CoR) o;
        return Objects.equals(Handler, coR.Handler) && Objects.equals(HandlerMethod, coR.HandlerMethod) && Objects.equals(PropagatorName, coR.PropagatorName) && Objects.equals(PropagatorType, coR.PropagatorType) && Objects.equals(File, coR.File);
    }

    @Override
    public int hashCode() {
        return Objects.hash(Handler, HandlerMethod, PropagatorName, PropagatorType, File);
    }
}
