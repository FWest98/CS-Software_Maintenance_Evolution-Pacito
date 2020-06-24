import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;

/*
Function that fetches information from JIRA's issue tracking
system, to obtain information regarding several issue tags
found on commit messages.

@param the analyzedProject String needs to be changed to the name
of the folder containing the issueTags from a certain project.
VERY IMPORTANT: in the obtainIssueKey function, you will need to add 
an entry to projectSpecificTags with the name of the tag used by the JIRA
repository.

@input a folder containing files holding the result from script
issueTagExtractor, which contains issue tags from JIRA, this script needs to be run.

@output a csv file containing all the information relevant for a
certain project, containing information from the commit, the issue,
the pattern changes, developers, etc.
Additionally, for each commit containing an issue tag, a file is created with
the same information as the csv, but with the addition of the comments extracted
from JIRA.
Lastly, a file is created for each existing issue there exists in this projects
JIRA repository, all these files are written onto AllFiles-projectName/
 */

public class JiraXMLIssueRequester {

    //Change according to the name of the folder where the issueTags folder is located
    private static String analyzedProject;

    public static void main(String[] args) throws IOException {

        if (args.length != 2){
            System.out.println("Error: No project name has been passed as an argument, this argument should be" +
                    "\"projectName\"-issueTags numberOfIssues \n For more information regarding the number of issues" +
                    "a project contains, go to " +
                    "https://issues.apache.org/jira/projects/SPECIFICPROJECTISSUETAG/issues/filter=allissues and write " +
                    "the number of the most recent issue");
            System.out.println("Proper Usage is: java -jar JiraIssueParser.jar projectName-issueTags 1223");
            System.exit(0);
        }

        analyzedProject = args[0];

        File[] issueTagsFiles = createDirectoryAndFileArray();

        File finalCSVFile = new File("finalResults-" + analyzedProject.substring(0,analyzedProject.indexOf("-")) + File.separator +
                analyzedProject.substring(0, analyzedProject.indexOf("-")) + "-finalResults-CSV.csv");

        FileWriter fw = new FileWriter(finalCSVFile.getPath());
        BufferedWriter bw = new BufferedWriter(fw);
        PrintWriter pw = new PrintWriter(bw);
        pw.println("Project" + "," + "CommitID" + "," + "Developer" + "," + "Title" + "," + "Summary" + ","
                + "IssueKey" + "," + "IssueType" + "," + "CreatedDate" + "," + "latestDateBetweenUpdatedAndResolved"
                + "," + "Abstract Factory" + "," + "Factory Method" + "," + "Singleton" + "," + "Adapter" + ","
                + "Bridge" + "," + "Composite" + "," + "Decorator" + "," + "Facade" + "," + "Flyweight" + "," + "Proxy"
                + "," + "Chain of Responsibility" + "," + "Mediator" + "," + "Observer" + "," + "State"  + ","
                + "Strategy"  + "," + "Template Method" + "," + "Visitor");
        pw.flush();
        pw.close();

        String projectIssueKey = null;

        /*
        This portion is responsible for checking whether a commit
        contains an issue tag on the commit message
         */
        for (File analyzedFile : issueTagsFiles) {

            String issueKey = obtainIssueKey(analyzedFile);

            if (issueKey != null) {
                get_response(issueKey, analyzedFile, finalCSVFile);
                if (projectIssueKey == null){
                    projectIssueKey = issueKey;
                    projectIssueKey.substring(0,projectIssueKey.indexOf("-"));
                }
            }
            else{
                addNonIssueEntryToCSV(analyzedFile, finalCSVFile);
            }

        }

        //Create directory to store results if it does not exist already
        File directory = new File("AllIssues-" + analyzedProject.substring(0,analyzedProject.indexOf("-")));
        directory.mkdir();


        // This portion of code handles the loop that checks for key-words in all the issues in the repository and writes
        // these to individual files
        int totalNumberOfIssues = Integer.parseInt(args[1]);
        if (projectIssueKey != null){
            for (int counter = 1; counter <= totalNumberOfIssues; counter++){
                String issueKey = projectIssueKey.substring(0,projectIssueKey.indexOf("-")) + "-" + String.valueOf(counter);
                scanIssue(issueKey);
            }
        }
    }

    /*
    This function is in charge of printing the information regarding pattern changes in commits that do not contain
    issue keys onto the final csv file.
     */
    private static void addNonIssueEntryToCSV(File analyzedFile, File finalCSVFile) throws IOException {

        FileWriter csvFileWriter = new FileWriter(finalCSVFile.getPath(), true);
        BufferedWriter csvBufferedWriter = new BufferedWriter(csvFileWriter);
        PrintWriter csvPrintWriter = new PrintWriter(csvBufferedWriter);

        File issueTagsFile = new File(analyzedProject + File.separator + analyzedFile.getName());
        FileReader fileReaderForCommitID = new FileReader(issueTagsFile.getPath());
        BufferedReader bufferedReaderForCommitID = new BufferedReader(fileReaderForCommitID);
        String firstLine = bufferedReaderForCommitID.readLine();
        String before;
        String after;
        //StringBuilder detectedPatterns = new StringBuilder();
        String detectedPattern;
        String bufferedLine = bufferedReaderForCommitID.readLine();
        boolean reached = false;

        List<String> patternChangesArray = new ArrayList<String>(Collections.nCopies(17, ""));

        while (bufferedLine != null && reached == false) {

            //In the issueTags file, there's a line with ='s that marks the end of the changes in patterns
            if (bufferedLine.contains("=")) {
                reached = true;
            } else {
                if (!bufferedLine.isEmpty()) {
                    before = bufferedLine.substring(bufferedLine.lastIndexOf(" ") + 1);
                    after = bufferedReaderForCommitID.readLine();
                    after = after.substring(after.lastIndexOf(" ") + 1);
                    detectedPattern = before.substring(0, before.indexOf("-"));

                    String numberBeforeString = before.substring(before.indexOf("-")+1);
                    int numberBefore = Integer.parseInt(numberBeforeString);
                    String numberAfterString = after.substring(after.indexOf("-")+1);
                    int numberAfter = Integer.parseInt(numberAfterString);

                    int patternDifferences = numberBefore - numberAfter;


                    if (patternDifferences > 0) {
                        switch(detectedPattern) {
                            case "Abstract Factory":
                                patternChangesArray.add(0, patternDifferences + " REMOVED");
                                break;
                            case "Factory Method":
                                patternChangesArray.add(1, patternDifferences + " REMOVED");
                                break;
                            case "Singleton":
                                patternChangesArray.add(2, patternDifferences + " REMOVED");
                                break;
                            case "Adapter":
                                patternChangesArray.add(3, patternDifferences + " REMOVED");
                                break;
                            case "Bridge":
                                patternChangesArray.add(4, patternDifferences + " REMOVED");
                                break;
                            case "Composite":
                                patternChangesArray.add(5, patternDifferences + " REMOVED");
                                break;
                            case "Decorator":
                                patternChangesArray.add(6, patternDifferences + " REMOVED");
                                break;
                            case "Facade":
                                patternChangesArray.add(7, patternDifferences + " REMOVED");
                                break;
                            case "Flyweight":
                                patternChangesArray.add(8, patternDifferences + " REMOVED");
                                break;
                            case "Proxy":
                                patternChangesArray.add(9, patternDifferences + " REMOVED");
                                break;
                            case "Chain of Responsibility":
                                patternChangesArray.add(10, patternDifferences + " REMOVED");
                                break;
                            case "Mediator":
                                patternChangesArray.add(11, patternDifferences + " REMOVED");
                                break;
                            case "Observer":
                                patternChangesArray.add(12, patternDifferences + " REMOVED");
                                break;
                            case "State":
                                patternChangesArray.add(13, patternDifferences + " REMOVED");
                                break;
                            case "Strategy":
                                patternChangesArray.add(14, patternDifferences + " REMOVED");
                                break;
                            case "Template Method":
                                patternChangesArray.add(15, patternDifferences + " REMOVED");
                                break;
                            case "Visitor":
                                patternChangesArray.add(16, patternDifferences + " REMOVED");
                                break;
                        }
                        //detectedPatterns.append(patternDifferences + " instances of the "
                        //+ detectedPattern + " Pattern were REMOVED,");
                    } else {
                        switch(detectedPattern) {
                            case "Abstract Factory":
                                patternChangesArray.add(0, Math.abs(patternDifferences) + " ADDED");
                                break;
                            case "Factory Method":
                                patternChangesArray.add(1, Math.abs(patternDifferences) + " ADDED");
                                break;
                            case "Singleton":
                                patternChangesArray.add(2, Math.abs(patternDifferences) + " ADDED");
                                break;
                            case "Adapter":
                                patternChangesArray.add(3, Math.abs(patternDifferences) + " ADDED");
                                break;
                            case "Bridge":
                                patternChangesArray.add(4, Math.abs(patternDifferences) + " ADDED");
                                break;
                            case "Composite":
                                patternChangesArray.add(5, Math.abs(patternDifferences) + " ADDED");
                                break;
                            case "Decorator":
                                patternChangesArray.add(6, Math.abs(patternDifferences) + " ADDED");
                                break;
                            case "Facade":
                                patternChangesArray.add(7, Math.abs(patternDifferences) + " ADDED");
                                break;
                            case "Flyweight":
                                patternChangesArray.add(8, Math.abs(patternDifferences) + " ADDED");
                                break;
                            case "Proxy":
                                patternChangesArray.add(9, Math.abs(patternDifferences) + " ADDED");
                                break;
                            case "Chain of Responsibility":
                                patternChangesArray.add(10, Math.abs(patternDifferences) + " ADDED");
                                break;
                            case "Mediator":
                                patternChangesArray.add(11, Math.abs(patternDifferences) + " ADDED");
                                break;
                            case "Observer":
                                patternChangesArray.add(12, Math.abs(patternDifferences) + " ADDED");
                                break;
                            case "State":
                                patternChangesArray.add(13, Math.abs(patternDifferences) + " ADDED");
                                break;
                            case "Strategy":
                                patternChangesArray.add(14, Math.abs(patternDifferences) + " ADDED");
                                break;
                            case "Template Method":
                                patternChangesArray.add(15, Math.abs(patternDifferences) + " ADDED");
                                break;
                            case "Visitor":
                                patternChangesArray.add(16, Math.abs(patternDifferences) + " ADDED");
                                break;
                        }
                        //detectedPatterns.append(Math.abs(patternDifferences) + " instances of the "
                        //        + detectedPattern + " Pattern were ADDED,");
                    }
                }
            }
            bufferedLine = bufferedReaderForCommitID.readLine();
        }

        String project = analyzedProject.substring(0,analyzedProject.indexOf("-")).toUpperCase();
        String commitID = firstLine.substring(firstLine.lastIndexOf(" ")+1);

        String patternChanges = String.join(",", patternChangesArray);

        // In this portion of the code we need to extract the commit message to compare to the issues description from
        // JIRA






        /////////////////////////////
        ///// WRITE TO CSV FILE /////
        /////////////////////////////

        csvPrintWriter.println(project + "," + commitID + "," + "" + "," + "" + "," + "" + ","
                + "" + "," + "" + "," + "" + "," + "" + "," + patternChanges);
        csvPrintWriter.flush();
        csvPrintWriter.close();
    }

    /*
    Function responsible for creating directory to store final results
    and creating an array of files from the input folder
     */
    private static File[] createDirectoryAndFileArray() {

        //Create directory to store results if it does not exist already
        File directory = new File("finalResults-" + analyzedProject.substring(0,analyzedProject.indexOf("-")));
        directory.mkdir();

        //Store the files from issueTags outputs to an array
        File[] files = new File(analyzedProject).listFiles();

        return files;
    }


    /*
    Function that scans a file which holds a commit message and
    checks if the projects issue tag is present.

    NOTE: You will need to add the issue tag from the project you are analyzing to the projectSpecificTags array
     */
    public static String obtainIssueKey(File analyzedFile) {

        String issueKey = null;

        String[] projectSpecificTags = {"ZOOKEEPER-", "DIRMINA-", "HDFS-", "CASSANDRA-"};

        BufferedReader reader;
        try {
            reader = new BufferedReader(new FileReader(analyzedFile));
            String line = reader.readLine();
            while (line != null) {

                if (stringContainsItemFromList(line,projectSpecificTags)){
                    String[] segments = line.split(" |\\.|:|\\(|\\)");

                    for (String segment : segments) {
                        if(stringContainsItemFromList(segment,projectSpecificTags)){
                            issueKey = segment;
                        }
                    }

                }

                line = reader.readLine();
            }
            reader.close();
        } catch (IOException e) {
            e.printStackTrace();
        }

        return issueKey;
    }

    /*
    Function to check if a String contains any word from an array of Strings
    https://stackoverflow.com/a/8995988
     */
    public static boolean stringContainsItemFromList(String inputStr, String[] items) {
        return Arrays.stream(items).parallel().anyMatch(inputStr::contains);
    }

    /*
    Function responsible for contacting the JIRA's issue URL, and obtaining
    the XML information, parsing it so that it is possible to write the
    information for each issue tag obtained.
     */
    public static void get_response(String issueKey, File outputFile, File csvFile) {
        try {
            String format = "xml";
            String url = "https://issues.apache.org/jira/si/jira.issueviews:issue-" + format + "/" + issueKey + "/" +
                    issueKey + ".xml";

            System.out.println(url);
            URL obj = new URL(url);
            HttpURLConnection con = (HttpURLConnection) obj.openConnection();
            int responseCode = con.getResponseCode();
            System.out.println("Response Code : " + responseCode);
            BufferedReader in = new BufferedReader(
                    new InputStreamReader(con.getInputStream()));
            String inputLine;
            StringBuffer response = new StringBuffer();
            while ((inputLine = in.readLine()) != null) {
                response.append(inputLine);
            }
            in.close();
            //print in String
            //System.out.println(response.toString());
            Document doc = DocumentBuilderFactory.newInstance().newDocumentBuilder()
                    .parse(new InputSource(new StringReader(response.toString())));
            NodeList errNodes = doc.getElementsByTagName("item");

            FileWriter csvFileWriter = new FileWriter(csvFile.getPath(), true);
            BufferedWriter csvBufferedWriter = new BufferedWriter(csvFileWriter);
            PrintWriter csvPrintWriter = new PrintWriter(csvBufferedWriter);

            FileWriter writePatternMentionsFW = new FileWriter("patternsMentionedInIssues.txt", true);
            BufferedWriter writePatternMentionsBW = new BufferedWriter(writePatternMentionsFW);

            if (errNodes.getLength() > 0) {
                Element err = (Element) errNodes.item(0);

                File issueTagsFile = new File(analyzedProject + File.separator + outputFile.getName());
                FileReader fileReaderForCommitID = new FileReader(issueTagsFile.getPath());
                BufferedReader bufferedReaderForCommitID = new BufferedReader(fileReaderForCommitID);
                String firstLine = bufferedReaderForCommitID.readLine();
                String before;
                String after;
                //StringBuilder detectedPatterns = new StringBuilder();
                String detectedPattern;
                String bufferedLine = bufferedReaderForCommitID.readLine();
                boolean reached = false;

                List<String> patternChangesArray = new ArrayList<String>(Collections.nCopies(17, ""));

                while (bufferedLine != null && reached == false) {

                    //In the issueTags file, there's a line with ='s that marks the end of the changes in patterns
                    if (bufferedLine.contains("=")) {
                        reached = true;
                    } else {
                        if (!bufferedLine.isEmpty()) {
                            before = bufferedLine.substring(bufferedLine.lastIndexOf(" ") + 1);
                            after = bufferedReaderForCommitID.readLine();
                            after = after.substring(after.lastIndexOf(" ") + 1);
                            detectedPattern = before.substring(0, before.indexOf("-"));

                            String numberBeforeString = before.substring(before.indexOf("-")+1);
                            int numberBefore = Integer.parseInt(numberBeforeString);
                            String numberAfterString = after.substring(after.indexOf("-")+1);
                            int numberAfter = Integer.parseInt(numberAfterString);

                            int patternDifferences = numberBefore - numberAfter;


                            if (patternDifferences > 0) {
                                switch(detectedPattern) {
                                    case "Abstract Factory":
                                        patternChangesArray.add(0, patternDifferences + " REMOVED");
                                        break;
                                    case "Factory Method":
                                        patternChangesArray.add(1, patternDifferences + " REMOVED");
                                        break;
                                    case "Singleton":
                                        patternChangesArray.add(2, patternDifferences + " REMOVED");
                                        break;
                                    case "Adapter":
                                        patternChangesArray.add(3, patternDifferences + " REMOVED");
                                        break;
                                    case "Bridge":
                                        patternChangesArray.add(4, patternDifferences + " REMOVED");
                                        break;
                                    case "Composite":
                                        patternChangesArray.add(5, patternDifferences + " REMOVED");
                                        break;
                                    case "Decorator":
                                        patternChangesArray.add(6, patternDifferences + " REMOVED");
                                        break;
                                    case "Facade":
                                        patternChangesArray.add(7, patternDifferences + " REMOVED");
                                        break;
                                    case "Flyweight":
                                        patternChangesArray.add(8, patternDifferences + " REMOVED");
                                        break;
                                    case "Proxy":
                                        patternChangesArray.add(9, patternDifferences + " REMOVED");
                                        break;
                                    case "Chain of Responsibility":
                                        patternChangesArray.add(10, patternDifferences + " REMOVED");
                                        break;
                                    case "Mediator":
                                        patternChangesArray.add(11, patternDifferences + " REMOVED");
                                        break;
                                    case "Observer":
                                        patternChangesArray.add(12, patternDifferences + " REMOVED");
                                        break;
                                    case "State":
                                        patternChangesArray.add(13, patternDifferences + " REMOVED");
                                        break;
                                    case "Strategy":
                                        patternChangesArray.add(14, patternDifferences + " REMOVED");
                                        break;
                                    case "Template Method":
                                        patternChangesArray.add(15, patternDifferences + " REMOVED");
                                        break;
                                    case "Visitor":
                                        patternChangesArray.add(16, patternDifferences + " REMOVED");
                                        break;
                                }
                                //detectedPatterns.append(patternDifferences + " instances of the "
                                        //+ detectedPattern + " Pattern were REMOVED,");
                            } else {
                                switch(detectedPattern) {
                                    case "Abstract Factory":
                                        patternChangesArray.add(0, Math.abs(patternDifferences) + " ADDED");
                                        break;
                                    case "Factory Method":
                                        patternChangesArray.add(1, Math.abs(patternDifferences) + " ADDED");
                                        break;
                                    case "Singleton":
                                        patternChangesArray.add(2, Math.abs(patternDifferences) + " ADDED");
                                        break;
                                    case "Adapter":
                                        patternChangesArray.add(3, Math.abs(patternDifferences) + " ADDED");
                                        break;
                                    case "Bridge":
                                        patternChangesArray.add(4, Math.abs(patternDifferences) + " ADDED");
                                        break;
                                    case "Composite":
                                        patternChangesArray.add(5, Math.abs(patternDifferences) + " ADDED");
                                        break;
                                    case "Decorator":
                                        patternChangesArray.add(6, Math.abs(patternDifferences) + " ADDED");
                                        break;
                                    case "Facade":
                                        patternChangesArray.add(7, Math.abs(patternDifferences) + " ADDED");
                                        break;
                                    case "Flyweight":
                                        patternChangesArray.add(8, Math.abs(patternDifferences) + " ADDED");
                                        break;
                                    case "Proxy":
                                        patternChangesArray.add(9, Math.abs(patternDifferences) + " ADDED");
                                        break;
                                    case "Chain of Responsibility":
                                        patternChangesArray.add(10, Math.abs(patternDifferences) + " ADDED");
                                        break;
                                    case "Mediator":
                                        patternChangesArray.add(11, Math.abs(patternDifferences) + " ADDED");
                                        break;
                                    case "Observer":
                                        patternChangesArray.add(12, Math.abs(patternDifferences) + " ADDED");
                                        break;
                                    case "State":
                                        patternChangesArray.add(13, Math.abs(patternDifferences) + " ADDED");
                                        break;
                                    case "Strategy":
                                        patternChangesArray.add(14, Math.abs(patternDifferences) + " ADDED");
                                        break;
                                    case "Template Method":
                                        patternChangesArray.add(15, Math.abs(patternDifferences) + " ADDED");
                                        break;
                                    case "Visitor":
                                        patternChangesArray.add(16, Math.abs(patternDifferences) + " ADDED");
                                        break;
                                }
                                //detectedPatterns.append(Math.abs(patternDifferences) + " instances of the "
                                //        + detectedPattern + " Pattern were ADDED,");
                            }
                        }
                    }
                    bufferedLine = bufferedReaderForCommitID.readLine();
                }

                String project = err.getElementsByTagName("project").item(0).getTextContent()
                        .replaceAll(",|;", "-");
                String commitID = firstLine.substring(firstLine.lastIndexOf(" ")+1);
                String developer = err.getElementsByTagName("assignee").item(0).getTextContent()
                        .replaceAll(",|;", "-");
                String title = err.getElementsByTagName("title").item(0).getTextContent()
                        .replaceAll(",|;", "-");
                String summary = err.getElementsByTagName("summary").item(0).getTextContent()
                        .replaceAll(",|;", "-");
                String parsedIssueKey = err.getElementsByTagName("key").item(0).getTextContent()
                        .replaceAll(",|;", "-");
                String issueType = err.getElementsByTagName("type").item(0).getTextContent()
                        .replaceAll(",|;", "-");
                String createdDate = err.getElementsByTagName("created").item(0).getTextContent()
                        .replaceAll(",|;", "-");
                String resolvedDate = err.getElementsByTagName("resolved").item(0).getTextContent()
                        .replaceAll(",|;", "-");
                String updatedDate = err.getElementsByTagName("updated").item(0).getTextContent()
                        .replaceAll(",|;", "-");
                //String patternChanges = detectedPatterns.toString();
                String patternChanges = String.join(",", patternChangesArray);

                /////////////////////////////
                ///// WRITE TO CSV FILE /////
                /////////////////////////////

                String latestDateBetweenUpdatedAndResolved;
                DateTimeFormatter jiraDateFormatter = DateTimeFormatter.ofPattern("E- dd MMM yyyy HH:mm:ss Z");
                LocalDateTime resolvedTime = LocalDateTime.parse(resolvedDate, jiraDateFormatter);
                LocalDateTime updatedTime = LocalDateTime.parse(updatedDate, jiraDateFormatter);

                if (resolvedTime.isAfter(updatedTime)){
                    latestDateBetweenUpdatedAndResolved = resolvedDate;
                }
                else{
                    latestDateBetweenUpdatedAndResolved = updatedDate;
                }

                csvPrintWriter.println(project + "," + commitID + "," + developer + "," + title + "," + summary + ","
                        + parsedIssueKey + "," + issueType + "," + createdDate + ","
                        + latestDateBetweenUpdatedAndResolved + "," + patternChanges);
                csvPrintWriter.flush();
                csvPrintWriter.close();

                /////////////////////////////////////
                ///// WRITE TO INDIVIDUAL FILES /////
                /////////////////////////////////////

                File finalResultsFile = new File("finalResults-" + analyzedProject.substring(0,analyzedProject.indexOf("-")) + File.separator +
                        "finalAnalysis-" + outputFile.getName());
                Files.copy(issueTagsFile.toPath(), finalResultsFile.toPath(), StandardCopyOption.REPLACE_EXISTING);
                FileWriter fr = new FileWriter(finalResultsFile, true);
                BufferedWriter br = new BufferedWriter(fr);
                br.write("\n\n\n==================================\n Issue " + issueKey + " Description \n=======================================");
                br.write("\n\nProject: " + err.getElementsByTagName("project").item(0).getTextContent());
                br.write("\n-----------------");
                br.write("\n\n-----------------\n");
                br.write("Title: " + err.getElementsByTagName("title").item(0).getTextContent());
                br.write("\n-----------------");
                br.write("\n\n-----------------\n");
                br.write("Summary: " + err.getElementsByTagName("summary").item(0).getTextContent());
                br.write("\n-----------------");
                br.write("\n\n-----------------\n");
                br.write("Issue type: " + err.getElementsByTagName("type").item(0).getTextContent());
                br.write("\n-----------------");
                br.write("\n\n-----------------\n");
                br.write("Current status: " + err.getElementsByTagName("status").item(0).getTextContent());
                br.write("\n-----------------");
                br.write("\n\n-----------------\n");
                br.write("Created at: " + err.getElementsByTagName("created").item(0).getTextContent());
                br.write("\n-----------------");
                br.write("\n\n-----------------\n");
                br.write("Resolved at: " + err.getElementsByTagName("resolved").item(0).getTextContent());
                br.write("\n-----------------");
                br.write("\n\n-----------------\n");
                br.write("Assigned to: " + err.getElementsByTagName("assignee").item(0).getTextContent());
                br.write("\n-----------------");
                br.write("\n\n-----------------\n");
                String description = err.getElementsByTagName("description").item(0).getTextContent()
                        .replaceAll("<p>|<\\/p>", "\n");
                br.write("Description: \n");
                for (String line : textLimiter(description, 90)) {
                    br.write(line);
                    br.write("\n");
                }

                /*
                Additional analysis is done here, where from an array of
                words (patterns), the description and comments from an issue
                are compared to check if these are discussed inside the issues
                 */
                String[] patternNames = {"Abstract Factory", "abstract factory", "Factory Method", "factory method",
                        "Singleton", "singleton","Adapter","adapter","Bridge","bridge",
                        "Composite","composite","Decorator","decorator","Facade","facade","Flyweight","flyweight",
                        "Proxy","proxy","Chain of Responsibility","chain of responsibility","Mediator","mediator",
                        "Observer","observer"," State "," state ","Strategy","strategy","Template Method","template method",
                        "Visitor","visitor","Pattern","pattern", "factory","Factory", "adapt", "decorate", "mediate", "observe",
                        "Builder","builder","prototype","Prototype","Command","command","Interpreter","interpreter","interprete",
                        "Iterator","iterator","iterate","Memento","memento","template","Template", "Visitor", "visitor",
                        "visit","chain", "Chain"};

                for (String patternInstance : patternNames) {
                    if (summary.contains(patternInstance)) {
                        writePatternMentionsBW.write("On issue key " + parsedIssueKey + " the " + patternInstance +
                                " pattern might have been discussed, namely here: \n");
                        writePatternMentionsBW.write("==============================\n");
                        for (String commentSmallerLine : textLimiter(summary, 90)) {
                            writePatternMentionsBW.write(commentSmallerLine);
                            writePatternMentionsBW.write("\n");
                        }
                        writePatternMentionsBW.write("==============================\n\n");
                    }
                }

                NodeList commentsNodes = doc.getElementsByTagName("comments");
                Element commentElement = (Element) commentsNodes.item(0);

                //br.write("\n-----------------");
                //br.write("\n\n-----------------\n");
                //br.write("Comments: \n\n");

                for (int i = 0; i < commentElement.getElementsByTagName("comment").getLength() -1; i++){
                    String comment = commentElement.getElementsByTagName("comment").item(i).getTextContent()
                            .replaceAll("<p>|<\\/p>", "");

                    for (String patternInstance : patternNames){
                        if (comment.contains(patternInstance)){
                            writePatternMentionsBW.write("On issue key " + parsedIssueKey + " the " + patternInstance
                                    + " pattern might have been discussed on the following comment: \n");
                            writePatternMentionsBW.write("==============================\n");
                            for (String commentSmallerLine : textLimiter(comment, 90)) {
                                writePatternMentionsBW.write(commentSmallerLine);
                                writePatternMentionsBW.write("\n");
                            }
                            writePatternMentionsBW.write("==============================\n\n");
                        }
                    }

                    //br.write("New Comment: \n");
                    //for (String commentSmallerLine : textLimiter(comment, 90)) {
                    //    br.write(commentSmallerLine);
                    //    br.write("\n");
                    //}
                    //br.write("\n\n");
                }

                br.close();
                fr.close();
                writePatternMentionsBW.flush();
                writePatternMentionsBW.close();
                writePatternMentionsFW.close();
            } else {
                // success
            }
        } catch (Exception e) {
            System.out.println(e);
        }
    }

    /*
    This function is responsible for doing checking all the issues in a JIRA repository for a given project
    and writing the information to separate files, it is similar to the getResponse, but this only checks for the
    summary, key and comments to detect possible occurrences of discussion of patterns in these issues.
     */
    private static void scanIssue(String issueKey) {

        try {
            String format = "xml";
            String url = "https://issues.apache.org/jira/si/jira.issueviews:issue-" + format + "/" + issueKey + "/" + issueKey + ".xml";

            System.out.println(url);
            URL obj = new URL(url);
            HttpURLConnection con = (HttpURLConnection) obj.openConnection();
            int responseCode = con.getResponseCode();
            System.out.println("Response Code : " + responseCode);
            BufferedReader in = new BufferedReader(
                    new InputStreamReader(con.getInputStream()));
            String inputLine;
            StringBuffer response = new StringBuffer();
            while ((inputLine = in.readLine()) != null) {
                response.append(inputLine);
            }
            in.close();
            //print in String
            //System.out.println(response.toString());
            Document doc = DocumentBuilderFactory.newInstance().newDocumentBuilder()
                    .parse(new InputSource(new StringReader(response.toString())));
            NodeList errNodes = doc.getElementsByTagName("item");

            FileWriter writeIssueFW = new FileWriter(new File("AllIssues-" + analyzedProject.substring(0,analyzedProject.indexOf("-"))
                    + File.separator + issueKey + ".txt"));
            BufferedWriter writeIssueBW = new BufferedWriter(writeIssueFW);

            if (errNodes.getLength() > 0) {
                Element err = (Element) errNodes.item(0);

                String summary = err.getElementsByTagName("summary").item(0).getTextContent().replaceAll(",|;", "-");
                String parsedIssueKey = err.getElementsByTagName("key").item(0).getTextContent().replaceAll(",|;", "-");

            /*
                Additional analysis is done here, where from an array of
                words (patterns), the description and comments from an issue
                are compared to check if these are discussed inside the issues
                 */
                String[] patternNames = {"Abstract Factory", "abstract factory", "Factory Method", "factory method",
                        "Singleton", "singleton","Adapter","adapter","Bridge","bridge",
                        "Composite","composite","Decorator","decorator","Facade","facade","Flyweight","flyweight",
                        "Proxy","proxy","Chain of Responsibility","chain of responsibility","Mediator","mediator",
                        "Observer","observer"," State "," state ","Strategy","strategy","Template Method","template method",
                        "Visitor","visitor","Pattern","pattern", "factory","Factory", "adapt", "decorate", "mediate", "observe",
                        "Builder","builder","prototype","Prototype","Command","command","Interpreter","interpreter","interprete",
                        "Iterator","iterator","iterate","Memento","memento","template","Template", "Visitor", "visitor",
                        "visit","chain", "Chain"};

                for (String patternInstance : patternNames) {
                    if (summary.contains(patternInstance)) {
                        writeIssueBW.write("On issue key " + parsedIssueKey + " the " + patternInstance +
                                " pattern might have been discussed, namely here: \n");
                        writeIssueBW.write("==============================\n");
                        for (String commentSmallerLine : textLimiter(summary, 90)) {
                            writeIssueBW.write(commentSmallerLine);
                            writeIssueBW.write("\n");
                        }
                        writeIssueBW.write("==============================\n\n");
                    }
                }

                NodeList commentsNodes = doc.getElementsByTagName("comments");
                Element commentElement = (Element) commentsNodes.item(0);

                writeIssueBW.write("\n-----------------");
                writeIssueBW.write("\n\n-----------------\n");
                writeIssueBW.write("Comments: \n\n");

                for (int i = 0; i < commentElement.getElementsByTagName("comment").getLength() -1; i++){
                    String comment = commentElement.getElementsByTagName("comment").item(i).getTextContent()
                            .replaceAll("<p>|<\\/p>", "");

                    for (String patternInstance : patternNames){
                        if (comment.contains(patternInstance)){
                            writeIssueBW.write("On issue key " + parsedIssueKey + " the " + patternInstance
                                    + " pattern might have been discussed on the following comment: \n");
                            writeIssueBW.write("==============================\n");
                            for (String commentSmallerLine : textLimiter(comment, 90)) {
                                writeIssueBW.write(commentSmallerLine);
                                writeIssueBW.write("\n");
                            }
                            writeIssueBW.write("==============================\n\n");
                        }
                    }

                    writeIssueBW.write("New Comment: \n");
                    for (String commentSmallerLine : textLimiter(comment, 90)) {
                        writeIssueBW.write(commentSmallerLine);
                        writeIssueBW.write("\n");
                    }
                    writeIssueBW.write("\n\n");
                }
            }
        }
        catch (Exception e) {
            System.out.println(e);
        }
    }

    /*
    Function that receives a String and prints portions of it to a line,
    so that there's no overflow of characters in a line, making the
    resulting file hard to read
     */
    private static List<String> textLimiter(String input, int limit) {
        List<String> returnList = new ArrayList<>();
        String[] parts = input.split(" ");
        StringBuilder sb = new StringBuilder();
        for (String part : parts) {
            if (sb.length() + part.length() > limit) {
                returnList.add(sb.toString().substring(0, sb.toString().length() - 1));
                sb = new StringBuilder();
            }
            sb.append(part + " ");
        }
        if (sb.length() > 0) {
            returnList.add(sb.toString());
        }
        return returnList;
    }
}
