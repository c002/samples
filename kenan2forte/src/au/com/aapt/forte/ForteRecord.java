package au.com.aapt.forte;

import org.apache.log4j.Logger;

public class ForteRecord extends Table
{
    private static final Logger log = Logger.getLogger("au.com.aapt.forte.ForteRecord");

    ForteClient fclient;
    ForteCustomer fcustomer;
    ForteContact fcontact;
    ForteAddress faddrstreet;
    ForteAddress faddrpostal;
    ForteAddress faddrinvoice;
    ForteCompany fcompany;
    ForteCustomerDetails fcustomerdetails;
    ForteMasterGroup fmastergroup=null;     // Not required
    ForteMasterGroupLink fmastergrouplink;

    public ForteRecord() throws K2FException
    {
        fclient = new ForteClient();
        fcustomer = new ForteCustomer();
        fcontact = new ForteContact();
        faddrstreet = new ForteAddress();
        faddrpostal = new ForteAddress();
        faddrinvoice = new ForteAddress();
        fcompany = new ForteCompany();
        fcustomerdetails = new ForteCustomerDetails();
        fmastergrouplink = new ForteMasterGroupLink();
    }

    public String toString()
    {
        StringBuilder s = new StringBuilder();
        s.append("   |--> ForteRecord()\n");
        if (fclient!=null) s.append(fclient.toString());
        if (fcustomer!=null) s.append(fcustomer.toString());
        if (fcontact!=null) s.append(fcontact.toString());
        if (faddrpostal!=null) s.append(faddrpostal.toString());
        if (faddrstreet!=null) s.append(faddrstreet.toString());
        if (faddrinvoice!=null) s.append(faddrinvoice.toString());
        if (fcompany!=null)s.append(fcompany.toString());
        if (fcustomerdetails!=null) s.append(fcustomerdetails.toString());
        if (fmastergrouplink!=null) s.append(fmastergrouplink.toString());

        return(s.toString());
    }

    public String summary() {
        StringBuilder s = new StringBuilder();
        s.append(getFclient().getClientid() + " " + getFclient().getName());
        return(s.toString());
    }

    public void update(DBConnect db) throws K2FException {
        if (fclient!=null) fclient.update(db);
        if (fcustomer!=null) fcustomer.update(db);
        if (fcontact!=null) fcontact.update(db);
        if (faddrstreet!=null) faddrstreet.update(db);
        if (faddrpostal!=null) faddrpostal.update(db);
        if (faddrinvoice!=null) faddrinvoice.update(db);
        if (fcompany!=null) fcompany.update(db);
        if (fcustomerdetails!=null) fcustomerdetails.update(db);
        if (fmastergrouplink!=null) fmastergrouplink.update(db);
    }

    public ForteAddress getFaddrstreet() {
        return faddrstreet;
    }

    public void setFaddrstreet(ForteAddress faddrstreet) {
        this.faddrstreet = faddrstreet;
    }

    public ForteAddress getFaddrpostal() {
        return faddrpostal;
    }

    public void setFaddrpostal(ForteAddress faddrpostal) {
        this.faddrpostal = faddrpostal;
    }

    public ForteClient getFclient() {
        return fclient;
    }

    public void setFclient(ForteClient fclient) {
        this.fclient = fclient;
    }

    public ForteContact getFcontact() {
        return fcontact;
    }

    public void setFcontact(ForteContact fcontact) {
        this.fcontact = fcontact;
    }

    public ForteAddress getFaddrinvoice() {
        return faddrinvoice;
    }

    public void setFaddrinvoice(ForteAddress faddrinvoice) {
        this.faddrinvoice = faddrinvoice;
    }

    public ForteCompany getFcompany() {
        return fcompany;
    }

    public void setFcompany(ForteCompany fcompany) {
        this.fcompany = fcompany;
    }

    public ForteCustomerDetails getFcustomerdetails() {
        return fcustomerdetails;
    }

    public void setFcustomerdetails(ForteCustomerDetails fcustomerdetails) {
        this.fcustomerdetails = fcustomerdetails;
    }

    public ForteMasterGroup getFmastergroup() {
        return fmastergroup;
    }

    public void setFmastergroup(ForteMasterGroup fmastergroup) {
        this.fmastergroup = fmastergroup;
    }

    public ForteCustomer getFcustomer() {
        return fcustomer;
    }

    public void setFcustomer(ForteCustomer fcustomer) {
        this.fcustomer = fcustomer;
    }

    public ForteMasterGroupLink getFmastergrouplink() {
        return fmastergrouplink;
    }

    public void setFmastergrouplink(ForteMasterGroupLink fmastergrouplink) {
        this.fmastergrouplink = fmastergrouplink;
    }

}
