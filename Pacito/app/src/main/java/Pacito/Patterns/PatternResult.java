package Pacito.Patterns;

import lombok.Getter;
import org.eclipse.jgit.revwalk.RevCommit;

@Getter
public class PatternResult {
    public Pattern pattern;
    public String patternName;

    public String introCommitName;
    public int introCommitNumber;
    public String introCommitMessage;

    public String outroCommitName;
    public int outroCommitNumber;
    public String outroCommitMessage;

    public int livespan;

    public PatternResult(Pattern pattern, int introNum, RevCommit intro) {
        this.pattern = pattern;
        this.patternName = pattern.getClass().getSimpleName();

        this.introCommitName = intro.getName();
        this.introCommitNumber = introNum;
        this.introCommitMessage = intro.getFullMessage();

        this.updateOutroCommit(introNum, intro);
    }

    public void updateOutroCommit(int outroNum, RevCommit outro) {
        if(outroNum < this.outroCommitNumber) return;

        this.outroCommitName = outro.getName();
        this.outroCommitNumber = outroNum;
        this.outroCommitMessage = outro.getFullMessage();

        this.livespan = outroNum - this.introCommitNumber + 1;
    }
}
