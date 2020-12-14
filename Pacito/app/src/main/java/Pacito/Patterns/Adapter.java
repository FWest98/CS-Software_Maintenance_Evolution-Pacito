package Pacito.Patterns;

import lombok.Getter;

import java.util.Arrays;
import java.util.Objects;

@Getter
public class Adapter extends Pattern {
    public String[] Adapting;
    public String Adapter;
    public String Adaptee;
    public String AdapterFile;
    public String AdapteeFile;

    @Override
    public void cleanPaths(String base) {
        AdapterFile = cleanPath(AdapterFile, base);
        AdapteeFile = cleanPath(AdapteeFile, base);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Adapter adapter = (Adapter) o;
        return Arrays.equals(Adapting, adapter.Adapting) && Objects.equals(Adapter, adapter.Adapter) && Objects.equals(Adaptee, adapter.Adaptee) && Objects.equals(AdapterFile, adapter.AdapterFile) && Objects.equals(AdapteeFile, adapter.AdapteeFile);
    }

    @Override
    public int hashCode() {
        int result = Objects.hash(Adapter, Adaptee, AdapterFile, AdapteeFile);
        result = 31 * result + Arrays.hashCode(Adapting);
        return result;
    }
}
