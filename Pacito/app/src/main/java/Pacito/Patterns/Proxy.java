package Pacito.Patterns;

import lombok.Getter;

import java.util.Arrays;
import java.util.Objects;

@Getter
public class Proxy extends Pattern {
    public String Proxy;
    public String Interface;
    public String[] Reals;
    public String File;

    @Override
    public void cleanPaths(String base) {
        File = cleanPath(File, base);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Proxy proxy = (Proxy) o;
        return Objects.equals(Proxy, proxy.Proxy) && Objects.equals(Interface, proxy.Interface) && Arrays.equals(Reals, proxy.Reals) && Objects.equals(File, proxy.File);
    }

    @Override
    public int hashCode() {
        int result = Objects.hash(Proxy, Interface, File);
        result = 31 * result + Arrays.hashCode(Reals);
        return result;
    }
}
