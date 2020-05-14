import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.List;
import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;

public class JiraXMLIssueRequester {

    //Change according to the name of the folder where the pinot outputs are available
    private static String analyzedProject = "mina-issueTags";

    public static void main(String[] args) {

        File[] issueTagsFiles = createDirectoryAndFileArray();

        for (File analyzedFile : issueTagsFiles) {

            String issueKey = obtainIssueKey(analyzedFile);

            if (issueKey != null) {
                get_response(issueKey, analyzedFile);
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

                if (line.contains("ZOOKEEPER-") || line.contains("DIRMINA-") || line.contains("HDFS-")) {
                    String[] segments = line.split(" |\\.|:|\\(|\\)");

                    for (String segment : segments) {
                        if (segment.contains("ZOOKEEPER-") || segment.contains("DIRMINA-") || segment.contains("HDFS-")) {
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


    public static void get_response(String issueKey, File outputFile) {
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
            if (errNodes.getLength() > 0) {
                Element err = (Element) errNodes.item(0);

                //File file = new File(outputFile.getName());


                File issueTagsFile = new File(".\\" + analyzedProject + "\\" + outputFile.getName());

                File finalResultsFile = new File(".\\finalResults-" + analyzedProject + "\\" + outputFile.getName());

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
                String description = err.getElementsByTagName("description").item(0).getTextContent().replaceAll("<p>|<\\/p>", "\n");
                br.write("Description: \n");
                for (String line : textLimiter(description, 40)) {
                    br.write(line);
                    br.write("\n");
                }
                br.close();
                fr.close();
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
