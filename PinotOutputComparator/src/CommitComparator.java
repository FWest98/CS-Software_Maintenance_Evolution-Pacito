import java.io.*;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;

public class CommitComparator {
    private static int counter = 1;

    public static void main(String[] args) throws IOException {

        File directory = new File(".\\results");
        directory.mkdir();

        File[] files = new File("./outputs-zookeeper").listFiles();

        sortFilesByNumericalOrder(files);

        patternComparator(files);
    }

    private static void patternComparator(File[] files) throws IOException {

        for (int i = 0; i < files.length-2; i++) {

            if (checkIfFilesAreValid(files[i], files[i+1])){
                ArrayList<String> firstPatterns = performAnalysis(files[i]);
                ArrayList<String> secondPatterns = performAnalysis(files[i+1]);

                pinotComparator(firstPatterns,secondPatterns);
            }
        }

    }

    private static boolean checkIfFilesAreValid(File file1, File file2) throws IOException {

        //true if one or both files contain
        boolean hasErrors = checkIfFileHasErrors(file1)||checkIfFileHasErrors(file2);

        if (hasErrors) {
            File errorAnalysis = new File(".\\results\\" + counter + "-Error.txt");
            errorAnalysis.createNewFile();
            counter++;
        }

        //means that the files are not empty and don't contain errors
        return (!checkIfAnyFileIsEmpty(file1,file2) && !hasErrors);

    }

    private static boolean checkIfAnyFileIsEmpty(File file1, File file2) throws IOException {
        if (file1.length() == 0 || file2.length() == 0){
            File noAnalysis = new File(".\\results\\" + counter + "-Blank.txt");
            noAnalysis.createNewFile();
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

    private static void pinotComparator(ArrayList<String> firstPatterns, ArrayList<String> secondPatterns) throws IOException {

        File resultFile;

        if (firstPatterns.equals(secondPatterns)){
            resultFile = new File(".\\results\\" + counter + "-No_differences.txt");
        }else{
            resultFile = new File(".\\results\\" + counter + "-VALID.txt");
        }
        counter++;

        FileWriter writer = new FileWriter(resultFile);
        BufferedWriter buffer = new BufferedWriter(writer);

        for (int i = 0; i < firstPatterns.size()-1; i++) {
            if (!(firstPatterns.get(i).equals(secondPatterns.get(i)))){
                buffer.write(firstPatterns.get(i) + "!=" + secondPatterns.get(i) + "\n");
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


