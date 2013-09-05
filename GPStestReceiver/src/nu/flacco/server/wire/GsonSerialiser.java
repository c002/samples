package nu.flacco.server.wire;

import java.util.ArrayList;

import com.google.gson.*;

public class GsonSerialiser <T> {

    private Class<T> ctype;

    public GsonSerialiser(Class<T> ctype)
    {
        this.ctype = ctype;
    }

    public static String getType(String jsonstr) 
    {
        try {
            Gson gson2 = new Gson();
            JsonParser parser = new JsonParser();
            JsonArray array = parser.parse(jsonstr).getAsJsonArray();
            String type = gson2.fromJson(array.get(0), String.class);

            return(type);
        } catch (java.lang.IllegalStateException ise) {
            return(null);   // not a JSON string
        }
    
    }
    
    public String pack(T obj)
    {
        Gson gson = new Gson();
        ArrayList <Object>list = new ArrayList<Object>();
        list.add(obj.getClass().getName());
        list.add(obj);
        String jsonstr = gson.toJson(list);

        return(jsonstr);
    }

    public T unpack(String jsonstr)
    {
        Gson gson2 = new Gson();
        //DataClass dc3=null;
    //    T t = null;

        JsonParser parser = new JsonParser();
        JsonArray array = parser.parse(jsonstr).getAsJsonArray();
        String type = gson2.fromJson(array.get(0), String.class);
        System.out.println("class is: " + type);
        System.out.println("ctype is: " + ctype.getName());

        T x = gson2.fromJson(array.get(1), ctype);
        return(x);

        //if (type.compareTo(t.getClass().getName())==0)
        //    return(gson2.fromJson(array.get(1), ctype));
      //  if (type.compareTo("haz.DataClass")==0)
           // dc3 = gson2.fromJson(array.get(1), DataClass.class);

       // return(null);
    }


}
