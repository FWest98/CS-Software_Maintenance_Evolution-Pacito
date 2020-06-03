import java.io.*;
import java.util.ArrayList;
import java.util.List;

public class Main {

    //How to run java -jar pathToJar nameOfTheProject booleanToDelete
    //nameOfTheProject is very important, should be the same name as the gitHub folder
    //If anything is written in the second argument booleanToDelete, the original files are deleted

    public static void main(String[] args) throws IOException {

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
                    //Loop through all found GenericTypes so far
                    for (String genericType : genericTypes) {
                        if (fileline.contains(genericType)) {
                            fileline = fileline.replace(genericType, "Object");
                        }
                    }
                    if (fileline.contains("@")) {
                        if (fileline.indexOf(" ", fileline.indexOf("@")) != -1) {
                            String annotation = fileline.substring(fileline.indexOf("@"),
                                    fileline.indexOf(" ", fileline.indexOf("@")));
                            fileline = fileline.replace(annotation + " ", "");
                        }
                    }
                    if (fileline.contains("<") && fileline.contains(">")) {
                        if(fileline.indexOf("<") < fileline.indexOf(">")){
                            String regex = "<(?<=<)(.*?)(?=>)>";
                            String newGenericType = fileline.substring(fileline.indexOf("<") + 1, fileline.indexOf(">"));

                            String[] multipleGenericTypes = newGenericType.replaceAll("<|>",",")
                                    .split(",");

                            for (String genericType: multipleGenericTypes) {
                                System.out.println(genericType);
                                genericTypes.add(genericType);
                            }
                            fileline = fileline.replaceAll(regex, "");
                        }
                    }
                }
                newFileBW.write(fileline+"\r\n");
            }
            newFileBW.flush();
            newFileFW.flush();
        }
        newListOfFilesBW.flush();
        newListOfFilesFW.flush();

        if (args[1] != null){
            // This portion of the code is used to delete the new files in the end (if necessary)
            FileReader newListOfFilesFReader = new FileReader(projectName+"-files.list");
            BufferedReader newListOfFilesBReader = new BufferedReader(newListOfFilesFReader);
            String newFilesListLine;
            while ((newFilesListLine = newListOfFilesBReader.readLine()) != null) {
                File fileToDelete = new File(newFilesListLine);
                fileToDelete.deleteOnExit();
            }
        }
    }
}
