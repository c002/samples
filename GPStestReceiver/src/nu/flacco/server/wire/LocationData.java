package nu.flacco.server.wire;

public class LocationData {
	
	private long timestamp;
    private String status=null;
    private String deviceId=null;	// IMEI eg.  *#06#  = 356402040782458
    private Coordinate coord;
    private float accuracy;
    private Boolean active;
    private String GCMId;
    private String provider;
	private String simserial;
	private String linenumber;
	private String subscriberid;	
    private String countryIso=null;
    private String simop=null;
    private String opname=null;
    private int cellid;
    private int areacode;
    private int scramble;
    private String cellstring; 
    
	public String toString()
	{
	    return(String.format("loc: ts=%d, st=%s, id=%s, sim=%s, ln=%s, sub=%s, " +
	    					 "ci=%d, ac=%d, sc=%d, cs=%s,so=%s, on=%s, iso=%s, coord=%s ",
	                            getTimestamp(), 
	                            getStatus(),
	                            getDeviceId(),
	                            getSimserial(),
	                            getLinenumber(),
	                            getSubscriberid(),
	                            getCellid(),
	                            getAreacode(),
	                            getScramble(),
	                            getCellstring(),
	                            getSimop(),
	                            getOpname(),
	                            getCountryIso(),
	                            getCoord().toString()
	                            ));
	}
	
	public long getTimestamp() {
		return timestamp;
	}
	
	public LocationData setTimestamp(long timestamp) {
		this.timestamp = timestamp;
		return this;
	}
	
	public String getDeviceId() {
		return deviceId;
	}
	public LocationData setDeviceId(String deviceId) {
		this.deviceId = deviceId;
		return this;
	}
	public String getStatus() {
		return status;
	}
	public LocationData setStatus(String status) {
		this.status = status;
		return this;
	}
	
	public Coordinate getCoord() {
		return coord;
	}
	
	public LocationData setCoord(Coordinate coord) {
		this.coord = coord;
		return this;
	}
	public float getAccuracy() {
		return accuracy;
	}
	public LocationData setAccuracy(float accuracy) {
		this.accuracy = accuracy;
		return this;
	}
	public String getProvider() {
		return provider;
	}
	public LocationData setProvider(String provider) {
		this.provider = provider;
		return this;
	}
    public String getSimserial() {
        return simserial;
    }
    public LocationData setSimserial(String simserial) {
        this.simserial = simserial;
		return this;
    }
    public String getLinenumber() {
        return linenumber;
    }
    public LocationData setLinenumber(String linenumber) {
        this.linenumber = linenumber;
		return this;
    }
	public int getCellid() {
		return cellid;
	}
	public LocationData setCellid(int cellid) {
		this.cellid = cellid;
		return this;
	}
	public int getAreacode() {
		return areacode;
	}
	public LocationData setAreacode(int areacode) {
		this.areacode = areacode;
		return this;
	}
	public int getScramble() {
		return scramble;
	}
	public LocationData setScramble(int scramble) {
		this.scramble = scramble;
		return this;
	}
	public String getCellstring() {
		return cellstring;
	}
	public LocationData setCellstring(String cellstring) {
		this.cellstring = cellstring;
		return this;
	}
	public String getSubscriberid() {
		return subscriberid;
	}
	public LocationData setSubscriberid(String subscriberid) {
		this.subscriberid = subscriberid;
		return this;
	}
	public String getCountryIso() {
		return countryIso;
	}
	public LocationData setCountryIso(String countryIso) {
		this.countryIso = countryIso;
		return this;
	}
	public String getSimop() {
		return simop;
	}
	public LocationData setSimop(String simop) {
		this.simop = simop;
		return this;
	}
	public String getOpname() {
		return opname;
	}
	public LocationData setOpname(String opname) {
		this.opname = opname;
		return this;
	}

	public String getGCMId() {
		return GCMId;
	}

	public void setGCMId(String gCMId) {
		GCMId = gCMId;
	}

	public Boolean getActive() {
		return active;
	}

	public void setActive(Boolean active) {
		this.active = active;
	}
	
}
