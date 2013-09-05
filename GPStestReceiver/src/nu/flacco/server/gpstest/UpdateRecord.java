package nu.flacco.server.gpstest;

public class UpdateRecord {

        final static int MILLION=1000000;

        private int sequence=0;
        private String deviceid;
        private String linenumber;
        private String subscriberid;
        private String opname;
        private String countryIso;
        private double longitude;
        private double lattitude;
        private long alitude;
        private long timestamp;
        private String provider;
        private float accuracy;
        private String GCMRegId;
        boolean active=true;
        
        boolean haveLocation=false;
        
        private int counter=0;
       // private Coordinate coord;

        public String toString()
        {
            return(String.format("devid=%s, ts=%d, lon=%3.6f, lat=%3.6f, alt=%d, prov=%s, acc=%3.2f\n gcmid=%s",
                    getDeviceid(), getTimestamp(),
                    getLongitude(), getLattitude(), getAltitude(),
                    getProvider(), getAccuracy(), getGCMRegId()) );

        }

        public int getSequence() {
            return sequence;
        }

        public void setSequence(int sequence) {
            this.sequence = sequence;
        }

        public String getDeviceid() {
            return deviceid;
        }

        public void setDeviceid(String deviceid) {
            this.deviceid = deviceid;
        }

        public double getLongitude() {
            return longitude;
        }

        public void setLongitude(double longitude) {
            this.longitude = longitude;
            counter++;
            if (counter>=2) setHaveLocation(true);
        }
        public void setLongitude(String longitude) {
            setLongitude(new Double(longitude).doubleValue());
        }

        public double getLattitude() {
            return lattitude;
        }
        public void setLattitude(String lattitude) {
            setLattitude(new Double(lattitude).doubleValue());
        }
        public void setLattitude(double lattitude) {
            this.lattitude = lattitude;
            counter++;
            if (counter>=2) setHaveLocation(true);
        }

        public long getAltitude() {
            return alitude;
        }

        public void setAltitude(long alitude) {
            this.alitude = alitude;
        }

        public long getTimestamp() {
            return timestamp;
        }

        public void setTimestamp(long timestamp) {
            this.timestamp = timestamp;
        }
        public void setTimestamp(String timestamp) {
            this.timestamp = new Long(timestamp).longValue();
        }

        public String getProvider() {
            return provider;
        }

        public void setProvider(String provider) {
            this.provider = provider;
        }

        public float getAccuracy() {
            return accuracy;
        }

        public void setAccuracy(float accuracy) {
            this.accuracy = accuracy;
        }
        public void setAccuracy(String accuracy) {
            this.accuracy = new Float(accuracy).floatValue();
        }

        public String getGCMRegId() {
            return GCMRegId;
        }

        public void setGCMRegId(String gCMRegId) {
            GCMRegId = gCMRegId;
        }

        public String getLinenumber() {
            return linenumber;
        }

        public void setLinenumber(String linenumber) {
            this.linenumber = linenumber;
        }

        public boolean isActive() {
			return active;
		}

		public void setActive(boolean active) {
			this.active = active;
		}

		public boolean isHaveLocation() {
            return haveLocation;
        }

        public void setHaveLocation(boolean haveLocation) {
            this.haveLocation = haveLocation;
        }

		public String getSubscriberid() {
			return subscriberid;
		}

		public void setSubscriberid(String subscriberid) {
			this.subscriberid = subscriberid;
		}

		public String getOpname() {
			return opname;
		}

		public void setOpname(String opname) {
			this.opname = opname;
		}

		public String getCountryIso() {
			return countryIso;
		}

		public void setCountryIso(String countryIso) {
			this.countryIso = countryIso;
		}

}
