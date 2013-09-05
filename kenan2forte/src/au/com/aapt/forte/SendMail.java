package au.com.aapt.forte;

//File Name SendEmail.java

import java.util.*;
import javax.mail.*;
import javax.mail.internet.*;
import javax.mail.search.RecipientStringTerm;
//import javax.activation.*;

import org.apache.log4j.Logger;

public class SendMail
{
    protected static final Logger log = Logger.getLogger("au.com.aapt.forte.MapKenan2Forte");

		
	public SendMail(String subject, String body)
	{
		String to = AppProps.getInstance().getMailTo();
		String from = AppProps.getInstance().getMailFrom();
		String host = AppProps.getInstance().getMailHost();

   // Get system properties
   Properties properties = System.getProperties();

   // Setup mail server
   properties.setProperty("mail.smtp.host", host);

   // Get the default Session object.
   Session session = Session.getDefaultInstance(properties);

   try{
      // Create a default MimeMessage object.
      MimeMessage message = new MimeMessage(session);

      // Set From: header field of the header.
      message.setFrom(new InternetAddress(from));

      // Set To: header field of the header.
      String [] recipients = to.split(",");
      for (int i=0; i<recipients.length; i++) {
    	  message.addRecipient(Message.RecipientType.TO,
                               new InternetAddress(recipients[i]));
      }
      // Set Subject: header field
      message.setSubject(subject);

      // Now set the actual message
      message.setText(body);

      // Send message
      Transport.send(message);
   }catch (MessagingException mex) {
      mex.printStackTrace();
   }
}
}