package nu.flacco.server.wire;

public class Acknowledge {
        public final static String STATUS_OK="OK";					// ack receipt
        public final static String STATUS_ERROR="ERROR";			// Some error condition
        public final static String STATUS_CONFIRM="CONFIRMED";		// Succesful confirmation
        public final static String STATUS_FAIL="FAIL";				// Failed confirmation
        
	    private long timestamp;
        private String status=null;
        private String message=null;
        
        public String toString() 
        { 
        	return(String.format("ts=%d, Status=%s, Msg=%s", timestamp, status, message));
        }
        public long getTimestamp() {
            return timestamp;
        }
        public void setTimestamp(long timestamp) {
            this.timestamp = timestamp;
        }
        public String getStatus() {
            return status;
        }
        public void setStatus(String status) {
            this.status = status;
        }
		public String getMessage() {
			return message;
		}
		public void setMessage(String message) {
			this.message = message;
		}

}
