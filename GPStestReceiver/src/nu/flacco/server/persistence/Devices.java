package nu.flacco.server.persistence;

import java.util.Date;

import javax.persistence.Entity;
import javax.persistence.Column;
import javax.persistence.GeneratedValue;
import javax.persistence.Id;
import javax.persistence.Table;
import javax.persistence.Transient;

@Entity
@Table(name="devices")
public class Devices {

    long iddevices;
    String deviceid;
    String linenumber;
    String subscriberid;
    String countryIso;
    String opname;
    String label;
    String gcmid;
    Integer flags;
    Date updated;
    Boolean active;

    public Devices() {}

    @Transient
    public String toString()
    {
        return(String.format("Devices: id=%d, devid=%s, ph=%s, " +
                "label=%s, gcmid=%s, flags=%d, upd=%s, act=%s",
                getIddevices(),
                getDeviceid(),
                getLinenumber(),
                getLabel(),
                getGcmid(),
                getFlags(),
                getUpdated(),
                isActive()
        ));
    }
    
    @Transient
    public void copyNotNull(Devices device)
    {
        // Not copying Id.
        setDeviceid(device.getDeviceid());
        setActive( device.isActive()==null ? isActive() : device.isActive() );
        setLinenumber(device.getLinenumber()==null ? getLinenumber() : device.getLinenumber() );
        setLabel(device.getLabel()==null ? getLabel() : device.getLabel());
        setGcmid(device.getGcmid()==null ? getGcmid() : device.getGcmid());
        setFlags(device.getFlags()==null ? getFlags() : device.getFlags());
        setUpdated(device.getUpdated());
        setCountryIso(device.getCountryIso()==null ? getCountryIso() : device.getCountryIso());
        setOpname(device.getOpname()==null ? getOpname() : device.getOpname());
        setSubscriberid(device.getSubscriberid()==null ? getSubscriberid() : device.getSubscriberid());
    }
    
    @Transient
    public boolean equals(Devices dev)
    {
        if (dev.getDeviceid() == getDeviceid()==true)
            //(dev.getLinenumber()==getLinenumber()==true))
            return(true);
        else
            return(false);
    }
    
    @Id
    @GeneratedValue
    public long getIddevices() {
        return iddevices;
    }
    public void setIddevices(long iddevices) {
        this.iddevices = iddevices;
    }

    @Column(name="deviceid")
    public String getDeviceid() {
        return deviceid;
    }
    public void setDeviceid(String deviceid) {
        this.deviceid = deviceid;
    }
    public String getLinenumber() {
        return linenumber;
    }
    public void setLinenumber(String linenumber) {
        this.linenumber = linenumber;
    }
    public String getLabel() {
        return label;
    }
    public void setLabel(String label) {
        this.label = label;
    }
    public String getGcmid() {
        return gcmid;
    }
    public void setGcmid(String gcmid) {
        this.gcmid = gcmid;
    }
    public Integer getFlags() {
        return flags;
    }
    public void setFlags(Integer flags) {
        this.flags = flags;
    }
    public Date getUpdated() {
        return updated;
    }
    public void setUpdated(Date updated) {
        this.updated = updated;
    }
    public Boolean isActive() {
        return active;
    }
    public void setActive(Boolean active) {
        this.active = active;
    }

	public String getSubscriberid() {
		return subscriberid;
	}

	public void setSubscriberid(String subscriberid) {
		this.subscriberid = subscriberid;
	}

	public String getCountryIso() {
		return countryIso;
	}

	public void setCountryIso(String countryIso) {
		this.countryIso = countryIso;
	}

	public String getOpname() {
		return opname;
	}

	public void setOpname(String opname) {
		this.opname = opname;
	}

}
