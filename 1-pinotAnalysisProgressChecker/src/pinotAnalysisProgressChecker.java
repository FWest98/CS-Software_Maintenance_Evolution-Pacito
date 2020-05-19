import java.io.*;

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
