package Pacito.Patterns;

import lombok.Getter;

import java.util.Objects;

@Getter
public class Template extends Pattern {
    public String Cls;
    public String Method;
    public String Source;
    public String File;

    @Override
    public void cleanPaths(String base) {
        File = cleanPath(File, base);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Template template = (Template) o;
        return Objects.equals(Cls, template.Cls) && Objects.equals(Method, template.Method) && Objects.equals(Source, template.Source) && Objects.equals(File, template.File);
    }

    @Override
    public int hashCode() {
        return Objects.hash(Cls, Method, Source, File);
    }
}
