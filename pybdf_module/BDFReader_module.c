/*** 
 * Python module wrapper for bdf_api.c 
 *
 * $Source: /usr/local/cvsroot/netflow/netflow/btalib/BDFReader_module.c,v $
 *
 ***/
#include <Python.h>
#include "bdf_api.h"

int initBDFReader(void);

static PyObject *pybdf_fopen(PyObject *self, PyObject *args);
static PyObject *pybdf_fclose(PyObject *self, PyObject *args);
static PyObject *pybdf_readhead(PyObject *self, PyObject *args);
static PyObject *pybdf_flowcount(PyObject *self, PyObject *args);
static PyObject *pybdf_readflow(PyObject *self, PyObject *args);
static PyObject *pybdf_taglookup(PyObject *self, PyObject *args);
static PyObject *pybdf_alttaglookup(PyObject *self, PyObject *args);
static PyObject *pybdf_alttagmasklookup(PyObject *self, PyObject *args);

static struct bdf_headers *bh; 

static char module_doc[] = "Implementation of a fast BDF file reader based on bdf_api.\n\
The following functions are available:\n\
fp=fopen(bdfname, mode) - Returns a python file object\n\
readhead(fp) - Reads the headers of a BDF file incl. tag and alttag dictionaries\n\
flowcount(fp) - Returns number of flows in file.\n\
taglookup(index) - Returns the name of tag with index\n\
alttaglookup(mask) - Returns the colon seperated list of alttag names for mask\n";


static char readhead_doc[] =
"readhead(fp) -> (bdf header tuple)\n\
\n\
fp is an open BDF file\n\
The return tuple contains the header fields, tag header and alttag header in the following order:\n\
(type, version_major, version_minor, file_size,collector_ip,checkpoint_time,time_start,time_end,rows)(type,rows)(type,rows)\n\
";

static char readflow_doc[] =
"readflow(fp) -> (bdf data tuple)\n\
\n\
fp is an open BDF file\n\
This returns a tuple contains the data for a flow as follows:\n\
( (long) ip_addr, (string) ip_addr, (long) ip_proxy, (string) ip_proxy, (long) Bytes_To,(long) Bytes_From,(int) tagindex ,(string) tagname, (int) AS_Number , (int) alttag mask )  \n\
";

/* Method table */

static PyMethodDef BDFReaderMethods[] = {
    {"fopen", pybdf_fopen, METH_VARARGS, "fp=fopen(name,mode) , Open a bdf file. Returns a python file object"},
    {"fclose", pybdf_fclose, METH_VARARGS, "fclose(fp) , Close a BDF file. Can also just use Python close()"},
    {"readhead", pybdf_readhead, METH_VARARGS, readhead_doc}, 
    {"flowcount", pybdf_flowcount, METH_VARARGS, "count=flowount(fp) , Returns the flowcounts in BDF file"}, 
    {"taglookup", pybdf_taglookup, METH_VARARGS, "tag=taglookup(index) , Returns the tagname at index"}, 
    {"alttaglookup", pybdf_alttaglookup, METH_VARARGS, "alttags=alttaglookup(mask) , Returns the alttag name for index"}, 
    {"alttagmasklookup", pybdf_alttagmasklookup, METH_VARARGS, "alttags=alttagmasklookup(mask) , Returns the alttag names for mask"}, 
    {"readflow", pybdf_readflow, METH_VARARGS, "flow=readflow(f), Returns the next flow data"}, 
    {NULL, NULL, 0, NULL}        /* Sentinel */
};


int initBDFReader(void)
{
    PyObject *m, *d;
    m = Py_InitModule3("BDFReader", BDFReaderMethods, module_doc);
    d = PyModule_GetDict(m);

    return(0);
}

/*
 * Open the filename given and return a python file object
 */
static PyObject *
pybdf_fopen(PyObject *self, PyObject *args)
{
    char *fname=NULL;
    FILE *fd;
    char *mode=NULL, *errstr=NULL;
    PyObject *pf;

    if (!PyArg_ParseTuple(args, "ss", &fname,&mode))
        return NULL;


    if (! (fd = fopen(fname, mode))) {
	errstr=(char *) malloc(1024); 
	sprintf(errstr,"BDFReader: Error opening BDF file %s", fname);
        PyErr_SetString(PyExc_IOError, errstr);
	return NULL;
    }

    pf = PyFile_FromFile(fd, fname, mode, fclose);
    return pf;
}

/*
 * Close a BDF file , takes a python file as arg.
 * Can just use pythons close() instead
 */
static PyObject *
pybdf_fclose(PyObject *self, PyObject *args)
{
    FILE *fp=NULL;
    PyObject *f;
    int res;

    if (!PyArg_ParseTuple(args, "O:ParseFile", &f))
        return NULL;

    if (PyFile_Check(f)) {
        fp = PyFile_AsFile(f);
    }

    res=fclose(fp);
    return Py_BuildValue("i", res);
}

/* 
 * Read all the headers in a BDF file
 *
 */
static PyObject *
pybdf_readhead(PyObject *self, PyObject *args)
{
    FILE *f1=NULL;
    PyObject *pf;
    int sts;
    char fmt[2048], errstr[1048];
    unsigned int type, vmajor, vminor;
    uint64_t fsize, cip, cp,tstart,tend,rows;

    unsigned int ttype, trows, atype,arows;

    if (!PyArg_ParseTuple(args, "O", &pf))
        return NULL;

    f1 = PyFile_AsFile(pf);	/* XXX */

    bh = (struct bdf_headers *) calloc(sizeof(struct bdf_headers),1); 

    if (! bh) { 
	PyErr_NoMemory();
	return NULL;
    }

    sts = bdf_readhead(f1, bh);

    if (sts<0) {
	sprintf(errstr,"BDFReader: %s", bdf_errlookup(sts));
        PyErr_SetString(PyExc_IOError, errstr);
	return NULL;
    }

    type=bh->head->type;
    vmajor=(bh->head->version >> 16);
    vminor=(bh->head->version & 0x00FF);
    fsize=bh->head->file_size;
    cip=bh->head->collector_ip;
    cp=bh->head->checkpoint_time;
    tstart=bh->head->time_start;
    tend=bh->head->time_end;
    rows=bh->head->rows;

    /* tag header */
    ttype=bh->taghead->type;
    trows=bh->taghead->rows;

    /* alttaghead */
    atype=bh->alttaghead->type;
    arows=bh->alttaghead->rows;

    /* tagdic */

    /* alttagdic */

    /* free(bh); */

    sprintf(fmt,"(iiiLLLLLL)(ii)(ii)");
    return Py_BuildValue(fmt,
		type, vmajor, vminor, fsize, 
		cip,cp,tstart,tend,rows,
		ttype, trows, atype,arows);
}

/*
 * Simple return the flowcount in file. Can just use 
 * readheader()
 *
 */
static PyObject *
pybdf_flowcount(PyObject *self, PyObject *args)
{
    FILE *f1=NULL;
    unsigned long count;
    PyObject *pf;

    if (!PyArg_ParseTuple(args, "O", &pf))
        return NULL;

     f1 = PyFile_AsFile(pf);	/* XXX */

     count = bdf_flowcount(f1);
     
     return Py_BuildValue("l", count);
}

/*
 * Read the next lot of flow data from BDF file.
 * The headers must have been read previously.
 */
static PyObject *
pybdf_readflow(PyObject *self, PyObject *args)
{
    FILE *f1=NULL;
    PyObject *pf;
    static bdf_file_data data;
    static char sip[24], sproxy[24];
    int n;
    unsigned long long ip_addr, ip_proxy;

    if (!PyArg_ParseTuple(args, "O", &pf))
        return NULL;

    if (PyFile_Check(pf)) {
        f1 = PyFile_AsFile(pf);
    }

    n = bdf_readflow(f1, &data);
    if (n) {

        ip_addr=data.ip_addr;	/* type conversion */
        ip_proxy=data.ip_proxy;	/* type conversion */
 
        ip2str(ip_addr, sip);	/* in bdf_api */
        ip2str(ip_proxy, sproxy);

        return Py_BuildValue("LsLsLLisii", 
		ip_addr, sip, ip_proxy, sproxy, data.bytes_to,
		data.bytes_from,data.tag_id,bh->tagdic[data.tag_id], data.aas,data.alt_tags);
    } else 
	return Py_BuildValue("s#", NULL);

}

/* 
 * Helper func to return the name of a tag. The name of the tag
 * is also returned as part of the flowdata in readflow()
 */
static PyObject *
pybdf_taglookup(PyObject *self, PyObject *args)
{
    int index=0;

    if (!PyArg_ParseTuple(args, "i", &index))
        return NULL;
 
    /* printf("%d \n", index); fflush(stdout); */

    if ((index>0) && (index<MAX_TAGS))
        return (Py_BuildValue("s", bh->tagdic[index]));
    else
        return (Py_BuildValue("s", "None"));
}

/*
 * Lookup the alttag name given a index.
 */
static PyObject *
pybdf_alttaglookup(PyObject *self, PyObject *args)
{
    int index=0;

    if (!PyArg_ParseTuple(args, "i", &index))
        return NULL;
 
    if ((index>0) && (index<MAX_ALTTAGS))
        return (Py_BuildValue("s", bh->alttagdic[index]));
    else
        return (Py_BuildValue("s", "None"));
}

/*
 * Lookup the alttag names given a mask.  A mask is a bitmask of size 32
 * where each bit maps onto an alttag. So multiple tags can be defined.
 * Returns a string with tags seperated by ':'
 */
static PyObject *
pybdf_alttagmasklookup(PyObject *self, PyObject *args)
{
    int mask=0, val=0, j=31;
    register unsigned int bit=1<<31;
    char str[1028]= { '\0' };

    if (!PyArg_ParseTuple(args, "i", &mask))
        return NULL;
 
    if ((mask>0) && (mask<MAX_ALTTAGS)) {
    	while (bit) {
	    val = mask & (bit=bit>>1); 
	    j--;
	    if (val) {
	        strcat(str,bh->alttagdic[j]);
	        strcat(str,":");
	    }
	}
        return (Py_BuildValue("s", str));
    } else
        return (Py_BuildValue("s", "None"));
}



