package Pacito.Patterns;

import lombok.Getter;

import java.util.Arrays;
import java.util.Objects;

@Getter
public class Observer extends Pattern {
    public String Iterator;
    public String ListenerType;
    public String Notify;
    public String Update;
    public String[] Subjects;
    public String File;

    @Override
    public void cleanPaths(String base) {
        File = cleanPath(File, base);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Observer observer = (Observer) o;
        return Objects.equals(Iterator, observer.Iterator) && Objects.equals(ListenerType, observer.ListenerType) && Objects.equals(Notify, observer.Notify) && Objects.equals(Update, observer.Update) && Arrays.equals(Subjects, observer.Subjects) && Objects.equals(File, observer.File);
    }

    @Override
    public int hashCode() {
        int result = Objects.hash(Iterator, ListenerType, Notify, Update, File);
        result = 31 * result + Arrays.hashCode(Subjects);
        return result;
    }
}
