// (c) 2017 Ekkehard Gentz (ekke)
// this project is based on ideas from
// http://blog.lasconic.com/share-on-ios-and-android-using-qml/
// see github project https://github.com/lasconic/ShareUtils-QML
// also inspired by:
// https://www.androidcode.ninja/android-share-intent-example/
// https://www.calligra.org/blogs/sharing-with-qt-on-android/
// https://stackoverflow.com/questions/7156932/open-file-in-another-app
// http://www.qtcentre.org/threads/58668-How-to-use-QAndroidJniObject-for-intent-setData
// https://stackoverflow.com/questions/5734678/custom-filtering-of-intent-chooser-based-on-installed-android-package-name
// see also /COPYRIGHT and /LICENSE

package org.scintilla.utils;

//import org.scintilla.activity.sharex.QShareActivity;
import org.qtproject.qt5.android.QtNative;

import java.lang.String;
import android.content.Intent;
import java.io.File;
import android.net.Uri;
import android.util.Log;

import android.content.ContentResolver;
import android.database.Cursor;
import android.provider.MediaStore;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.FileOutputStream;
import java.io.FileInputStream;
import java.io.BufferedReader;
import java.io.InputStreamReader;

import java.util.List;
import android.content.pm.ResolveInfo;
import java.util.ArrayList;
import android.content.pm.PackageManager;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;
import java.util.Comparator;
import java.util.Collections;
import android.content.Context;
import android.os.Parcelable;

import android.os.Build;

import android.support.v4.content.FileProvider;
import android.support.v4.app.ShareCompat;

public class QStorageAccess
{
    // reference Authority as defined in AndroidManifest.xml
    private static String AUTHORITY="org.scintilla.sciteqt.fileprovider";

    public static int REQUEST_ID_OPEN_FILE = 101;
    public static int REQUEST_ID_CREATE_FILE = 102;

    protected QStorageAccess()
    {
    }

public static boolean openFile() {
    if (QtNative.activity() == null)
        return false;

    // using v4 support library create the Intent from ShareCompat
    Intent openIntent = ShareCompat.IntentBuilder.from(QtNative.activity()).getIntent();
    openIntent.setAction(Intent.ACTION_OPEN_DOCUMENT);

    openIntent.setType("*/*");

    openIntent.addCategory(Intent.CATEGORY_OPENABLE);

    openIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
    openIntent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION);

    QtNative.activity().startActivityForResult(openIntent, REQUEST_ID_OPEN_FILE);

    return true;
}

public static byte[] readFile(String fileUri) {
    if (QtNative.activity() == null)
        return new byte[0];

    try {
        ParcelFileDescriptor pfd = QtNative.activity().getContentResolver().openFileDescriptor(Uri.parse(fileUri), "r");
        FileInputStream fileInputStream = new FileInputStream(pfd.getFileDescriptor());
        byte[] content = new byte[fileInputStream.available()];
        fileInputStream.read(content);
        // Let the document provider know you're done by closing the stream.
        fileInputStream.close();
        pfd.close();
        return content;
    } catch (FileNotFoundException e) {
        e.printStackTrace();
    } catch (IOException e) {
        e.printStackTrace();
    }
    return new byte[0];
}

private static boolean alterDocument(Uri uri, byte[] content) {
    try {
        ParcelFileDescriptor pfd = QtNative.activity().getContentResolver().openFileDescriptor(uri, "w");
        FileOutputStream fileOutputStream = new FileOutputStream(pfd.getFileDescriptor());
        fileOutputStream.write(content);
        // Let the document provider know you're done by closing the stream.
        fileOutputStream.close();
        pfd.close();
        return true;
    } catch (FileNotFoundException e) {
        e.printStackTrace();
    } catch (IOException e) {
        e.printStackTrace();
    }
    return false;
}

public static boolean updateFile(String fileUri, byte[] fileContent) {
    if (QtNative.activity() == null)
        return false;

    return alterDocument(Uri.parse(fileUri), fileContent);
}

public static boolean deleteFile(String fileUri) {
    if (QtNative.activity() == null)
        return false;

    boolean ok = false;
    try
    {
        ok = DocumentsContract.deleteDocument(QtNative.activity().getContentResolver(), Uri.parse(fileUri));
    }
    catch(FileNotFoundException exc)
    {
        ok = false;
    }
    return ok;
}

public static boolean createFile(String fileName, String mimeType) {
    if (QtNative.activity() == null)
        return false;

    // using v4 support library create the Intent from ShareCompat
    Intent openIntent = ShareCompat.IntentBuilder.from(QtNative.activity()).getIntent();
    openIntent.setAction(Intent.ACTION_CREATE_DOCUMENT);

    openIntent.setType(mimeType);

    openIntent.addCategory(Intent.CATEGORY_OPENABLE);

    openIntent.putExtra(Intent.EXTRA_TITLE, fileName);

    openIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
    openIntent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION);

    QtNative.activity().startActivityForResult(openIntent, REQUEST_ID_CREATE_FILE);

    return true;
}

}
