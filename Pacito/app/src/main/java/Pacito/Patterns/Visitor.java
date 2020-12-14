package Pacito.Patterns;

import lombok.Getter;

import java.util.Arrays;
import java.util.Objects;

@Getter
public class Visitor extends Pattern {
    public String Visitor;
    public String Visitee;
    public String Accept;
    public String Visit;

    public boolean IsThisExposed; // false or otherwise exposed is filled
    public String Exposed;

    public String File;

    public String AbstractVisitee;
    public String[] VisiteeImplementations;

    @Override
    public void cleanPaths(String base) {
        File = cleanPath(File, base);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Visitor visitor = (Visitor) o;
        return IsThisExposed == visitor.IsThisExposed && Objects.equals(Visitor, visitor.Visitor) && Objects.equals(Visitee, visitor.Visitee) && Objects.equals(Accept, visitor.Accept) && Objects.equals(Visit, visitor.Visit) && Objects.equals(Exposed, visitor.Exposed) && Objects.equals(File, visitor.File) && Objects.equals(AbstractVisitee, visitor.AbstractVisitee) && Arrays.equals(VisiteeImplementations, visitor.VisiteeImplementations);
    }

    @Override
    public int hashCode() {
        int result = Objects.hash(Visitor, Visitee, Accept, Visit, IsThisExposed, Exposed, File, AbstractVisitee);
        result = 31 * result + Arrays.hashCode(VisiteeImplementations);
        return result;
    }
}
