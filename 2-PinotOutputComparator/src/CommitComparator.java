import java.io.*;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;

public class CommitComparator {
    private static int counter = 1;

    //Change according to the name of the folder where the pinot outputs are available
    private static String analyzedProject = "outputs-flink";

    public static void main(String[] args) throws IOException {

        //Create directory to store results if it does not exist already
        File directory = new File(".\\results-"+analyzedProject);
        directory.mkdir();

        //Store the files from pinot outputs to an array
        File[] files = new File(".\\" + analyzedProject).listFiles();

        //Sort files by numerical order, since by default they are sorted
        //Alphabetically
        sortFilesByNumericalOrder(files);

        //Where the comparison is made for all files
        patternComparator(files);

        System.out.println("Processing finished!");
    }

    private static void patternComparator(File[] files) throws IOException {

        for (int i = 0; i < files.length-2; i++) {

            if (checkIfFilesAreValid(files[i], files[i+1])){
                ArrayList<String> firstPatterns = performAnalysis(files[i]);
                ArrayList<String> secondPatterns = performAnalysis(files[i+1]);

                pinotComparator(firstPatterns,secondPatterns, files[i+1]);
            }
        }

    }

    private static boolean checkIfFilesAreValid(File file1, File file2) throws IOException {

        //true if one or both files contain
        boolean hasErrors = checkIfFileHasErrors(file1)||checkIfFileHasErrors(file2);

        if (hasErrors) {
            //File errorAnalysis = new File(".\\results-" + analyzedProject + "\\" + "Error-" + counter + ".txt");
            //errorAnalysis.createNewFile();
            counter++;
        }

        //means that the files are not empty and don't contain errors
        return (!checkIfAnyFileIsEmpty(file1,file2) && !hasErrors);

    }

    private static boolean checkIfAnyFileIsEmpty(File file1, File file2) throws IOException {
        if (file1.length() == 0 || file2.length() == 0){
            //File noAnalysis = new File(".\\results-"+analyzedProject + "\\Blank-" + counter + ".txt");
            //noAnalysis.createNewFile();
            counter++;
            return true;
        }
        //means that one or two files are not empty
        return false;
    }

    private static boolean checkIfFileHasErrors(File file) {

        BufferedReader reader;
        try {
            reader = new BufferedReader(new FileReader(file));
            String line = reader.readLine();
            while (line != null) {

                if(line.contains("pinot")||line.contains("jikes")){
                    return true;
                }
                line = reader.readLine();
            }
            reader.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        //means that one or two files are not empty
        return false;
    }

    private static ArrayList<String> performAnalysis(File pinotOutput) {

        ArrayList<String> patterns = new ArrayList<String>();
        boolean reached = false;

        BufferedReader reader;
        try {
            reader = new BufferedReader(new FileReader(pinotOutput));
            String line = reader.readLine();
            while (line != null) {

                if(line.isEmpty()){
                    reached=false;
                }
                if (reached){
                    if(!line.equals("==============================") &&
                            !line.equals("------------------------------") &&
                            !line.equals("Structural Patterns") &&
                            !line.equals("Behavioral Patterns")
                    ){
                        line = line.replaceAll("[ ]{2,}","-");
                        patterns.add(line);
                        // read next line
                    }
                }
                if (line.equals("Creational Patterns")){
                    reached=true;
                }
                line = reader.readLine();
            }
            reader.close();
        } catch (IOException e) {
            e.printStackTrace();
        }

        return patterns;
    }

    private static void pinotComparator(ArrayList<String> firstPatterns, ArrayList<String> secondPatterns, File second) throws IOException {

        //-4 since I need to remove the .txt extension
        String commitHash = second.getName().substring(second.getName().lastIndexOf("-")+1, second.getName().length()-4);

        File resultFile;

        if (firstPatterns.equals(secondPatterns)){
            resultFile = new File(".\\results-" + analyzedProject + "\\" + "No_differences-" + counter + ".txt");
        }else{
            resultFile = new File(".\\results-" + analyzedProject + "\\" + "VALID-" + counter + ".txt");
        }
        counter++;

        FileWriter writer = new FileWriter(resultFile);
        BufferedWriter buffer = new BufferedWriter(writer);

        buffer.write("Pattern changes caused by commit: " +  commitHash + "\n\n");

        for (int i = 0; i < firstPatterns.size()-1; i++) {
            if (!(firstPatterns.get(i).equals(secondPatterns.get(i)))){
                buffer.write("From: " + firstPatterns.get(i) + "\nTo:   " + secondPatterns.get(i) + "\n\n");
            }
        }
        buffer.close();
    }

    ///////////////////////
    // AUXILIARY METHODS //
    ///////////////////////

    private static void sortFilesByNumericalOrder(File[] files) {
        Arrays.sort(files, new Comparator<File>() {
            public int compare(File o1, File o2) {
                int n1 = extractNumber(o1.getName());
                int n2 = extractNumber(o2.getName());
                return n1 - n2;
            }

            private int extractNumber(String name) {
                int i = 0;
                try {
                    String number = name.substring(0, name.indexOf('-')-1);
                    i = Integer.parseInt(number);
                } catch(Exception e) {
                    i = 0; // if filename does not match the format
                    // then default to 0
                }
                return i;
            }
        });
    }

    public static void showFiles(File[] files) {
        for (File file : files) {
            if (file.isDirectory()) {
                System.out.println("Directory: " + file.getName());
            } else {
                System.out.println("File: " + file.getName());
            }
        }
    }
}


