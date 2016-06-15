// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.document.serialization;

import com.yahoo.document.Document;
import com.yahoo.document.Field;
import com.yahoo.document.datatypes.*;
import com.yahoo.text.Utf8;
import org.apache.commons.codec.binary.Base64;

import java.io.UnsupportedEncodingException;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * Date: Apr 17, 2008
 *
 * @author <a href="mailto:humbe@yahoo-inc.com">H&aring;kon Humberset</a>
 */
public class XmlSerializationHelper {

    public static void printArrayXml(Array array, XmlStream xml) {
        List<FieldValue> lst = array.getValues();
        for (FieldValue value : lst) {
            xml.beginTag("item");
            value.printXml(xml);
            xml.endTag();
        }
    }

    public static <K extends FieldValue, V extends FieldValue> void printMapXml(MapFieldValue<K, V> map, XmlStream xml) {
        for (Map.Entry<K, V> e : map.entrySet()) {
            FieldValue key = e.getKey();
            FieldValue val = e.getValue();
            xml.beginTag("item");
            xml.beginTag("key");
            key.printXml(xml);
            xml.endTag();
            xml.beginTag("value");
            val.printXml(xml);
            xml.endTag();
            xml.endTag();
        }
    }

    public static void printByteXml(ByteFieldValue b, XmlStream xml) {
        xml.addContent(b.toString());
    }

    public static void printDocumentXml(Document doc, XmlStream xml) {
        xml.addAttribute("documenttype", doc.getDataType().getName());
        xml.addAttribute("documentid", doc.getId());
        final java.lang.Long lastModified = doc.getLastModified();
        if (lastModified != null) {
            xml.addAttribute("lastmodifiedtime", lastModified);
        }
        doc.getHeader().printXml(xml);
        doc.getBody().printXml(xml);
    }

    public static void printDoubleXml(DoubleFieldValue d, XmlStream xml) {
        xml.addContent(d.toString());
    }

    public static void printFloatXml(FloatFieldValue f, XmlStream xml) {
        xml.addContent(f.toString());
    }

    public static void printIntegerXml(IntegerFieldValue f, XmlStream xml) {
        xml.addContent(f.toString());
    }

    public static void printLongXml(LongFieldValue l, XmlStream xml) {
        xml.addContent(l.toString());
    }

    public static void printRawXml(Raw r, XmlStream xml) {
        xml.addAttribute("binaryencoding", "base64");
        xml.addContent(new Base64(0).encodeToString(r.getByteBuffer().array()));
    }

    public static void printStringXml(StringFieldValue s, XmlStream xml) {
        String content = s.getString();
        if (containsNonPrintableCharactersString(content)) {
            byte[] bytecontent = Utf8.toBytes(content);
            xml.addAttribute("binaryencoding", "base64");
            xml.addContent(new Base64(0).encodeToString(bytecontent));
        } else {
            xml.addContent(content);
        }
    }

    public static void printStructXml(Struct s, XmlStream xml) {
        Iterator<Map.Entry<Field, FieldValue>> it = s.iterator();
        while (it.hasNext()) {
            Map.Entry<Field, FieldValue> val = it.next();
            xml.beginTag(val.getKey().getName());
            val.getValue().printXml(xml);
            xml.endTag();
        }
    }

    public static void printWeightedSetXml(WeightedSet ws, XmlStream xml) {
        Iterator<FieldValue> it = ws.fieldValueIterator();
        while (it.hasNext()) {
            FieldValue val = it.next();
            xml.beginTag("item");
            xml.addAttribute("weight", ws.get(val));
            val.printXml(xml);
            xml.endTag();
        }
    }

    private static boolean containsNonPrintableCharactersByte(final byte[] buffer) {
        for (byte b : buffer) {
            if (b < 32 && (b != 9 && b != 10 && b != 13)) return true;
        }
        return false;
    }

    private static boolean containsNonPrintableCharactersString(final CharSequence buffer) {
        for (int i = 0; i < buffer.length(); i++) {
            char b = buffer.charAt(i);
            if (b < 32 && (b != 9 && b != 10 && b != 13)) return true;
        }
        return false;
    }
}
