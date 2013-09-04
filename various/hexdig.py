# Return md5 digest as hex ascii string
#
import md5, string
def hexdigest(s):

    md5auth= md5.new(s).digest()

    hexdig = string.join(map(lambda x : string.replace(hex(ord(x)) , '0x',''), md5auth),'')

    return(hexdig)
