package Pacito.Patterns;

import lombok.Getter;

import java.util.Arrays;
import java.util.Objects;

@Getter
public class State extends Pattern {
    public String Context;
    public String State;
    public String[] StateImplementations;

    public String Delegator;
    public String StateChanger;
    public String[] ChangeInvokers;

    public String ContextFile;
    public String StateFile;

    @Override
    public void cleanPaths(String base) {
        ContextFile = cleanPath(ContextFile, base);
        StateFile = cleanPath(StateFile, base);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        State state = (State) o;
        return Objects.equals(Context, state.Context) && Objects.equals(State, state.State) && Arrays.equals(StateImplementations, state.StateImplementations) && Objects.equals(Delegator, state.Delegator) && Objects.equals(StateChanger, state.StateChanger) && Arrays.equals(ChangeInvokers, state.ChangeInvokers) && Objects.equals(ContextFile, state.ContextFile) && Objects.equals(StateFile, state.StateFile);
    }

    @Override
    public int hashCode() {
        int result = Objects.hash(Context, State, Delegator, StateChanger, ContextFile, StateFile);
        result = 31 * result + Arrays.hashCode(StateImplementations);
        result = 31 * result + Arrays.hashCode(ChangeInvokers);
        return result;
    }
}
