import java.io.*;

/*
This class aims to obtain an overview of the output from pinot
This can have 3 states:
    valid analysis (output was produced);
    error analysis (output produced an error);
    blank analysis (an error occurred so no output was produced)

@param This class receives as an arguments a folder-name in the variable projectName,
which needs to be changed when running the program for different projects.

@input This jar is supposed to be ran inside a folder containing the three files which were outputted from
the script "blank-error-validChecker.sh".
The three files are named *projectName*-valid.list, *projectName*-blank.list and
*projectName*-error.list

@outputs The output is a single csv file containing the full analysis, with a list of the commit and
corresponding analysis.
This help in understanding how the outputs vary with the evolution of the project

@author Filipe Capela S4040112
 */
public class pinotAnalysisProgressChecker {

    public static void main(String[] args) throws IOException {

        if (args.length == 0){
            System.out.println("Error: No project name has been passed as an argument, this argument should be the name" +
                    "before the -blanks.list, -valid.list, -error.list files, for example, mina");
            System.out.println("Proper Usage is: java -jar pinotAnalysisProgressChecker.jar projectName");
            System.exit(0);
        }

        //name of the folder where the project intended to be analyzed is
        String projectName = args[0];

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
