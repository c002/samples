# 
# 

class Const:
    """Global Constants"""

    ISONRTR=1
    ISINDB=2
    IGNORE=3

    HAVE_ROUTER=10
    HAVE_INTERFACE=11
    HAVE_BANDWIDTH=12
    HAVE_DESCRIPTION=13
    HAVE_IFSTATUS=14
    HAVE_CUSTOMER=15
    HAVE_ROUTES=16
    HAVE_ROUTE=17       # db & conig both have the route
    HAVE_GATEWAY=18     # db & config Destination gateways match
    HAVE_NETGATEWAY=19  # db & config Subnets match in destination for a route

    INTERFACE_SHUTDOWN=101	# Route attribute, interface assoc is shutdown
    DEFAULT_OUTRATE=102		# Rate limmiting set to interface default
    DEFAULT_INRATE=103		# Rate limmiting set to interface default

