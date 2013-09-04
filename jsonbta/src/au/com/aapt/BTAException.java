package au.com.aapt;

public class BTAException extends java.lang.Exception  {
    /**
     *
     */
    private static final long serialVersionUID = 1L;
    /**
     * FATAL expcetion.  Application should quit
     */
    public static int FATAL=1;
    /**
     * Non FATAL exception
     */
    public static int WARN=2;

    /**
     * Creates a new instance of <code>AHDException</code> without detail message.
     */
    public BTAException() {
    }

    /**
     * Constructs an instance of <code>ClyException</code> with the specified detail message.
     * @param msg the detail message.
     */
    public BTAException(String msg) {
        super(msg);
    }

    /**
     * Exceptin type and message
     * @param severity The severity of the exception
     * @param msg The text message describing the reason for the error
     */
    public BTAException(int severity, String msg) {
        super(msg);
        throw new java.lang.Error(msg);
    }
}
