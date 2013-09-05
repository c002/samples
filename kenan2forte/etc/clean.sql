delete from cust.vforge_company where 
clientid>220000 and clientid<240000
and modifydate between '10-may-2012' and '31-may-2012';

delete from cust.vforge_contact where 
clientid>220000 and clientid<240000
and modifydate between '10-may-2012' and '31-may-2012';

delete from cust.vforge_address where 
clientid>220000 and clientid<240000
and modifydate between '10-may-2012' and '31-may-2012';

delete from cust.vforge_client where 
clientid>220000 and clientid<240000
and modifydate between '10-may-2012' and '31-may-2012';

delete from orders_f.customerdetails where 
modifydate between '10-may-2012' and '31-may-2012'
and to_number(fkcustomerid)>220000 and to_number(fkcustomerid)<240000;

delete from orders_f.customer where 
customerid>220000 and customerid<240000
and modifydate between '10-may-2012' and '31-may-2012';
