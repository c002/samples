package au.com.aapt;

import java.math.BigDecimal;

import net.sf.json.JSONObject;

public class CustomerDataOrder {

    private String customerName=null;
    private String dataOrderName=null;
    private int customer=0;
    private int dataOrder=0;
    private int trafficIncluded=0;
    private long toUsage=0;
    private long fromUsage=0;

    public CustomerDataOrder() {

    }

    // Unmarshall object from JSONObject
    public CustomerDataOrder(JSONObject jsondo) {
        setCustomerName(jsondo.getString( "customerName" ));
        setDataOrderName(jsondo.getString( "dataOrderName" ));
        setCustomer(jsondo.getInt( "customer" ));
        setDataOrder(jsondo.getInt( "dataOrder" ));
        setTrafficIncluded(jsondo.getInt( "trafficIncluded" ));
        setToUsage(jsondo.getLong( "toUsage" ));
        setFromUsage(jsondo.getLong( "fromUsage" ));
    }

    public String toString()
    {
        StringBuilder sb = new StringBuilder();

        sb.append("Customer="+getCustomer()+",");
        sb.append("CustomerName="+getCustomerName()+",");
        sb.append("DataOrder="+getDataOrder()+",");
        sb.append("DataOrderName="+getDataOrderName()+",");
        sb.append("TrafficIncluded(GB)="+getTrafficIncluded()+",");
        sb.append("toUsage(MB)="+getToUsage()+",");
        sb.append("fromUsage(MB)="+getFromUsage());

        return(sb.toString());
    }

    public String getCustomerName() {
        return customerName;
    }
    public void setCustomerName(String customerName) {
        this.customerName = customerName;
    }
    public String getDataOrderName() {
        return dataOrderName;
    }
    public void setDataOrderName(String dataOrderName) {
        this.dataOrderName = dataOrderName;
    }
    public Integer getCustomer() {
        return customer;
    }
    public void setCustomer(Integer customer) {
        this.customer = customer;
    }
    public void setCustomer(int customer) {
        this.customer = new Integer(customer);
    }
    public Integer getDataOrder() {
        return dataOrder;
    }
    public void setDataOrder(Integer dataOrder) {
        this.dataOrder = dataOrder;
    }
    public void setDataOrder(int dataOrder) {
        this.dataOrder = new Integer(dataOrder);
    }
    public Integer getTrafficIncluded() {
        return trafficIncluded;
    }
    public void setTrafficIncluded(Integer trafficIncluded) {
        this.trafficIncluded = trafficIncluded;
    }
    public void setTrafficIncluded(int trafficIncluded) {
        this.trafficIncluded = new Integer(trafficIncluded);
    }
    public Long getToUsage() {
        return toUsage;
    }
    public void setToUsage(Long toUsage) {
        this.toUsage = toUsage;
    }
    public void setToUsage(long toUsage) {
        this.toUsage = new Long(toUsage);
    }
    public Long getFromUsage() {
        return fromUsage;
    }
    public void setFromUsage(Long fromUsage) {
        this.fromUsage = fromUsage;
    }
    public void setFromUsage(long fromUsage) {
        this.fromUsage = new Long(fromUsage);
    }
}
