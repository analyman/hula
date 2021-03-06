#include <openssl/ssl.h>    // conflict with uv.h
#include <openssl/crypto.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/utils.h"
#include "./utils_hideuv.h"

/** global logger */
Logger::Logger* logger = new Logger::Logger("kproxy", "./kproxy.log", true);


X509* mem2cert(void* m, size_t len) //{
{
    X509 *cert = NULL;
    BIO* cbio = BIO_new_mem_buf(m, len);
    cert = PEM_read_bio_X509(cbio, NULL, 0, NULL);
    return cert;
} //}
RSA* mem2rsa(void* m, size_t len) //{
{
    RSA *rsa = NULL;
    BIO* kbio = BIO_new_mem_buf(m, len);
    rsa = PEM_read_bio_RSAPrivateKey(kbio, NULL, 0, NULL);
    return rsa;
} //}

static union {uint16_t u16; struct {uint8_t u8; uint8_t xx;};} bytes_endian_test = {.u16 = 0xFFEE};
static bool little_endian() {return bytes_endian_test.u8 == 0xEE;}
struct _ipv4_addr {uint8_t a, b, c, d;};
struct __int_16 {uint8_t a, b;};
static char ipv4str[20];
char* ip4_to_str(uint32_t addr) //{
{
    _ipv4_addr* x = (_ipv4_addr*)&addr;
    sprintf(ipv4str, "%d.%d.%d.%d", x->a, x->b, x->c, x->d);
    return ipv4str;
} //}
bool str_to_ip4(const char* str, uint32_t* out) //{
{
    size_t l = 0;
    uint8_t dot = 0;
    uint8_t dot_p[3];
    while(str[l] != '\0') {
        if(l > 19) return false; // 4 * 4 + 3 = 19
        if(str[l] < '0' || str[l] > '9') {
            if(str[l] != '.') return false;
            if(dot == 3) return false;
            dot_p[dot] = l;
            dot++;
        }
        l++;
    }
    if(dot != 3) return false;

    if(dot_p[0] - 0        > 3 || 
       dot_p[1] - dot_p[0] > 4 || 
       dot_p[2] - dot_p[1] > 4 ||
       strlen(str) - dot_p[2] > 4)
        return false;

    char copyx[20];
    strcpy(copyx, str);
    copyx[dot_p[0]] = 0;
    copyx[dot_p[1]] = 0;
    copyx[dot_p[2]] = 0;
    dot_p[0]++;
    dot_p[1]++;
    dot_p[2]++;

    _ipv4_addr addr;
    addr.a = atoi(copyx + 0);
    addr.b = atoi(copyx + dot_p[0]);
    addr.c = atoi(copyx + dot_p[1]);
    addr.d = atoi(copyx + dot_p[2]);

    *out = *(uint32_t*)&addr;
    return true;
} //}

uint32_t k_htonl(uint32_t v) //{
{
    if(little_endian()) {
        _ipv4_addr vx;
        *(uint32_t*)&vx = v;
        uint8_t t = vx.a;
        vx.a = vx.d; vx.d = t;
        t = vx.b;
        vx.b = vx.c; vx.c = t;
        return *(uint32_t*)&vx;
    } else {
        return v;
    }
} //}
uint32_t k_ntohl(uint32_t v) //{
{
    if(little_endian()) {
        _ipv4_addr vx;
        *(uint32_t*)&vx = v;
        uint8_t t = vx.a;
        vx.a = vx.d; vx.d = t;
        t = vx.b;
        vx.b = vx.c; vx.c = t;
        return *(uint32_t*)&vx;
    } else {
        return v;
    }
} //}
uint16_t k_htons(uint16_t v) //{
{
    if(little_endian()) {
        __int_16 vx;
        *(uint16_t*)&vx = v;
        uint8_t t = vx.a;
        vx.a = vx.b; vx.b = t;
        return *(uint16_t*)&vx;
    } else {
        return v;
    }
} //}
uint16_t k_ntohs(uint16_t v) //{
{
    if(little_endian()) {
        __int_16 vx;
        *(uint16_t*)&vx = v;
        uint8_t t = vx.a;
        vx.a = vx.b; vx.b = t;
        return *(uint16_t*)&vx;
    } else {
        return v;
    }
} //}

int ip4_addr(const char* ip, int port, struct sockaddr_in* addr) {return __ip4_addr(ip, port, addr);}
bool k_inet_ntop(int af, const void* src, char* dst, size_t size) {return __k_inet_ntop(af, src, dst, size);}
bool k_inet_pton(int af, const char* src, void* dst)              {return __k_inet_pton(af, src, dst);}

std::pair<std::string, uint16_t> k_sockaddr_to_str(struct sockaddr* addr) {return __k_sockaddr_to_str(addr);}


std::string time_t_to_UTCString(time_t time) //{
{
    std::stringstream timestr;
    auto tmtime = std::gmtime(&time);
    timestr << std::put_time(tmtime, "%a, %d %b %Y %H:%M:%S %Z");
    return timestr.str();
} //}


const char* Sha1Bin(const char* str, size_t len) //{
{
    static char obuf[20];
    SHA1((const unsigned char*)str, len, (unsigned char*)obuf);
    std::string ret(40, '=');

    return obuf;
} //}
static const char lowercasehex[] = "0123456789abcdef";
std::string Sha1Hex(const char* str, size_t len) //{
{
    auto obuf = Sha1Bin(str, len);
    std::string ret(40, '=');

    for(size_t i=0;i<20;i++) {
        ret[2*i]     = lowercasehex[obuf[i] & 0xf0];
        ret[2*i + 1] = lowercasehex[obuf[i] & 0x0f];
    }

    return ret;
} //}

