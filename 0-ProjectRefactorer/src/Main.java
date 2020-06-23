import java.io.*;
import java.util.ArrayList;
import java.util.List;

public class Main {

    //How to run java -jar pathToJar projectName
    //projectName is very important, should be the same name as the gitHub folder

    public static void main(String[] args) throws IOException {

        if (args.length == 0){
            System.out.println("Error: No project name (name of GitHub folder) has been passed as an argument");
            System.out.println("Proper Usage is: java -jar 0-ProjectRefactorer.jar projectName");
            System.exit(0);
        }

        String projectName = args[0];
        FileReader listOfFilesFReader = new FileReader(projectName + "-files.list");
        BufferedReader listOfFilesBReader = new BufferedReader(listOfFilesFReader);
        List<File> projectFiles = new ArrayList<>();
        String line;

        while ((line = listOfFilesBReader.readLine()) != null) {
            projectFiles.add(new File(line));
        }
        FileWriter newListOfFilesFW = new FileWriter(projectName+"-newfiles.list");
        BufferedWriter newListOfFilesBW = new BufferedWriter(newListOfFilesFW);

        for (File analysedFile : projectFiles) {
            List<String> genericTypes = new ArrayList<>();
            FileReader tempFR = new FileReader(analysedFile);
            BufferedReader tempBR = new BufferedReader(tempFR);
            String newFilePath = analysedFile.getCanonicalPath().replace(".java","-refactored.java");
            FileWriter newFileFW = new FileWriter(new File(newFilePath));
            BufferedWriter newFileBW = new BufferedWriter(newFileFW);
            newListOfFilesBW.write(newFilePath+"\n");
            String fileline;

            while ((fileline = tempBR.readLine()) != null) {
                //Condition to not check comment lines
                if (!fileline.contains(" *")) {
                    if (fileline.contains("<") && fileline.contains(">")) {
                        if(fileline.indexOf("<") < fileline.indexOf(">")){
                            String regexRemoveDiamonds = "<(?<=<)(.*?)(?=>)>>|<(?<=<)(.*?)(?=>)>";
                            fileline = fileline.replaceAll(regexRemoveDiamonds, "");
                        }
                    }

                    // REMOVE GENERIC TYPES OF A LINE
                    String regexRemoveGenericTypes = "(?<=^|[^a-zA-Z0-9])([A-Z])(?=[^a-zA-Z0-9])";
                    fileline = fileline.replaceAll(regexRemoveGenericTypes, "Object");

                    // REMOVE ANNOTATIONS OF A LINE
                    String regexForAnnotations = "(?<=.|^)(@[a-zA-Z].+?)(?=' '|$)";
                    fileline = fileline.replaceAll(regexForAnnotations,"");

                }
                newFileBW.write(fileline+"\r\n");
            }
            newFileBW.flush();
            newFileFW.flush();
            newFileBW.close();
            newFileFW.close();

        }
        newListOfFilesBW.flush();
        newListOfFilesFW.flush();
        newListOfFilesBW.close();
        newListOfFilesFW.close();
    }

}
