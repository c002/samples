/*
 * ClyException.java
 *
 * Created on 13 November 2007, 14:55
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

package au.com.aapt.forte;

/**
 *
 * @author t600387
 */

public class K2FException extends java.lang.Exception {
    /**
     *
     */
    private static final long serialVersionUID = 1L;
    final static String rcsid = "$Source: /import/cvsroot/dev/internal/kenan2forte/src/au/com/aapt/forte/K2FException.java,v $";
    final static String rcssrc= "$Id: K2FException.java,v 1.1 2012/03/28 04:23:42 harryr Exp $";

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
    public K2FException() {
    }

    public K2FException(Exception e) {
        super(e);
    }
    /**
     * Constructs an instance of <code>ClyException</code> with the specified detail message.
     * @param msg the detail message.
     */
    public K2FException(String msg) {
        super(msg);
    }

    /**
     * Exceptin type and message
     * @param severity The severity of the exception
     * @param msg The text message describing the reason for the error
     */
    public K2FException(int severity, String msg) {
        super(msg);
        throw new java.lang.Error(msg);
    }
}
