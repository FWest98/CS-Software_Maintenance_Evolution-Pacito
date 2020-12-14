package Pacito.Patterns;

import lombok.Getter;

import java.util.Arrays;
import java.util.Objects;

@Getter
public class Strategy extends Pattern {
    public String Context;
    public String Strategy;
    public String[] StrategyImplementations;
    public String Delegator;

    public String ContextFile;
    public String StrategyFile;

    @Override
    public void cleanPaths(String base) {
        ContextFile = cleanPath(ContextFile, base);
        StrategyFile = cleanPath(StrategyFile, base);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Strategy strategy = (Strategy) o;
        return Objects.equals(Context, strategy.Context) && Objects.equals(Strategy, strategy.Strategy) && Arrays.equals(StrategyImplementations, strategy.StrategyImplementations) && Objects.equals(Delegator, strategy.Delegator) && Objects.equals(ContextFile, strategy.ContextFile) && Objects.equals(StrategyFile, strategy.StrategyFile);
    }

    @Override
    public int hashCode() {
        int result = Objects.hash(Context, Strategy, Delegator, ContextFile, StrategyFile);
        result = 31 * result + Arrays.hashCode(StrategyImplementations);
        return result;
    }
}
