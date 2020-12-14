package Pacito.Patterns;

import lombok.Getter;

import java.util.Objects;

@Getter
public class Bridge extends Pattern {
    public String Delegator;
    public String DelegatorFile;
    public String Delegated;
    public String DelegatedFile;

    @Override
    public void cleanPaths(String base) {
        DelegatorFile = cleanPath(DelegatorFile, base);
        DelegatedFile = cleanPath(DelegatedFile, base);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Bridge bridge = (Bridge) o;
        return Objects.equals(Delegator, bridge.Delegator) && Objects.equals(DelegatorFile, bridge.DelegatorFile) && Objects.equals(Delegated, bridge.Delegated) && Objects.equals(DelegatedFile, bridge.DelegatedFile);
    }

    @Override
    public int hashCode() {
        return Objects.hash(Delegator, DelegatorFile, Delegated, DelegatedFile);
    }
}
