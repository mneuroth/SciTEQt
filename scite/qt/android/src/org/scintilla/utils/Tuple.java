package org.scintilla.utils;

/// helper class to hold success flag and content of a file read operation
class Tuple {
    private boolean mSuccess;
    private byte[] mContent;

    public Tuple(boolean success, byte[] content)
    {
        mSuccess = success;
        mContent = content;
    }

    boolean GetSuccessFlag()
    {
        return mSuccess;
    }

    byte[] GetContent()
    {
        return mContent;
    }
}
