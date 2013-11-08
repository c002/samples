# This file was created automatically by SWIG.
# Don't modify this file, modify the SWIG interface instead.
# This file is compatible with both classic and new-style classes.
import _bdf_api
def _swig_setattr(self,class_type,name,value):
    if (name == "this"):
        if isinstance(value, class_type):
            self.__dict__[name] = value.this
            if hasattr(value,"thisown"): self.__dict__["thisown"] = value.thisown
            del value.thisown
            return
    method = class_type.__swig_setmethods__.get(name,None)
    if method: return method(self,value)
    self.__dict__[name] = value

def _swig_getattr(self,class_type,name):
    method = class_type.__swig_getmethods__.get(name,None)
    if method: return method(self)
    raise AttributeError,name

import types
try:
    _object = types.ObjectType
    _newclass = 1
except AttributeError:
    class _object : pass
    _newclass = 0


api_version = _bdf_api.api_version
BDF_FILE_MAX_TAG_LENGTH = _bdf_api.BDF_FILE_MAX_TAG_LENGTH
BDF_FILE_MAX_ALTTAG_LENGTH = _bdf_api.BDF_FILE_MAX_ALTTAG_LENGTH
MAX_TAGS = _bdf_api.MAX_TAGS
BLOCKSIZE = _bdf_api.BLOCKSIZE
class bdf_file_header(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, bdf_file_header, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, bdf_file_header, name)
    __swig_setmethods__["type"] = _bdf_api.bdf_file_header_type_set
    __swig_getmethods__["type"] = _bdf_api.bdf_file_header_type_get
    if _newclass:type = property(_bdf_api.bdf_file_header_type_get,_bdf_api.bdf_file_header_type_set)
    __swig_setmethods__["version"] = _bdf_api.bdf_file_header_version_set
    __swig_getmethods__["version"] = _bdf_api.bdf_file_header_version_get
    if _newclass:version = property(_bdf_api.bdf_file_header_version_get,_bdf_api.bdf_file_header_version_set)
    __swig_setmethods__["file_size"] = _bdf_api.bdf_file_header_file_size_set
    __swig_getmethods__["file_size"] = _bdf_api.bdf_file_header_file_size_get
    if _newclass:file_size = property(_bdf_api.bdf_file_header_file_size_get,_bdf_api.bdf_file_header_file_size_set)
    __swig_setmethods__["collector_ip"] = _bdf_api.bdf_file_header_collector_ip_set
    __swig_getmethods__["collector_ip"] = _bdf_api.bdf_file_header_collector_ip_get
    if _newclass:collector_ip = property(_bdf_api.bdf_file_header_collector_ip_get,_bdf_api.bdf_file_header_collector_ip_set)
    __swig_setmethods__["checkpoint_time"] = _bdf_api.bdf_file_header_checkpoint_time_set
    __swig_getmethods__["checkpoint_time"] = _bdf_api.bdf_file_header_checkpoint_time_get
    if _newclass:checkpoint_time = property(_bdf_api.bdf_file_header_checkpoint_time_get,_bdf_api.bdf_file_header_checkpoint_time_set)
    __swig_setmethods__["time_start"] = _bdf_api.bdf_file_header_time_start_set
    __swig_getmethods__["time_start"] = _bdf_api.bdf_file_header_time_start_get
    if _newclass:time_start = property(_bdf_api.bdf_file_header_time_start_get,_bdf_api.bdf_file_header_time_start_set)
    __swig_setmethods__["time_end"] = _bdf_api.bdf_file_header_time_end_set
    __swig_getmethods__["time_end"] = _bdf_api.bdf_file_header_time_end_get
    if _newclass:time_end = property(_bdf_api.bdf_file_header_time_end_get,_bdf_api.bdf_file_header_time_end_set)
    __swig_setmethods__["rows"] = _bdf_api.bdf_file_header_rows_set
    __swig_getmethods__["rows"] = _bdf_api.bdf_file_header_rows_get
    if _newclass:rows = property(_bdf_api.bdf_file_header_rows_get,_bdf_api.bdf_file_header_rows_set)
    def __init__(self,*args):
        _swig_setattr(self, bdf_file_header, 'this', apply(_bdf_api.new_bdf_file_header,args))
        _swig_setattr(self, bdf_file_header, 'thisown', 1)
    def __del__(self, destroy= _bdf_api.delete_bdf_file_header):
        try:
            if self.thisown: destroy(self)
        except: pass
    def __repr__(self):
        return "<C bdf_file_header instance at %s>" % (self.this,)

class bdf_file_headerPtr(bdf_file_header):
    def __init__(self,this):
        _swig_setattr(self, bdf_file_header, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, bdf_file_header, 'thisown', 0)
        _swig_setattr(self, bdf_file_header,self.__class__,bdf_file_header)
_bdf_api.bdf_file_header_swigregister(bdf_file_headerPtr)

class bdf_file_tag_header(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, bdf_file_tag_header, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, bdf_file_tag_header, name)
    __swig_setmethods__["type"] = _bdf_api.bdf_file_tag_header_type_set
    __swig_getmethods__["type"] = _bdf_api.bdf_file_tag_header_type_get
    if _newclass:type = property(_bdf_api.bdf_file_tag_header_type_get,_bdf_api.bdf_file_tag_header_type_set)
    __swig_setmethods__["rows"] = _bdf_api.bdf_file_tag_header_rows_set
    __swig_getmethods__["rows"] = _bdf_api.bdf_file_tag_header_rows_get
    if _newclass:rows = property(_bdf_api.bdf_file_tag_header_rows_get,_bdf_api.bdf_file_tag_header_rows_set)
    def __init__(self,*args):
        _swig_setattr(self, bdf_file_tag_header, 'this', apply(_bdf_api.new_bdf_file_tag_header,args))
        _swig_setattr(self, bdf_file_tag_header, 'thisown', 1)
    def __del__(self, destroy= _bdf_api.delete_bdf_file_tag_header):
        try:
            if self.thisown: destroy(self)
        except: pass
    def __repr__(self):
        return "<C bdf_file_tag_header instance at %s>" % (self.this,)

class bdf_file_tag_headerPtr(bdf_file_tag_header):
    def __init__(self,this):
        _swig_setattr(self, bdf_file_tag_header, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, bdf_file_tag_header, 'thisown', 0)
        _swig_setattr(self, bdf_file_tag_header,self.__class__,bdf_file_tag_header)
_bdf_api.bdf_file_tag_header_swigregister(bdf_file_tag_headerPtr)

class bdf_file_alttag_header(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, bdf_file_alttag_header, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, bdf_file_alttag_header, name)
    __swig_setmethods__["type"] = _bdf_api.bdf_file_alttag_header_type_set
    __swig_getmethods__["type"] = _bdf_api.bdf_file_alttag_header_type_get
    if _newclass:type = property(_bdf_api.bdf_file_alttag_header_type_get,_bdf_api.bdf_file_alttag_header_type_set)
    __swig_setmethods__["rows"] = _bdf_api.bdf_file_alttag_header_rows_set
    __swig_getmethods__["rows"] = _bdf_api.bdf_file_alttag_header_rows_get
    if _newclass:rows = property(_bdf_api.bdf_file_alttag_header_rows_get,_bdf_api.bdf_file_alttag_header_rows_set)
    def __init__(self,*args):
        _swig_setattr(self, bdf_file_alttag_header, 'this', apply(_bdf_api.new_bdf_file_alttag_header,args))
        _swig_setattr(self, bdf_file_alttag_header, 'thisown', 1)
    def __del__(self, destroy= _bdf_api.delete_bdf_file_alttag_header):
        try:
            if self.thisown: destroy(self)
        except: pass
    def __repr__(self):
        return "<C bdf_file_alttag_header instance at %s>" % (self.this,)

class bdf_file_alttag_headerPtr(bdf_file_alttag_header):
    def __init__(self,this):
        _swig_setattr(self, bdf_file_alttag_header, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, bdf_file_alttag_header, 'thisown', 0)
        _swig_setattr(self, bdf_file_alttag_header,self.__class__,bdf_file_alttag_header)
_bdf_api.bdf_file_alttag_header_swigregister(bdf_file_alttag_headerPtr)

class bdf_file_alttag(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, bdf_file_alttag, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, bdf_file_alttag, name)
    __swig_setmethods__["alttag"] = _bdf_api.bdf_file_alttag_alttag_set
    __swig_getmethods__["alttag"] = _bdf_api.bdf_file_alttag_alttag_get
    if _newclass:alttag = property(_bdf_api.bdf_file_alttag_alttag_get,_bdf_api.bdf_file_alttag_alttag_set)
    __swig_setmethods__["id"] = _bdf_api.bdf_file_alttag_id_set
    __swig_getmethods__["id"] = _bdf_api.bdf_file_alttag_id_get
    if _newclass:id = property(_bdf_api.bdf_file_alttag_id_get,_bdf_api.bdf_file_alttag_id_set)
    def __init__(self,*args):
        _swig_setattr(self, bdf_file_alttag, 'this', apply(_bdf_api.new_bdf_file_alttag,args))
        _swig_setattr(self, bdf_file_alttag, 'thisown', 1)
    def __del__(self, destroy= _bdf_api.delete_bdf_file_alttag):
        try:
            if self.thisown: destroy(self)
        except: pass
    def __repr__(self):
        return "<C bdf_file_alttag instance at %s>" % (self.this,)

class bdf_file_alttagPtr(bdf_file_alttag):
    def __init__(self,this):
        _swig_setattr(self, bdf_file_alttag, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, bdf_file_alttag, 'thisown', 0)
        _swig_setattr(self, bdf_file_alttag,self.__class__,bdf_file_alttag)
_bdf_api.bdf_file_alttag_swigregister(bdf_file_alttagPtr)

class bdf_file_data(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, bdf_file_data, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, bdf_file_data, name)
    __swig_setmethods__["ip_addr"] = _bdf_api.bdf_file_data_ip_addr_set
    __swig_getmethods__["ip_addr"] = _bdf_api.bdf_file_data_ip_addr_get
    if _newclass:ip_addr = property(_bdf_api.bdf_file_data_ip_addr_get,_bdf_api.bdf_file_data_ip_addr_set)
    __swig_setmethods__["ip_proxy"] = _bdf_api.bdf_file_data_ip_proxy_set
    __swig_getmethods__["ip_proxy"] = _bdf_api.bdf_file_data_ip_proxy_get
    if _newclass:ip_proxy = property(_bdf_api.bdf_file_data_ip_proxy_get,_bdf_api.bdf_file_data_ip_proxy_set)
    __swig_setmethods__["bytes_to"] = _bdf_api.bdf_file_data_bytes_to_set
    __swig_getmethods__["bytes_to"] = _bdf_api.bdf_file_data_bytes_to_get
    if _newclass:bytes_to = property(_bdf_api.bdf_file_data_bytes_to_get,_bdf_api.bdf_file_data_bytes_to_set)
    __swig_setmethods__["bytes_from"] = _bdf_api.bdf_file_data_bytes_from_set
    __swig_getmethods__["bytes_from"] = _bdf_api.bdf_file_data_bytes_from_get
    if _newclass:bytes_from = property(_bdf_api.bdf_file_data_bytes_from_get,_bdf_api.bdf_file_data_bytes_from_set)
    __swig_setmethods__["tag_id"] = _bdf_api.bdf_file_data_tag_id_set
    __swig_getmethods__["tag_id"] = _bdf_api.bdf_file_data_tag_id_get
    if _newclass:tag_id = property(_bdf_api.bdf_file_data_tag_id_get,_bdf_api.bdf_file_data_tag_id_set)
    __swig_setmethods__["aas"] = _bdf_api.bdf_file_data_aas_set
    __swig_getmethods__["aas"] = _bdf_api.bdf_file_data_aas_get
    if _newclass:aas = property(_bdf_api.bdf_file_data_aas_get,_bdf_api.bdf_file_data_aas_set)
    __swig_setmethods__["alt_tags"] = _bdf_api.bdf_file_data_alt_tags_set
    __swig_getmethods__["alt_tags"] = _bdf_api.bdf_file_data_alt_tags_get
    if _newclass:alt_tags = property(_bdf_api.bdf_file_data_alt_tags_get,_bdf_api.bdf_file_data_alt_tags_set)
    def __init__(self,*args):
        _swig_setattr(self, bdf_file_data, 'this', apply(_bdf_api.new_bdf_file_data,args))
        _swig_setattr(self, bdf_file_data, 'thisown', 1)
    def __del__(self, destroy= _bdf_api.delete_bdf_file_data):
        try:
            if self.thisown: destroy(self)
        except: pass
    def __repr__(self):
        return "<C bdf_file_data instance at %s>" % (self.this,)

class bdf_file_dataPtr(bdf_file_data):
    def __init__(self,this):
        _swig_setattr(self, bdf_file_data, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, bdf_file_data, 'thisown', 0)
        _swig_setattr(self, bdf_file_data,self.__class__,bdf_file_data)
_bdf_api.bdf_file_data_swigregister(bdf_file_dataPtr)

class bdf_file_block(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, bdf_file_block, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, bdf_file_block, name)
    __swig_setmethods__["block"] = _bdf_api.bdf_file_block_block_set
    __swig_getmethods__["block"] = _bdf_api.bdf_file_block_block_get
    if _newclass:block = property(_bdf_api.bdf_file_block_block_get,_bdf_api.bdf_file_block_block_set)
    def __init__(self,*args):
        _swig_setattr(self, bdf_file_block, 'this', apply(_bdf_api.new_bdf_file_block,args))
        _swig_setattr(self, bdf_file_block, 'thisown', 1)
    def __del__(self, destroy= _bdf_api.delete_bdf_file_block):
        try:
            if self.thisown: destroy(self)
        except: pass
    def __repr__(self):
        return "<C bdf_file_block instance at %s>" % (self.this,)

class bdf_file_blockPtr(bdf_file_block):
    def __init__(self,this):
        _swig_setattr(self, bdf_file_block, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, bdf_file_block, 'thisown', 0)
        _swig_setattr(self, bdf_file_block,self.__class__,bdf_file_block)
_bdf_api.bdf_file_block_swigregister(bdf_file_blockPtr)

class bdf_headers(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, bdf_headers, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, bdf_headers, name)
    __swig_setmethods__["head"] = _bdf_api.bdf_headers_head_set
    __swig_getmethods__["head"] = _bdf_api.bdf_headers_head_get
    if _newclass:head = property(_bdf_api.bdf_headers_head_get,_bdf_api.bdf_headers_head_set)
    __swig_setmethods__["taghead"] = _bdf_api.bdf_headers_taghead_set
    __swig_getmethods__["taghead"] = _bdf_api.bdf_headers_taghead_get
    if _newclass:taghead = property(_bdf_api.bdf_headers_taghead_get,_bdf_api.bdf_headers_taghead_set)
    __swig_setmethods__["alttaghead"] = _bdf_api.bdf_headers_alttaghead_set
    __swig_getmethods__["alttaghead"] = _bdf_api.bdf_headers_alttaghead_get
    if _newclass:alttaghead = property(_bdf_api.bdf_headers_alttaghead_get,_bdf_api.bdf_headers_alttaghead_set)
    __swig_setmethods__["alttag"] = _bdf_api.bdf_headers_alttag_set
    __swig_getmethods__["alttag"] = _bdf_api.bdf_headers_alttag_get
    if _newclass:alttag = property(_bdf_api.bdf_headers_alttag_get,_bdf_api.bdf_headers_alttag_set)
    __swig_setmethods__["tagdic"] = _bdf_api.bdf_headers_tagdic_set
    __swig_getmethods__["tagdic"] = _bdf_api.bdf_headers_tagdic_get
    if _newclass:tagdic = property(_bdf_api.bdf_headers_tagdic_get,_bdf_api.bdf_headers_tagdic_set)
    __swig_setmethods__["alttagdic"] = _bdf_api.bdf_headers_alttagdic_set
    __swig_getmethods__["alttagdic"] = _bdf_api.bdf_headers_alttagdic_get
    if _newclass:alttagdic = property(_bdf_api.bdf_headers_alttagdic_get,_bdf_api.bdf_headers_alttagdic_set)
    def __init__(self,*args):
        _swig_setattr(self, bdf_headers, 'this', apply(_bdf_api.new_bdf_headers,args))
        _swig_setattr(self, bdf_headers, 'thisown', 1)
    def __del__(self, destroy= _bdf_api.delete_bdf_headers):
        try:
            if self.thisown: destroy(self)
        except: pass
    def __repr__(self):
        return "<C bdf_headers instance at %s>" % (self.this,)

class bdf_headersPtr(bdf_headers):
    def __init__(self,this):
        _swig_setattr(self, bdf_headers, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, bdf_headers, 'thisown', 0)
        _swig_setattr(self, bdf_headers,self.__class__,bdf_headers)
_bdf_api.bdf_headers_swigregister(bdf_headersPtr)

class summarydetails(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, summarydetails, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, summarydetails, name)
    __swig_setmethods__["total_to_bytes"] = _bdf_api.summarydetails_total_to_bytes_set
    __swig_getmethods__["total_to_bytes"] = _bdf_api.summarydetails_total_to_bytes_get
    if _newclass:total_to_bytes = property(_bdf_api.summarydetails_total_to_bytes_get,_bdf_api.summarydetails_total_to_bytes_set)
    __swig_setmethods__["total_from_bytes"] = _bdf_api.summarydetails_total_from_bytes_set
    __swig_getmethods__["total_from_bytes"] = _bdf_api.summarydetails_total_from_bytes_get
    if _newclass:total_from_bytes = property(_bdf_api.summarydetails_total_from_bytes_get,_bdf_api.summarydetails_total_from_bytes_set)
    __swig_setmethods__["count"] = _bdf_api.summarydetails_count_set
    __swig_getmethods__["count"] = _bdf_api.summarydetails_count_get
    if _newclass:count = property(_bdf_api.summarydetails_count_get,_bdf_api.summarydetails_count_set)
    def __init__(self,*args):
        _swig_setattr(self, summarydetails, 'this', apply(_bdf_api.new_summarydetails,args))
        _swig_setattr(self, summarydetails, 'thisown', 1)
    def __del__(self, destroy= _bdf_api.delete_summarydetails):
        try:
            if self.thisown: destroy(self)
        except: pass
    def __repr__(self):
        return "<C summarydetails instance at %s>" % (self.this,)

class summarydetailsPtr(summarydetails):
    def __init__(self,this):
        _swig_setattr(self, summarydetails, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, summarydetails, 'thisown', 0)
        _swig_setattr(self, summarydetails,self.__class__,summarydetails)
_bdf_api.summarydetails_swigregister(summarydetailsPtr)
cvar = _bdf_api.cvar

fopen = _bdf_api.fopen

fclose = _bdf_api.fclose

bdf_readhead = _bdf_api.bdf_readhead

bdf_readflow = _bdf_api.bdf_readflow

bdf_readxflow = _bdf_api.bdf_readxflow

bdf_flowcount = _bdf_api.bdf_flowcount

bdf_summary = _bdf_api.bdf_summary

bdf_tagname = _bdf_api.bdf_tagname

bdf_alttagname = _bdf_api.bdf_alttagname

bdf_getdata = _bdf_api.bdf_getdata

bdf_fastread = _bdf_api.bdf_fastread


