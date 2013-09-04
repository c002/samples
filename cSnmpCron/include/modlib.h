int rtrInfo(u_char *community, char *target, snmp_rtrinfo_t *info, taskresult_t *taskresult);
int rtrCalls(u_char *community, char *target, snmp_callhist_t *calls, taskresult_t *taskresult);
int rtrOctets(u_char *community, char *target, snmp_octets_t *octets, taskresult_t *taskresult);
