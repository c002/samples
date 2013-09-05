package nu.flacco.server.wire;

public class Coordinate {

    private int lon=0;
    private int lat=0;
    static int MILLION=1000000;

    public String toString()
    {
        String lonstr= String.format("%1.6f", new Float((float) lon) /1000000);
        String latstr= String.format("%1.6f", new Float((float) lat) /1000000);
        return("GeoPoint: "+latstr+","+lonstr);
    }

    public Coordinate()
    {

    }
    
     public static String mkLngLatstr(double lon, double lat)
    {
           return(mkLngLatstr((int) (lon*MILLION), (int) (lat*MILLION)));
    }

    public static String mkLngLatstr(long lon, long lat)
    {

           return(mkLngLatstr((int) lon, (int) lat));
    }

    public static String mkLngLatstr(int lon, int lat)
    {
        StringBuilder sb = new StringBuilder();

        if (lon>=0) 	 sb.append((String.format("%1.6fE,",new Float((float)lon/MILLION))));
        else if (lon<0) sb.append((String.format("%.6fW,", new Float(Math.abs((float)lon/MILLION)))));
         if (lat>=0) 	 sb.append(String.format("%1.6fN",new Float((float)lat/MILLION)));
        else if (lat<0) sb.append(String.format("%1.6fS", new Float(Math.abs(((float)lat)/MILLION))));

           return(sb.toString());
    }

    public String mkLngLatstr()
    {
        StringBuilder sb = new StringBuilder();

        if (lon>=0) 	 sb.append((String.format("%1.6fE,",new Float((float)lon/MILLION))));
        else if (lon<0) sb.append((String.format("%.6fW,", new Float(Math.abs((float)lon/MILLION)))));
         if (lat>=0) 	 sb.append(String.format("%1.6fN",new Float((float)lat/MILLION)));
        else if (lat<0) sb.append(String.format("%1.6fS", new Float(Math.abs(((float)lat)/MILLION))));

           return(sb.toString());
    }

    public Coordinate(int lon, int lat) {
        this.lat = lat;
        this.lon = lon;
    }

    public Coordinate(long lon, long lat) {
        this.lat = (int) (lat);
        this.lon = (int) (lon);
    }

    public Coordinate(double lon, double lat) {
        this.lat = (int) (lat * MILLION);
        this.lon = (int) (lon * MILLION);
    }

    public int getLon() {
        return lon;
    }
    public void setLon(int lon) {
        this.lon = lon;
    }
    public int getLat() {
        return lat;
    }
    public void setLat(double lat) {
        this.lat = (int) (lat*MILLION);
    }

    public double getDoubleLon() {
        return (double) lon/MILLION;
    }

    public double getDoubleLat() {
        return (double) lat/MILLION;
    }

    public void setLon(double lon) {
        this.lon = (int) (lon*MILLION);
    }
    public void setLat(int lat) {
        this.lat = lat;
    }

    // + = N , - = S
    public String toLatstr()
    {
        if (lat>=0) 	return(String.format("%1.6fN",new Float((float)lat/MILLION)));
        else if (lat<0) return(String.format("%1.6fS", new Float(Math.abs(((float)lat)/MILLION))));
        return(null);
    }

    // + = E, - = W
    public String toLngStr()
    {
        if (lon>=0) 	return(String.format("%1.6fE",new Float((float)lon/MILLION)));
        else if (lon<0) return(String.format("%.6fW", new Float(Math.abs((float)lon/MILLION))));
        return(null);

    }

}
