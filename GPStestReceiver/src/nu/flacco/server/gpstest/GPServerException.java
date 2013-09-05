/*
 * ClyException.java
 *
 * Created on 13 November 2007, 14:55
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

package nu.flacco.server.gpstest;

/**
 *
 * @author t600387
 */

public class GPServerException extends java.lang.Exception {
    /**
     *
     */
    private static final long serialVersionUID = 1L;
    final static String rcsid = "$Source$";
    final static String rcssrc= "$Id:$";

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
    public GPServerException() {
    }

    public GPServerException(Exception e) {
        super(e);
    }
    /**
     * Constructs an instance of <code>ClyException</code> with the specified detail message.
     * @param msg the detail message.
     */
    public GPServerException(String msg) {
        super(msg);
    }

    /**
     * Exceptin type and message
     * @param severity The severity of the exception
     * @param msg The text message describing the reason for the error
     */
    public GPServerException(int severity, String msg) {
        super(msg);
        throw new java.lang.Error(msg);
    }
}
