import java.io.*;
import java.util.ArrayList;
import java.util.List;

public class Main {

    //How to run java -jar pathToJar nameOfTheProject
    //nameOfTheProject is very important, should be the same name as the gitHub folder
    //If anything is written in the second argument booleanToDelete, the original files are deleted

    public static void main(String[] args) throws IOException {

        if (args.length == 0){
            System.out.println("Error: No project name (name of GitHub folder) has been passed as an argument");
            System.out.println("Proper Usage is: java -jar 0-ProjectRefactorer.jar nameOfGitHubFolder");
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
                            /*
                            String newGenericType = fileline.substring(fileline.indexOf("<") + 1, fileline.indexOf(">"));
                            // If it is a nested generic
                            if (newGenericType.contains("<")){
                                newGenericType = fileline.substring(fileline.indexOf("<") + 1, fileline.indexOf(">")+1);

                                // when there's a <HashMap<int,X>>, usually only the nested contains GenericTypes
                                newGenericType = newGenericType.substring(newGenericType.indexOf("<")+1,
                                        newGenericType.indexOf(">"));

                                //System.out.println("newGen2" + newGenericType);
                            }
                            String[] multipleGenericTypes = newGenericType.split(",");

                            for (String genericType: multipleGenericTypes) {
                                //Check if the genericType starts with upperCase

                                if (genericType != null && !genericType.isEmpty()){
                                    char[] genericTypeIntoChars = genericType.toCharArray();
                                    if (Character.isUpperCase(genericTypeIntoChars[0])){
                                        //System.out.println(genericType);
                                        if (genericType != "Object"){
                                            genericTypes.add(genericType);
                                        }
                                    }
                                }
                            }
                            */
                        }
                    }

                    // REMOVE GENERIC TYPES OF A LINE
                    String regexRemoveGenericTypes = "(?<=^|[^a-zA-Z0-9])([A-Z])(?=[^a-zA-Z0-9])";
                    fileline = fileline.replaceAll(regexRemoveGenericTypes, "Object");


                    //String regexForAnnotations = "^|(@[a-zA-Z]+)$(?=\\s)";
                    //fileline.replaceAll(regexForAnnotations,"");
                    //String regexAnnotations = "(?<=^|)(@[a-zA-Z])";

                    String anotherRegex = "(?<=.|^)(@[a-zA-Z].+?)(?=\\s|$)";
                    //String regexAnnotation = "@[a-zA-Z].+?(?=$)";
                    fileline.replaceAll(anotherRegex,"");

                    /*if (fileline.contains("@")) {
                        if (fileline.indexOf(" ", fileline.indexOf("@")) != -1) {
                            String annotation = fileline.substring(fileline.indexOf("@"),
                                    fileline.indexOf(" ", fileline.indexOf("@")));
                            fileline = fileline.replace(annotation + " ", "");
                        }
                        else{
                            fileline = "";
                        }
                    }

                     */

                    //Loop through all found GenericTypes so far
                    /*for (String genericType : genericTypes) {
                        if (fileline.contains(genericType)) {
                            fileline = fileline.replace(genericType, "Object");
                        }
                    }
                     */
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
        /*
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

         */
    }

}
