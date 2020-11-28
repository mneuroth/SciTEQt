// (c) 2017 Ekkehard Gentz (ekke)
// this project is based on ideas from
// http://blog.lasconic.com/share-on-ios-and-android-using-qml/
// see github project https://github.com/lasconic/ShareUtils-QML
// also inspired by:
// https://www.androidcode.ninja/android-share-intent-example/
// https://www.calligra.org/blogs/sharing-with-qt-on-android/
// https://stackoverflow.com/questions/7156932/open-file-in-another-app
// http://www.qtcentre.org/threads/58668-How-to-use-QAndroidJniObject-for-intent-setData
// OpenURL in At Android: got ideas from:
// https://github.com/BernhardWenzel/open-url-in-qt-android
// https://github.com/tobiatesan/android_intents_qt
//
// see also /COPYRIGHT and /LICENSE

package org.scintilla.activity.sharex;

import org.qtproject.qt5.android.QtNative;

import org.qtproject.qt5.android.bindings.QtActivity;
import android.os.*;
import android.content.*;
import android.app.*;

import java.lang.String;
import android.content.Intent;
import java.io.File;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.InputStream;
import java.io.ByteArrayOutputStream;
import android.net.Uri;
import android.util.Log;
import android.content.ContentResolver;
import android.webkit.MimeTypeMap;

import org.scintilla.utils.*;



public class QShareActivity extends QtActivity
{
    // native - must be implemented in Cpp via JNI
    // 'file' scheme or resolved from 'content' scheme:
    public static native void setFileUrlReceived(String url);
    // InputStream from 'content' scheme:
    public static native void setFileReceivedAndSaved(String url);
    //
    public static native void setTextContentReceived(String text);
    //
    public static native void setUnknownContentReceived(String errorMsg);
    //
    public static native void fireActivityResult(int requestCode, int resultCode, String uriTxt);
    //
    public static native void fireFileOpenActivityResult(int resultCode, String fileUri, String decodedFileUri, byte[] fileContent);
    //
    public static native void fireFileCreateActivityResult(int resultCode, String fileUri, String decodedFileUri);
    //
    public static native boolean checkFileExits(String url);

    public static native void qDebugOutput(String txt);

    public static boolean isIntentPending;
    public static boolean isInitialized;
    public static String workingDirPath;

    // Use a custom Chooser without providing own App as share target !
    // see QShareUtils.java createCustomChooserAndStartActivity()
    // Selecting your own App as target could cause AndroidOS to call
    // onCreate() instead of onNewIntent()
    // and then you are in trouble because we're using 'singleInstance' as LaunchMode
    // more details: my blog at Qt
    @Override
    public void onCreate(Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);
//qDebugOutput("--> onCreate");
          Log.d("ekkescorner", "onCreate QShareActivity");
          // now we're checking if the App was started from another Android App via Intent
          Intent theIntent = getIntent();
          if (theIntent != null){
//qDebugOutput("--> onCreate (1)");
              String theAction = theIntent.getAction();
//qDebugOutput("--> onCreate (2) "+theAction);
              if (theAction != null){
                  Log.d("ekkescorner onCreate ", theAction);
//qDebugOutput("--> onCreate (3)");
                  // QML UI not ready yet
                  // delay processIntent();
                  isIntentPending = true;
              }
          }
//qDebugOutput("--> onCreate (4)");
    } // onCreate


private String readTextFromUri(Uri uri) throws IOException {
    InputStream inputStream = getContentResolver().openInputStream(uri);
    BufferedReader reader = new BufferedReader(new InputStreamReader(
            inputStream));
    StringBuilder stringBuilder = new StringBuilder();
    String line;
    while ((line = reader.readLine()) != null) {
        stringBuilder.append(line+"\n");
    }
    inputStream.close();
    reader.close();
    return stringBuilder.toString();
}

private byte[] readBinaryFileFromUri(Uri uri) throws IOException {
    InputStream inputStream = getContentResolver().openInputStream(uri);

    //byte[] array = inputStream.readAllBytes();

    ByteArrayOutputStream buffer = new ByteArrayOutputStream();

    int nRead;
    byte[] data = new byte[4096];

    while((nRead = inputStream.read(data, 0, data.length)) != -1)
    {
        buffer.write(data, 0, nRead);
    }

    return buffer.toByteArray();
}

    // we start Activity with result code
    // to test JNI with QAndroidActivityResultReceiver you must comment or rename
    // this method here - otherwise you'll get wrong request or result codes
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if(requestCode == org.scintilla.utils.QShareUtils.REQUEST_ID_OPEN_FILE)
        {
            if(resultCode == RESULT_OK)
            {
                String uri = data.getData().toString();
                String decodedUri = Uri.decode(data.getData().toString());
                // returns something like: "content://com.android.externalstorage.documents/document/0000.0000:Texte/datei.txt"
                // !decode something like: "content://com.android.externalstorage.documents/document/0000.0000%3ATexte%2Fdatei.txt" (sd-card)
                // !decode something like: "content://com.google.android.apps.docs.sotrage/document/acc%D1%3Bdoc%3Dencoded%3DOr...." (google-drive)
                // see: https://stackoverflow.com/questions/25171246/open-a-google-drive-file-content-uri-after-using-kitkat-storage-access-framework
                // for readable file name ?
                // see: https://github.com/googlesamples/android-DirectorySelection
                byte[] content = null;
                try
                {
                    content = readBinaryFileFromUri(data.getData());
                }
                catch(IOException exc)
                {
                    content = new byte[0];
                }
                fireFileOpenActivityResult(resultCode, uri, decodedUri, content);
            }
            else
            {
                fireFileOpenActivityResult(resultCode, new String(), new String(), new byte[0]);
            }
        }
        else if(requestCode == org.scintilla.utils.QShareUtils.REQUEST_ID_CREATE_FILE)
        {
            if(resultCode == RESULT_OK)
            {
                String uri = data.getData().toString();
                String decodedUri = Uri.decode(data.getData().toString());
                fireFileCreateActivityResult(resultCode, uri, decodedUri);
            }
            else
            {
                fireFileCreateActivityResult(resultCode, new String(), new String());
            }
        }
        else
        {
            String uriTxt = "";
            // Check which request we're responding to
            //Log.d("ekkescorner onActivityResult", "requestCode: "+requestCode);
            if (resultCode == RESULT_OK) {
                //Log.d("ekkescorner onActivityResult - resultCode: ", "SUCCESS");
                try
                {
                    uriTxt = data.getData().toString();
                    //uriTxt = readTextFromUri(data.getData());
                }
                catch(Exception exc)
                {
                    //Log.d("ekkescorner onActivityResult Exception", exc.toString());
                }
            } else {
                //Log.d("ekkescorner onActivityResult - resultCode: ", "CANCEL");
            }
            // hint: result comes back too fast for Action SEND
            // if you want to delete/move the File add a Timer w 500ms delay
            // see Example App main.qml - delayDeleteTimer
            // if you want to revoke permissions for older OS
            // it makes sense also do this after the delay
            fireActivityResult(requestCode, resultCode, uriTxt);
        }
    }

    // if we are opened from other apps:
    @Override
    public void onNewIntent(Intent intent) {
//qDebugOutput("--> onNewIntent");
      //Log.d("ekkescorner", "onNewIntent");
      super.onNewIntent(intent);
      setIntent(intent);
      // Intent will be processed, if all is initialized and Qt / QML can handle the event
      if(isInitialized) {
          processIntent();
      } else {
          isIntentPending = true;
      }
    } // onNewIntent

    public void checkPendingIntents(String workingDir) {
//qDebugOutput("--> checkPendingIntents");
        isInitialized = true;
        workingDirPath = workingDir;
        //Log.d("ekkescorner", workingDirPath);
        if(isIntentPending) {
            isIntentPending = false;
            //Log.d("ekkescorner", "checkPendingIntents: true");
            processIntent();
        } else {
            //Log.d("ekkescorner", "nothingPending");
        }
    } // checkPendingIntents

    // process the Intent if Action is SEND or VIEW
    private void processIntent(){
//qDebugOutput("--> PROCESS_INTENT");

      Intent intent = getIntent();

      Uri intentUri;
      String intentScheme;
      String intentAction;
      // we are listening to android.intent.action.SEND or VIEW (see Manifest)
      if (intent.getAction().equals("android.intent.action.VIEW")){
//qDebugOutput("--> PROCESS_INTENT VIEW");
             intentAction = "VIEW";
             intentUri = intent.getData();
      } else if (intent.getAction().equals("android.intent.action.SEND")){
//qDebugOutput("--> PROCESS_INTENT SEND");
//             String type = intent.getType();
//qDebugOutput(type);
             intentAction = "SEND";
             Bundle bundle = intent.getExtras();
             intentUri = (Uri)bundle.get(Intent.EXTRA_STREAM);
      } else if (intent.getAction().equals("android.intent.action.MAIN")){
             // ignore main intent of this application !
             return;
      } else {
//qDebugOutput("--> PROCESS_INTENT UNKNOWN");
//qDebugOutput(intent.getAction().toString());
              //Log.d("ekkescorner Intent unknown action:", intent.getAction());
              setUnknownContentReceived("Warning: Intent unknown action:"+intent.getAction());
              return;
      }
      //Log.d("ekkescorner action:", intentAction);
      if (intentUri == null){
//qDebugOutput("--> PROCESS_INTENT URI is NULL");         // this is the branch when selected text is shared with sciteqt --> empty uri --> https://developer.android.com/reference/android/content/Intent --> Intent.EXTRA_TEXT
            Bundle bundle = intent.getExtras();
            String txt = bundle.getCharSequence(Intent.EXTRA_TEXT).toString();
//qDebugOutput("TXT=");
//qDebugOutput(txt);
            setTextContentReceived(txt);
            //Log.d("ekkescorner Intent URI:", "is null");
            return;
      }

      //Log.d("ekkescorner Intent URI:", intentUri.toString());

      // content or file
      intentScheme = intentUri.getScheme();
      if (intentScheme == null){
//qDebugOutput("--> PROCESS_INTENT URI is NULL (2)");
            //Log.d("ekkescorner Intent URI Scheme:", "is null");
            setUnknownContentReceived("Warning: Intent URI Scheme is null");
            return;
      }
      if(intentScheme.equals("file")){
//qDebugOutput("--> PROCESS_INTENT file ok (2)");
            // URI as encoded string
            //Log.d("ekkescorner Intent File URI: ", intentUri.toString());
            setFileUrlReceived(intentUri.toString());
            // we are done Qt can deal with file scheme
            return;
      }
      if(!intentScheme.equals("content")){
//qDebugOutput("--> PROCESS_INTENT no CONTENT");
              //Log.d("ekkescorner Intent URI unknown scheme: ", intentScheme);
              setUnknownContentReceived("Warning: Intent URI Scheme is "+intentScheme.toString());
              return;
      }
      // ok - it's a content scheme URI
      // we will try to resolve the Path to a File URI
      // if this won't work or if the File cannot be opened,
      // we'll try to copy the file into our App working dir via InputStream
      // hopefully in most cases PathResolver will give a path

//qDebugOutput("--> PROCESS_INTENT <<CONTINUE>>");
      // you need the file extension, MimeType or Name from ContentResolver ?
      // here's HowTo get it:
      //Log.d("ekkescorner Intent Content URI: ", intentUri.toString());
      ContentResolver cR = this.getContentResolver();
      MimeTypeMap mime = MimeTypeMap.getSingleton();
      String fileExtension = mime.getExtensionFromMimeType(cR.getType(intentUri));
      //Log.d("ekkescorner","Intent extension: "+fileExtension);
      String mimeType = cR.getType(intentUri);
      //Log.d("ekkescorner"," Intent MimeType: "+mimeType);
      String name = QShareUtils.getContentName(cR, intentUri);
      if(name != null) {
           //Log.d("ekkescorner Intent Name:", name);
      } else {
           //Log.d("ekkescorner Intent Name:", "is NULL");
      }
      String filePath;
      filePath = QSharePathResolver.getRealPathFromURI(this, intentUri);
      if(filePath == null) {
//qDebugOutput("--> PROCESS_INTENT FILEPATH is NULL");
            //Log.d("ekkescorner QSharePathResolver:", "filePath is NULL");
      } else {
            //Log.d("ekkescorner QSharePathResolver:", filePath);
            // to be safe check if this File Url really can be opened by Qt
            // there were problems with MS office apps on Android 7
//qDebugOutput("--> PROCESS_INTENT exit file exists");
            if (checkFileExits(filePath)) {
                setFileUrlReceived(filePath);
                // we are done Qt can deal with file scheme
                return;
            }
      }

      // trying the InputStream way:
      filePath = QShareUtils.createFile(cR, intentUri, workingDirPath);
      if(filePath == null) {
//qDebugOutput("--> PROCESS_INTENT URI is NULL (3)");
           //Log.d("ekkescorner Intent FilePath:", "is NULL");
           setUnknownContentReceived("Warning: Intent URI Scheme is really null");
           return;
      }
//qDebugOutput("--> PROCESS_INTENT NORMAL EXIT "+filePath);
      setFileReceivedAndSaved(filePath);
    } // processIntent
} // class QShareActivity
