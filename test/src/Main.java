public class Main {

    String ola = "@Supreasasufhasifas ";
    String ola1 = "asjhdasjfahsfj \"@\"";
    String ola2 = " @Overrid";

    String regexForAnnotations = "^|(@[a-zA-Z]+)$(?=\\s)";
    String regexAnnotations = "(?<=^|)(@[a-zA-Z])";
    String anotherRegex = "(?<=.|^)(@[a-zA-Z].+?)(?=' '|$)";
    String regexAnnotation = "@[a-zA-Z].+?(?=$)";
    String dumbRegex = "(?<=^|.)@[a-zA-Z]";
    String maluco = "(@[a-zA-Z].+$)";


}
