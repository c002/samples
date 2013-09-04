package au.com.aapt;

import java.math.BigDecimal;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.util.ArrayList;
import java.util.Iterator;

import au.com.aapt.AppProps;

public class BtaDataSource {

    AppProps appprops=null;
    ResultSet results = null;
    CustomerDataOrder cdo=null;
    
    public BtaDataSource()
    {
        try {
            appprops = AppProps.getInstance();
            appprops.loadProperties("application.properties");
        } catch (Exception e) {
            e.printStackTrace();
        }
        
        cdo = new CustomerDataOrder();
    }
    
    public CustomerDataOrder getDataOrderTraffic(int  customer, int dataorder)
    {
        Connection conn = null;
        PreparedStatement stmt=null;
        boolean haveDoResults=false;
        
        int affinityid=0;
        
        cdo.setCustomer(customer);
        cdo.setDataOrder(dataorder);
         // get Data Order Info for request
        String query="SELECT val.name, billable_item_id, vbi.affinity_id, " +
                      "do.trafficincluded, vc.name , cd.PATHTOMARKET " + 
                      "FROM cust.vforge_billable_item vbi, cust.vforge_affinity_link val, " + 
                      "cust.vforge_affinity va, " + 
                      "cust.vforge_client vc, " + 
                      "orders_f.dataorder do, " +
                      "ORDERS_F.SUBORDERS so, " +
                      "orders_f.customerdetails cd " +
                      "WHERE vbi.affinity_id=val.affinity_id " + 
                      "and do.fkcustomerid = ? " +
                      "and do.dataorderid = ? " +
                      "AND va.affinity_id=vbi.affinity_id " +
                      "AND parent_affinity_id IS NULL " + 
                      "AND do.fkcustomerid=cd.FKCUSTOMERID " +
                      "AND do.fkcustomerid=va.client_id " +
                      "AND do.fkcustomerid=vc.clientid " +
                      "AND do.dataorderid = va.order_id " +
                      "and so.suborderid = do.dataorderid " +
                      "AND do.status='Completed' " +
                      "AND SO.ORDERTYPE!='Cancellation' "+ 
                      "order by do.fkcustomerid ";

      try {
           conn = connect("ordersf");
           stmt = conn.prepareStatement(query);
           stmt.setInt(1, customer);
           stmt.setInt(2, dataorder);
           //stmt.executeQuery();

           results = stmt.executeQuery();
          
           while (results.next()) 
           {
               haveDoResults=true;
               cdo.setDataOrderName(results.getString(1));
               affinityid= results.getInt(3);
               cdo.setCustomerName(results.getString(5));
               cdo.setTrafficIncluded(toMbytes(results.getString(4)));
           }
           stmt.close();  
           
           if (haveDoResults==false) {
               stmt.close();
               conn.close();
               return(null);
           }
               
           // Get all  billable entities in DataOrder.
           // Test: cust=137839, do=2336807, affid=206473
           query="select c.billable_item_id " +
               "from cust.vforge_affinity_link b, " + 
               "cust.vforge_billable_item c " + 
               "where PARENT_AFFINITY_ID = ? " +
               "and b.affinity_id = c.affinity_id " +
               "and b.ACTIVE_TO is null ";
               
           stmt = conn.prepareStatement(query);
           stmt.setInt(1, affinityid);  
           results = stmt.executeQuery();
          
          // ArrayList billents = new ArrayList();  // 1.4 code
           ArrayList <Integer>billents = new ArrayList<Integer>();
           while (results.next()) 
           {
               Integer billent = new Integer(results.getInt(1));
               billents.add(billent);
           }

           // Close Ordersf connection
           stmt.close();
           conn.close();

           conn = connect("bta4");
           // get the subcustomers for Billable Entity
           query="select i.sub from " +
                 "bta4.billable_entity_link bel, " +
                 "bta4.customer_routes r, "+
                 "bta4.customer_ip_address i " +
                 "where " +
                 "bel.entity = ? " +
                 "and bel.link_id = r.link_id " + 
                 "and bel.customer = r.customer "+
                 "and bel.customer = i.customer "+
                 "and r.ip_id = i.ip_id " +
                 "and sysdate between r.start_time and r.end_time " +
                 "and sub!=0 ";
            
           stmt = conn.prepareStatement(query);
           
           ArrayList <Integer>subcusts = new ArrayList<Integer>();
           //ArrayList subcusts = new ArrayList();
           StringBuffer substr= new StringBuffer("(");
           
           // 1.4 code
      //     Iterator it = billents.iterator();
      //     while (it.hasNext()==true) {
          //     Integer be = (Integer) it.next();
          for (Integer be : billents) {
               stmt.setInt(1, be);
               results = stmt.executeQuery();
               while (results.next()) 
               {
                   int sub = results.getInt(1);
                   subcusts.add(sub);
                   substr.append(sub);
                   substr.append(",");
               }
               
           }
            
           substr.replace(substr.lastIndexOf(","), substr.lastIndexOf(",")+1,")");

          stmt.close();
          conn.close();
       
           // get total traffic for the Subcustomers in dataorders from start of month now.
           query="select trunc(sum(TO_USAGE)/1000000), trunc(sum(from_usage)/1000000) " +
               " from daily_summary " +
               " where " + 
               " cid=? " +
               " and sub in " + substr + 
               " and daystamp between  " + 
               " to_date( to_char(sysdate,'YYYY-MM')||'-01', 'YYYY-MM-DD') and sysdate";
           
           conn = connect("bta4traf2");
           stmt = conn.prepareStatement(query);
           stmt.setInt(1, customer);  
           results = stmt.executeQuery();
           
           while (results.next()) {
               cdo.setToUsage(results.getLong(1));
               cdo.setFromUsage(results.getLong(2));
           }
       } catch (Exception e) {
           e.printStackTrace();
           
       } finally {
           try {
               stmt.close();
               conn.close();
               
           } catch (Exception e){};
       }
       
       return(cdo);
   }
    
    private Connection connect(String dbstr)
    {

        String dbUser = null;
        String dbPass = null;
        String dbURL = null;

        Connection conn = null;

        if (dbstr.compareToIgnoreCase("bta4")==0) {
            dbUser = appprops.getBta_dbuser(); //"bta4_www";
            dbPass = appprops.getBta_dbpwd(); //"bta4_www98";
            dbURL =  appprops.getBta_dburl(); //"jdbc:oracle:thin:@orac.off.connect.com.au:1521:bta4";
         } else if (dbstr.compareToIgnoreCase("bta4traf2")==0) {
             dbUser = appprops.getBtausage_dbuser(); //"bta4_hist";
             dbPass = appprops.getBtausage_dbpwd(); // "krakenbta4";
             dbURL = appprops.getBtausage_dburl();  // "jdbc:oracle:thin:@orin.off.connect.com.au:1521:traf2";
         } else if (dbstr.compareToIgnoreCase("snmp")==0) {
             dbUser = appprops.getSnmp_dbuser(); //"bta4_hist";
             dbPass = appprops.getSnmp_dbpwd(); //"krakenbta4";
             dbURL = appprops.getSnmp_dburl();  //"jdbc:oracle:thin:@orin.off.connect.com.au:1521:traf2";
         } else if (dbstr.compareToIgnoreCase("radius")==0) {
             dbUser = appprops.getRadius_dbuser();
             dbPass = appprops.getRadius_dbpwd();
             dbURL = appprops.getRadius_dburl();
         } else if (dbstr.compareToIgnoreCase("ordersf")==0) {
             dbUser = appprops.getOrdersf_dbuser(); 
             dbPass = appprops.getOrdersf_dbpwd();
             dbURL = appprops.getOrdersf_dburl(); 
         }

         try {
            Class.forName("oracle.jdbc.driver.OracleDriver");

            conn = DriverManager.getConnection(dbURL, dbUser, dbPass);

            return(conn);
         } catch (Exception e) {
             e.printStackTrace();
         }
         return(null);
    }

    
    // Convert Free Form Forte GBytes included field.
    public int toMbytes(String trafficinc) 
    {
        StringBuffer sb = new StringBuffer();
        
        for (int i=0;i<trafficinc.length(); i++) {
          
            if (trafficinc.charAt(i)>='0' && trafficinc.charAt(i)<='9') { 
               sb.append(trafficinc.charAt(i));
            }
        }
        
        return(new Integer(sb.toString()).intValue() * 1000);
    }
}
