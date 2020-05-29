import java.io.*;

/*
This class aims to obtain an overview of the output from pinot
This can have 3 states:
    valid analysis (output was produced);
    error analysis (output produced an error);
    blank analysis (an error occurred so no output was produced)

This class receives as an arguments a folder-name in the variable projectName,
which needs to be changed when running the program for different projects.

The input folder needs to be in the same root directory as the project, in this case
something like ~/Internship_RuG_2020/1-pinotAnalysisProgressChecker/*hadoop-hdfs-blanks.list*


This input is supposed to be 3 different files, which are produced when running
the script "blank-error-validChecker.sh" on the folder of the pinot analysis output
The three files are named *projectName*-valid.list, *projectName*-blank.list and
*projectName*-error.list

The output is a single csv file containing the full analysis, with a list of the commit and
corresponding analysis.
This help in understanding how the outputs vary with the evolution of the project

@author Filipe Capela S4040112
 */
public class pinotAnalysisProgressChecker {

    private static String projectName = "hadoop-hdfs";

    public static void main(String[] args) throws IOException {

        FileWriter finalProductFW = new FileWriter(projectName+"-finalAnalysis.csv");
        BufferedWriter finalProductBW = new BufferedWriter(finalProductFW);

        /////////////////////////////////////////////////////////////
        // BLANK
        /////////////////////////////////////////////////////////////

        FileReader blankFR = new FileReader(projectName+"-blanks.list");
        BufferedReader blankBR = new BufferedReader(blankFR);

        String nextLineBlank = blankBR.readLine();

        while(nextLineBlank != null){

            finalProductBW.write(nextLineBlank.substring(0,nextLineBlank.indexOf("-"))+",1,blank\n");

            nextLineBlank = blankBR.readLine();
        }

        blankBR.close();
        blankFR.close();

        /////////////////////////////////////////////////////////////
        // ERROR
        /////////////////////////////////////////////////////////////

        FileReader emptyFR = new FileReader(projectName+"-errors.list");
        BufferedReader emptyBR = new BufferedReader(emptyFR);

        String nextLineEmpty = emptyBR.readLine();

        while(nextLineEmpty != null){

            finalProductBW.write(nextLineEmpty.substring(0,nextLineEmpty.indexOf("-"))+",2,error\n");

            nextLineEmpty = emptyBR.readLine();
        }

        emptyBR.close();
        emptyFR.close();

        /////////////////////////////////////////////////////////////
        // VALID
        /////////////////////////////////////////////////////////////

        FileReader validFR = new FileReader(projectName+"-valid.list");
        BufferedReader validBR = new BufferedReader(validFR);

        String nextLineValid = validBR.readLine();

        while(nextLineValid != null){

            finalProductBW.write(nextLineValid.substring(0,nextLineValid.indexOf("-"))+",3,valid\n");

            nextLineValid = validBR.readLine();
        }

        validBR.close();
        validFR.close();


        finalProductBW.flush();
        finalProductBW.close();
    }

}
