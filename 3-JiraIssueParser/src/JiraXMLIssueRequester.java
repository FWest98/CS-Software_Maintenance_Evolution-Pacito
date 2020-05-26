import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;

public class JiraXMLIssueRequester {

    //Change according to the name of the folder where the pinot outputs are available
    private static String analyzedProject = "mina-J7-issueTags";

    public static void main(String[] args) throws IOException {

        File[] issueTagsFiles = createDirectoryAndFileArray();

        File finalCSVFile = new File(".\\finalResults-" + analyzedProject + "\\" + analyzedProject + "-CSV.csv");

        FileWriter fw = new FileWriter(finalCSVFile.getPath());
        BufferedWriter bw = new BufferedWriter(fw);
        PrintWriter pw = new PrintWriter(bw);
        pw.println("Project" + "," + "CommitID" + "," + "Developer" + "," + "Title" + "," + "Summary" + ","
                + "IssueKey" + "," + "IssueType" + "," + "CreatedDate" + "," + "ResolvedDate" + "," + "PatternChanges");
        pw.flush();
        pw.close();

        for (File analyzedFile : issueTagsFiles) {

            String issueKey = obtainIssueKey(analyzedFile);

            if (issueKey != null) {
                get_response(issueKey, analyzedFile, finalCSVFile);
            }
        }

    }

    private static File[] createDirectoryAndFileArray() {

        //Create directory to store results if it does not exist already
        File directory = new File(".\\finalResults-" + analyzedProject);
        directory.mkdir();

        //Store the files from issueTags outputs to an array
        File[] files = new File(".\\" + analyzedProject).listFiles();

        return files;
    }


    public static String obtainIssueKey(File analyzedFile) {

        String issueKey = null;

        BufferedReader reader;
        try {
            reader = new BufferedReader(new FileReader(analyzedFile));
            String line = reader.readLine();
            while (line != null) {

                if (line.contains("ZOOKEEPER-") || line.contains("DIRMINA-") || line.contains("HDFS-") || line.contains("CASSANDRA-")) {
                    String[] segments = line.split(" |\\.|:|\\(|\\)");

                    for (String segment : segments) {
                        if (segment.contains("ZOOKEEPER-") || segment.contains("DIRMINA-") || segment.contains("HDFS-") || line.contains("CASSANDRA-")) {
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


    public static void get_response(String issueKey, File outputFile, File csvFile) {
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

            FileWriter csvFileWriter = new FileWriter(csvFile.getPath(), true);
            BufferedWriter csvBufferedWriter = new BufferedWriter(csvFileWriter);
            PrintWriter csvPrintWriter = new PrintWriter(csvBufferedWriter);

            FileWriter writePatternMentionsFW = new FileWriter("patternsMentionedInIssues.txt", true);
            BufferedWriter writePatternMentionsBW = new BufferedWriter(writePatternMentionsFW);

            if (errNodes.getLength() > 0) {
                Element err = (Element) errNodes.item(0);

                File issueTagsFile = new File(".\\" + analyzedProject + "\\" + outputFile.getName());
                FileReader fileReaderForCommitID = new FileReader(issueTagsFile.getPath());
                BufferedReader bufferedReaderForCommitID = new BufferedReader(fileReaderForCommitID);
                String firstLine = bufferedReaderForCommitID.readLine();
                String before;
                String after;
                StringBuilder detectedPatterns = new StringBuilder();
                String detectedPattern;
                String bufferedLine = bufferedReaderForCommitID.readLine();
                boolean reached = false;

                while (bufferedLine != null && reached == false) {

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
                                detectedPatterns.append(patternDifferences + " instances of the "
                                        + detectedPattern + " Pattern were REMOVED,");
                            } else {
                                detectedPatterns.append(Math.abs(patternDifferences) + " instances of the "
                                        + detectedPattern + " Pattern were ADDED,");
                            }
                        }
                    }
                    bufferedLine = bufferedReaderForCommitID.readLine();
                }

                String project = err.getElementsByTagName("project").item(0).getTextContent().replaceAll(",|;", "-");
                String commitID = firstLine.substring(firstLine.lastIndexOf(" ")+1);
                String developer = err.getElementsByTagName("assignee").item(0).getTextContent().replaceAll(",|;", "-");
                String title = err.getElementsByTagName("title").item(0).getTextContent().replaceAll(",|;", "-");
                String summary = err.getElementsByTagName("summary").item(0).getTextContent().replaceAll(",|;", "-");
                String parsedIssueKey = err.getElementsByTagName("key").item(0).getTextContent().replaceAll(",|;", "-");
                String issueType = err.getElementsByTagName("type").item(0).getTextContent().replaceAll(",|;", "-");
                String createdDate = err.getElementsByTagName("created").item(0).getTextContent().replaceAll(",|;", "-");
                String resolvedDate = err.getElementsByTagName("resolved").item(0).getTextContent().replaceAll(",|;", "-");
                String patternChanges = detectedPatterns.toString();

                /////////////////////////////
                ///// WRITE TO CSV FILE /////
                /////////////////////////////

                csvPrintWriter.println(project + "," + commitID + "," + developer + "," + title + "," + summary + ","
                        + parsedIssueKey + "," + issueType + "," + createdDate + "," + resolvedDate + "," + patternChanges);
                csvPrintWriter.flush();
                csvPrintWriter.close();

                /////////////////////////////////////
                ///// WRITE TO INDIVIDUAL FILES /////
                /////////////////////////////////////

                File finalResultsFile = new File(".\\finalResults-" + analyzedProject + "\\finalAnalysis-" + outputFile.getName());
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


                String[] patternNames = {"Abstract Factory", "abstract factory", "Factory Method", "factory method",
                        "Singleton", "singleton","Adapter","adapter","Bridge","bridge",
                        "Composite","composite","Decorator","decorator","Facade","facade","Flyweight","flyweight",
                        "Proxy","proxy","Chain of Responsibility","chain of responsibility","Mediator","mediator",
                        "Observer","observer"," State "," state ","Strategy","strategy","Template Method","template method",
                        "Visitor","visitor","Pattern","pattern"};

                for (String patternInstance : patternNames) {
                    if (summary.contains(patternInstance)) {
                        writePatternMentionsBW.write("On issue key " + parsedIssueKey + " the " + patternInstance +
                                "pattern might have been discussed, namely here: \n");
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

                br.write("\n-----------------");
                br.write("\n\n-----------------\n");
                br.write("Comments: \n\n");

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

                    br.write("New Comment: \n");
                    for (String commentSmallerLine : textLimiter(comment, 90)) {
                        br.write(commentSmallerLine);
                        br.write("\n");
                    }
                    br.write("\n\n");
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
