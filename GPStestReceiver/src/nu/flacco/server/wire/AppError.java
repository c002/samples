package nu.flacco.server.wire;

public class AppError {

    String error;
    String detail;

    public AppError(String error, String detail)
    {
        this.error=error;
        this.detail=detail;
    }
    
    public void setError(AppError error)
    {
        this.error=error.getError();
        this.detail=error.getDetail();
    }
    
    public String toString()
    {
    	return String.format("AppError: error=%s, detail=%s",error, detail);
    }
    
    public AppError()
    {}

    public String getError() {
        return error;
    }
    public void setError(String error) {
        this.error = error;
    }
    public String getDetail() {
        return detail;
    }
    public void setDetail(String detail) {
        this.detail = detail;
    }


}
