import java.io.*;

public class PomFileManipulator {

    public static void main(String[] args) throws IOException {

        if (args.length != 1){
            System.out.println("Error: No project name has been passed as an argument, this argument should be" +
                    "\"projectName\"");
            System.out.println("Proper Usage is: java -jar 4-PomFileManipulator.jar projectName");
            System.exit(0);
        }

        String analyzedProject = args[0];

        FileReader listOfPomsFR = new FileReader(analyzedProject + "-poms.list");
        BufferedReader listOfPomsBR = new BufferedReader(listOfPomsFR);

        String currentPomFile = listOfPomsBR.readLine();
        while (currentPomFile != null) {

            //Create copy of original POM File
            File copyOfPom = new File(currentPomFile.replace(".xml", "new.xml")
                    .replace("/",File.separator));

            //Create writer to the refactored POM
            FileWriter refactoredPomFW = new FileWriter(copyOfPom);
            BufferedWriter refactoredPomBW = new BufferedWriter(refactoredPomFW);

            //Create reader to original POM
            FileReader currentPomFileFR = new FileReader(new File(currentPomFile));
            BufferedReader currentPomFileBR = new BufferedReader(currentPomFileFR);
            String currentLine = currentPomFileBR.readLine();

            //Placeholders for objects inside <dependency block>
            String groupID;
            String artifactID;

            //In order to find the lines contained inside a <dependency> block, a boolean is used
            boolean checkIfInsideDependency = false;

            while (currentLine != null) {

                if (currentLine.substring(currentLine.indexOf("<")+1,currentLine.indexOf(">"))
                        .equals("dependency")){
                    StringBuilder dependencyBlock = new StringBuilder();
                    dependencyBlock.append(currentLine).append("\n");

                    //add groupID to StringBuilder
                    currentLine = currentPomFileBR.readLine();
                    if (currentLine.contains("tl-netty2")){
                        currentLine = currentLine.replace("tl-netty2", "net.gleamynode");
                        groupID = currentLine;
                    }
                    dependencyBlock.append(currentLine).append("\n");

                    //add artifactID to StringBuilder
                    currentLine = currentPomFileBR.readLine();
                    if (currentLine.contains("tl-netty2")){
                        currentLine = currentLine.replace("tl-netty2", "netty2");
                        artifactID = currentLine;
                    }
                    dependencyBlock.append(currentLine).append("\n");

                    //add possible fields (versions, exclusions, etc) to the StringBuilder.
                    currentLine = currentPomFileBR.readLine();
                    while(!currentLine.substring(currentLine.indexOf("<")+1,currentLine.indexOf(">"))
                            .equals("/dependency")){
                        dependencyBlock.append(currentLine).append("\n");
                        currentLine = currentPomFileBR.readLine();
                    }
                    //add last line without \n
                    dependencyBlock.append(currentLine);

                    String dependencyBlockString = dependencyBlock.toString();

                    if (!dependencyBlockString.contains(analyzedProject)){
                        refactoredPomBW.write(dependencyBlock.toString()+"\n");
                    }

                    //move one line after /dependencies
                    currentLine = currentPomFileBR.readLine();
                }


                if (currentLine.contains("repositories")){
                    //Skip all lines inside repositories block
                    while(!currentLine.contains("/repositories")){
                        currentLine = currentPomFileBR.readLine();
                    }
                    //Skip line to not write /repositories
                    currentLine = currentPomFileBR.readLine();
                }

                if (currentLine.contains("${pom.version}")){
                    currentLine = currentLine.replace("${pom.version}", "${project.version}");
                }

                refactoredPomBW.write(currentLine+"\n");
                currentLine = currentPomFileBR.readLine();
            }
            refactoredPomBW.flush();

            currentPomFileBR.close();
            currentPomFileFR.close();

            File pomToDelete = new File(currentPomFile);
            pomToDelete.renameTo(new File(currentPomFile.replace("pom","pom")));
            copyOfPom.renameTo(new File(currentPomFile.replace("pom","newpom")));

            currentPomFile = listOfPomsBR.readLine();
        }
        listOfPomsBR.close();
    }
}
