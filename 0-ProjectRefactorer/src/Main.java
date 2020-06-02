import java.io.*;
import java.nio.Buffer;
import java.util.List;
import java.util.regex.Pattern;

public class Main {

    public static void main(String[] args) throws IOException {

        String projectName = "mina";

        FileReader listOfFilesFReader = new FileReader(projectName + "-files.list");
        BufferedReader listOfFilesBReader = new BufferedReader(listOfFilesFReader);

        List<File> projectFiles = null;
        List<String> genericTypes = null;

        String line;
        while ((line = listOfFilesBReader.readLine()) != null) {
            System.out.println(line);
            projectFiles.add(new File(line));
        }

        FileWriter newListOfFilesFW = new FileWriter(projectName+"-newfiles.list");
        BufferedWriter newListOfFilesBW = new BufferedWriter(newListOfFilesFW);


        for (File analysedFile : projectFiles) {

            FileReader tempFR = new FileReader(analysedFile);
            BufferedReader tempBR = new BufferedReader(tempFR);

            String newFilePath = analysedFile.getCanonicalPath().replace(".java","refactored.java");

            FileWriter newFileFW = new FileWriter(new File(newFilePath));
            BufferedWriter newFileBW = new BufferedWriter(newFileFW);

            newListOfFilesBW.write(newFilePath+"\n");

            String fileline;
            while ((fileline = tempBR.readLine()) != null) {

                for (String genericType : genericTypes) {
                    if (fileline.contains(genericType)){
                        fileline.replace(genericType, "Object");
                        newFileBW.write(fileline);
                    }
                }

                if (fileline.contains("@")){
                    String annotation = fileline.substring(fileline.indexOf("@"),fileline.indexOf(" ", fileline.indexOf("@")));
                    newFileBW.write(fileline.replace(annotation,""));
                }

                if (Pattern.matches("/[<>]+", fileline)){
                    String newGenericType = fileline.substring(fileline.indexOf("<")+1, fileline.indexOf(">"));
                    genericTypes.add(newGenericType);
                    fileline.replace(newGenericType, "Object");
                }
            }
            newFileBW.flush();
            newFileFW.flush();
            newFileBW.close();
            newFileFW.close();
        }

        newListOfFilesBW.flush();
        newListOfFilesBW.close();
        newListOfFilesFW.flush();
        newListOfFilesFW.close();

    }
}
