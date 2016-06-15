// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.config.codegen;

import java.io.*;
import java.util.List;
import java.util.ArrayList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * This class generates a tree of CNodes from a .def file.
 *
 * @author gjoranv
 * @author <a href="mailto:musum@yahoo-inc.com">Harald Musum</a>
 */
public class DefParser {
    static final Pattern commentPattern = Pattern.compile("^\\s*#+\\s*(.*?)\\s*$");
    public static final Pattern versionPattern = Pattern.compile("^(version\\s*=\\s*)([0-9][0-9-]*)$");
    // Namespace must start with a letter, since Java (Java language Spec, section  3.8) and C++ identifiers cannot start with a digit
    public static final Pattern namespacePattern = Pattern.compile("^(namespace\\s*=\\s*)(([a-z][a-z0-9_]*)+([.][a-z][a-z0-9_]*)*)$");

    private final BufferedReader reader;
    private final String name;
    private InnerCNode root = null;
    private NormalizedDefinition normalizedDefinition = null;


	private String comment = "";

    /**
     * Creates a new parser for a .def file with the given name and that can be accessed by the given reader.
     *
     * @param name  The name of the .def file (not including version number and the '.def' suffix).
     * @param defReader  A reader to the .def file.
     */
    public DefParser(String name, Reader defReader) {
        this.name = createName(name);
        if (defReader == null) {
            throw new CodegenRuntimeException("Must have a non-null reader for a .def file.");
        }
        if (defReader instanceof BufferedReader) {
            reader = (BufferedReader)defReader;
        } else {
            reader = new BufferedReader(defReader);
        }
    }

    // If name contains namespace, return just name
    private String createName(String name) {
        if (name.contains(".")) {
            return name.substring(name.lastIndexOf(".") + 1);
        } else {
            return name;
        }
    }

    /**
     * Parses the .def file upon the initial call. Subsequent calls returns the result from the initial call.
     *
     * @return  A tree of CNodes representing this instance's .def file.
     * @throws CodegenRuntimeException upon errors.
     */
    public InnerCNode getTree() throws CodegenRuntimeException {
        try {
            if (root == null) parse();
        } catch (DefParserException | IOException e) {
            throw new CodegenRuntimeException("Error parsing or reading config definition." + e.getMessage(), e);
        }
        return root;
    }

    /**
     * Parses the input from the reader and builds a tree of CNodes representing the .def file.
     *
     * @throws IOException upon reader errors.
     * @throws DefParserException upon parsing errors.
     */
    void parse() throws IOException, DefParserException {
        root = new InnerCNode(name);
        normalizedDefinition = new NormalizedDefinition();

        String s;
        List<String> originalInput = new ArrayList<>();
        while ((s = reader.readLine()) != null) {
            originalInput.add(s);
        }
        reader.close();


        // Parse and build tree of the original input
        parseLines(root, originalInput, normalizedDefinition);
        root.setMd5(normalizedDefinition.generateMd5Sum());
    }

    /**
     * Parses one line from def-file and adds it to the tree.
     * TODO: Method too long!
     *
     * @param root     The root CNode in the tree.
     * @param line     A line from the def-file.
     * @param nd       A NormalizedDefinition object
     * @throws IllegalArgumentException upon error in line.
     */
    private void parseLine(CNode root, String line, NormalizedDefinition nd) throws IllegalArgumentException {
        line = NormalizedDefinition.normalize(line);
        line = line.trim();
        if (line.length() == 0) {
            // If having empty lines in between comments, that is logically a break in the comment too
            if (!comment.isEmpty()) {
                comment += "\n";
            }
            return;
        }
        Matcher commentMatch = commentPattern.matcher(line);
        if (commentMatch.matches()) {
            parseCommentLine(commentMatch);
            return;
        }
        Matcher versionMatch = versionPattern.matcher(line);
        if (versionMatch.matches()) {
            parseVersionLine(versionMatch);
            return;
        }
        Matcher namespaceMatcher = namespacePattern.matcher(line);
        if (namespaceMatcher.matches()) {
            parseNamespaceLine(namespaceMatcher.group(2));
            nd.addNormalizedLine(line);
            return;
        }
        // Only add lines that are not version, namespace or comment lines
        nd.addNormalizedLine(line);
        DefLine defLine = new DefLine(line);
        root.setLeaf(root.getName() + "." + defLine.getName(), defLine, comment);
        comment = "";
    }

    private void parseCommentLine(Matcher commentMatch) {
        if (!comment.isEmpty()) comment += "\n";
        String addition = commentMatch.group(1);
        if (addition.isEmpty()) addition = " ";
        comment += addition;
    }

    private void parseVersionLine(Matcher matcher) {
        root.setVersion(matcher.group(2));
        root.setComment(comment);
        comment = "";
    }

    private void parseNamespaceLine(String namespace) {
        if (namespace.startsWith("com.yahoo."))
            throw new IllegalArgumentException("Remove 'com.yahoo.' from the namespace '" + namespace +
                    "' - it will be automatically added to the java package name.");
        root.setNamespace(namespace);
        root.setComment(comment);
        comment = "";
    }

    void parseLines(CNode root, List<String> defLines, NormalizedDefinition nd) throws DefParserException {
        DefParserException failure = null;
        int lineNumber = 1;
        for (String line : defLines) {
            try {
                parseLine(root, line, nd);
                lineNumber++;
            } catch (IllegalArgumentException e) {
                String msg = "Error when parsing line " + lineNumber + ": " + line + "\n" + e.getMessage();
                failure = new DefParserException(msg, e);
                break;
            }
        }

        if (failure != null) {
            throw (failure);
        }
    }

    public NormalizedDefinition getNormalizedDefinition() {
 		return normalizedDefinition;
 	}

    /**
     * For debugging - dump the tree from the given root to System.out.
     */
    public static void dumpTree(CNode root, String indent) {
        StringBuilder sb = new StringBuilder(indent + root.getName());
        if (root instanceof LeafCNode) {
            LeafCNode leaf = ((LeafCNode)root);
            if (leaf.getDefaultValue() != null) {
                sb.append(" = ").append(((LeafCNode)root).getDefaultValue().getValue());
            }
        }
        System.out.println(sb.toString());
        if (!root.getComment().isEmpty()) {
            String comment = root.getComment();
            if (comment.contains("\n")) {
                comment = comment.substring(0, comment.indexOf("\n")) + "...";
            }
            if (comment.length() > 60) {
                comment = comment.substring(0, 57) + "...";
            }
            System.out.println(indent + "    comment: " + comment);
        }
        CNode[] children = root.getChildren();
        for (CNode c : children) {
            dumpTree(c, indent + "  ");
        }

    }

    class DefParserException extends Exception {
        DefParserException(String s, Throwable cause) {
            super(s, cause);
        }
    }
}
