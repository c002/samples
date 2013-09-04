package nu.flacco.android.bta;

import org.json.JSONObject;

public class CustomerDataOrder {

    private String customerName=null;
    private String dataOrderName=null;
    private int customer=0;
    private int dataOrder=0;
    private int trafficIncluded=0;
    private long toUsage=0;
    private long fromUsage=0;
    private String error=null;
    private String errordetail=null;
    private String status=null;

    public CustomerDataOrder() {

    }

    // Unmarshall object from JSONObject
    public CustomerDataOrder(JSONObject jsondata) {
        try {
            if (jsondata.has("customerName"))
                setCustomerName(jsondata.getString( "customerName" ));
            if (jsondata.has("dataOrderName"))
                setDataOrderName(jsondata.getString( "dataOrderName" ));
            if (jsondata.has("customer"))
                setCustomer(jsondata.getInt( "customer" ));
            if (jsondata.has("dataOrder"))
                setDataOrder(jsondata.getInt( "dataOrder" ));
            if (jsondata.has("trafficIncluded"))
                setTrafficIncluded(jsondata.getInt( "trafficIncluded" ));
            if (jsondata.has("toUsage"))
                setToUsage(jsondata.getLong( "toUsage" ));
            if (jsondata.has("fromUsage"))
                setFromUsage(jsondata.getLong( "fromUsage" ));
            if (jsondata.has("error"))
                setError(jsondata.getString( "error" ));
            if (jsondata.has("errordetail"))
                setErrordetail(jsondata.getString( "errordetail" ));
            if (jsondata.has("status"))
                setStatus(jsondata.getString( "status" ));
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public String toString()
    {
        StringBuilder sb = new StringBuilder();

        sb.append("Customer="+getCustomer()+"\n");
        sb.append("CustomerName="+getCustomerName()+"\n");
        sb.append("DataOrder="+getDataOrder()+"\n");
        sb.append("DataOrderName="+getDataOrderName()+"\n");
        sb.append("TrafficIncluded(GB)="+getTrafficIncluded()+"\n");
        sb.append("toUsage(MB)="+getToUsage()+"\n");
        sb.append("fromUsage(MB)="+getFromUsage()+"\n");

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
    public Integer getDataOrder() {
        return dataOrder;
    }
    public void setDataOrder(Integer dataOrder) {
        this.dataOrder = dataOrder;
    }
    public int getTrafficIncluded() {
        return trafficIncluded;
    }
    public void setTrafficIncluded(int trafficIncluded) {
        this.trafficIncluded = trafficIncluded;
    }
    public long getToUsage() {
        return toUsage;
    }
    public void setToUsage(long toUsage) {
        this.toUsage = toUsage;
    }
    public long getFromUsage() {
        return fromUsage;
    }
    public void setFromUsage(long fromUsage) {
        this.fromUsage = fromUsage;
    }

    public String getError() {
        return error;
    }

    public void setError(String error) {
        this.error = error;
    }

    public String getErrordetail() {
        return errordetail;
    }

    public void setErrordetail(String errordetail) {
        this.errordetail = errordetail;
    }

    public String getStatus() {
        return status;
    }

    public void setStatus(String status) {
        this.status = status;
    }

}
