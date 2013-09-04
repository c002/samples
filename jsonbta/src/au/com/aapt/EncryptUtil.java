package au.com.aapt;

import java.io.IOException;
import java.io.UnsupportedEncodingException;

//import sun.misc.BASE64Decoder;
//import sun.misc.BASE64Encoder;
import org.apache.commons.codec.binary.Base64;

public class EncryptUtil {
    
    
    public static final String DEFAULT_ENCODING="UTF-8"; 

    public static String base64encode(String text){
       try {
            String s = Base64.encodeBase64URLSafeString(text.getBytes( DEFAULT_ENCODING ));
            return(s);
       }
       catch (Exception e ) {
          return null;
       }
    }//base64encode

    public static String base64decode(String text)
    {
        try {
              
              String s1 = new String(Base64.decodeBase64(text.getBytes(DEFAULT_ENCODING)));
              return(s1);
          }
          catch ( Exception e ) {
            return null;
          }

       }//base64decode

        public String encodeXor64(String s) {
            return(encodeXor64(s,"nokey"));
        }
        
        public String encodeXor64(String s, String key) {
           return(base64encode( xorMessage( s, key) ));       
        }
        public String decodeXor64(String s) {
            return(decodeXor64(s,"nokey"));
        }
        
        public String decodeXor64(String s, String key) {
            return(xorMessage(base64decode(s), key ));       
         }
        
       public static String xorMessage(String message, String key){
       
           try {
           if (message==null || key==null ) return null;

          char[] keys=key.toCharArray();
          char[] mesg=message.toCharArray();

          int ml=mesg.length;
          int kl=keys.length;
          char[] newmsg=new char[ml];

          for (int i=0; i<ml; i++){
             newmsg[i]=(char)(mesg[i]^keys[i%kl]);
          }//for i
          mesg=null; keys=null;
          return new String(newmsg);
       }
       catch ( Exception e ) {
          return null;
        }
          
       }//xorMessage
       
 }//class
