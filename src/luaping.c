/*
MIT License

Copyright (c) 2024-2026 Kritzel Kratzel.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Code heavily insprired by lsleep library (https://github.com/andrewstarks/lsleep)
*/

#ifdef _WINDLL
#include <winsock2.h>	// https://stackoverflow.com/questions/1372480
#include <windows.h>
#include <ws2tcpip.h>
#include <ipexport.h>
#include <icmpapi.h>
#include <ip2string.h>
#include <time.h>
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define TRUE 1
#define FALSE 0
#define ICPM_MSG_LEN 32
#define LUAPING_VERSION "luaping 1.1"

// Globals
// https://stackoverflow.com/questions/75701
DWORD timeout = 1000;	// in milliseconds

#ifdef _WINDLL
static int luaping_settimeout(lua_State *L) {
  // changes default timeout in milliseconds
  const lua_Integer newTimeout = luaL_checkinteger (L,1);
  if (newTimeout >= 0) {
    timeout = newTimeout;
    lua_pushboolean (L, TRUE);
    lua_pushnil(L);	// empty errmsg
  }
  else {
    lua_pushboolean (L, FALSE);
    lua_pushstring(L, "Timeout must be positive.");
  }
  return 2;
}

static int luaping_timeout(lua_State *L) {
  // gets current timeout in milliseconds
  lua_pushinteger (L, (lua_Integer)timeout);
  return 1;
}

static const char* errorMnemonic(ULONG status){
  // Error messages corresponding to
  // https://learn.microsoft.com/en-us/windows/win32/api/ipexport/ns-ipexport-icmp_echo_reply
  const char *errmsg;
  switch(status) {
  case IP_SUCCESS :
    errmsg = "";
    break;
  case IP_BUF_TOO_SMALL :
    errmsg = "The reply buffer was too small.";
    break;
  case IP_DEST_NET_UNREACHABLE :
    errmsg = "The destination network was unreachable.";
    break;
  case IP_DEST_HOST_UNREACHABLE :
    errmsg = "The destination host was unreachable.";
    break;
  case IP_DEST_PROT_UNREACHABLE :
    errmsg = "The destination protocol was unreachable.";
    break;
  case IP_DEST_PORT_UNREACHABLE :
    errmsg = "The destination port was unreachable.";
    break;
  case IP_NO_RESOURCES :
    errmsg = "Insufficient IP resources were available.";
    break;
  case IP_BAD_OPTION :
    errmsg = "A bad IP option was specified.";
    break;
  case IP_HW_ERROR :
    errmsg = "A hardware error occurred.";
    break;
  case IP_PACKET_TOO_BIG :
    errmsg = "The packet was too big.";
    break;
  case IP_REQ_TIMED_OUT :
    errmsg = "The request timed out.";
    break;
  case IP_BAD_REQ :
    errmsg = "A bad request.";
    break;
  case IP_BAD_ROUTE :
    errmsg = "A bad route.";
    break;
  case IP_TTL_EXPIRED_TRANSIT :
    errmsg = "The time to live (TTL) expired in transit.";
    break;
  case IP_TTL_EXPIRED_REASSEM :
    errmsg = "The time to live expired during fragment reassembly.";
    break;
  case IP_PARAM_PROBLEM :
    errmsg = "A parameter problem.";
    break;
  case IP_SOURCE_QUENCH :
    errmsg = "Datagrams are arriving too fast to be processed and datagrams may have been discarded.";
    break;
  case IP_OPTION_TOO_BIG :
    errmsg = "An IP option was too big.";
    break;
  case IP_BAD_DESTINATION :
    errmsg = "A bad destination.";
    break;
  case IP_GENERAL_FAILURE :
    errmsg = "A general failure. This error can be returned for some malformed ICMP packets.";
    break;
  default :
    errmsg = "An unspecified error occurred.";
  }
  return errmsg;
}

static int luaping_ping(lua_State *L) {
  // This implementation is based on the code provided at:
  // https://learn.microsoft.com/de-de/windows/win32/api/icmpapi/nf-icmpapi-icmpsendecho
  DWORD myTimeout = 0;
  const char *ip = NULL;

  // Check and act according to number of arguments
  if (lua_gettop(L) == 1) {
    // signature 1 - only one arg (the ip-address)
    ip = luaL_checkstring(L,1);
    // use global timeout value
    myTimeout = timeout;
  }
  else if (lua_gettop(L) == 2) {
    // signature 2 - ip-address and inidividual timeout
    ip = luaL_checkstring(L,1);
    // use individual timeout value
    myTimeout= (DWORD)luaL_checkinteger(L,2);
  }
  else {
    // bailing out
    return luaL_error(L,"Wrong number of arguments.");
  }

  // try to identify IPv4 or IPv6 address numbers
  // unsigned long ipaddr = inet_addr(ip);
  struct in_addr addr4;
  int is_ipv4 = inet_pton(AF_INET, ip, &addr4);
  unsigned long ipaddr = (is_ipv4 == 1) ? addr4.S_un.S_addr : INADDR_NONE;
  PCSTR term;
  IN6_ADDR ip6addr, ip6srcaddr;
  NTSTATUS ip6stat = RtlIpv6StringToAddressA(ip, &term, &ip6addr);

  // generate ICPM_MSG_LEN bytes of random ICPM data
  char SendData[ICPM_MSG_LEN];
  srand(time(NULL));
  for (int i=0; i<ICPM_MSG_LEN; i++) {
    SendData[i] = (char)rand()%256;
  }

  if (ipaddr != INADDR_NONE && ip6stat != STATUS_SUCCESS) {
    // got valid IPv4 address

    HANDLE hIcmpFile = IcmpCreateFile();
    if (hIcmpFile == INVALID_HANDLE_VALUE) {
      lua_pushboolean (L, FALSE);
      lua_pushstring(L, "Unable to open ICMP handle.");
      return 2;
    }

    DWORD ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
    LPVOID ReplyBuffer = (VOID*) malloc(ReplySize);
    if (ReplyBuffer == NULL) {
      lua_pushboolean (L, FALSE);
      lua_pushstring(L, "Unable to allocate ICMP memory.");
      return 2;
    }   

    DWORD dwRetVal = IcmpSendEcho(hIcmpFile, ipaddr, SendData, sizeof(SendData), 
				  NULL, ReplyBuffer, ReplySize, myTimeout);

    if (dwRetVal != 0) {
      PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY) ReplyBuffer;
      const char *errmsg = errorMnemonic(pEchoReply->Status);
      if (strcmp(errmsg,"") == 0) {
	// no error, ping successful
	lua_pushboolean (L, TRUE);
	lua_pushnil(L);	// empty errmsg
      }
      else {
	// some error occured
	lua_pushboolean (L, FALSE);
	lua_pushstring(L, errmsg);
      }
    }
    else {
      lua_pushboolean (L, FALSE);
      lua_pushstring(L, "No IPv4 ping echo received.");
    }
    free(ReplyBuffer);
    IcmpCloseHandle(hIcmpFile);
    return 2;
  }
  else if (ipaddr == INADDR_NONE && ip6stat == STATUS_SUCCESS) {
    // got valid IPv6 address

    HANDLE hIcmp6File = Icmp6CreateFile();
    if (hIcmp6File == INVALID_HANDLE_VALUE) {
      lua_pushboolean (L, FALSE);
      lua_pushstring(L, "Unable to open ICMP6 handle.");
      return 2;
    }

    DWORD ReplySize = sizeof(ICMPV6_ECHO_REPLY) + sizeof(SendData) + 8;
    LPVOID ReplyBuffer = (VOID*) malloc(ReplySize);
    if (ReplyBuffer == NULL) {
      lua_pushboolean (L, FALSE);
      lua_pushstring(L, "Unable to allocate ICMPV6 memory.");
      return 2;
    }   

    // There is no Icmp6SendEcho() function in analogy to IcmpSendEcho() for IPv4.
    // Closest neighbour is Icmp6SendEcho2() which requires an explicit source address;
    // we simply use IPv6 localhost as source address.
    RtlIpv6StringToAddressA("::1", &term, &ip6srcaddr);
    SOCKADDR_IN6 srcAddr;
    memset(&srcAddr, 0, sizeof(srcAddr));
    srcAddr.sin6_family = AF_INET6;
    srcAddr.sin6_addr = ip6srcaddr;

    // Fill IPv6 target address
    SOCKADDR_IN6 destAddr;
    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin6_family = AF_INET6;
    destAddr.sin6_addr = ip6addr;

    DWORD dwRetVal = Icmp6SendEcho2(hIcmp6File, NULL, NULL, NULL,
				    (struct sockaddr_in6 *)&srcAddr,
				    (struct sockaddr_in6 *)&destAddr,
				    SendData, sizeof(SendData), 
				    NULL, ReplyBuffer, ReplySize, myTimeout);

    if (dwRetVal != 0) {
      PICMPV6_ECHO_REPLY pV6EchoReply = (PICMPV6_ECHO_REPLY) ReplyBuffer;
      const char *errmsg = errorMnemonic(pV6EchoReply->Status);
      if (strcmp(errmsg,"") == 0) {
	// no error, ping successful
	lua_pushboolean (L, TRUE);
	lua_pushnil(L);	// empty errmsg
      }
      else {
	// some error occured
	lua_pushboolean (L, FALSE);
	lua_pushstring(L, errmsg);
      }
    }
    else {
      lua_pushboolean (L, FALSE);
      lua_pushstring(L, "No IPv6 ping echo received.");
    }
    free(ReplyBuffer);
    IcmpCloseHandle(hIcmp6File);
    return 2;    
  }
  else {
    // Here we assume we got a hostname
    // https://learn.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-getaddrinfo

    WSADATA wsaData;
    struct addrinfo hints;
    struct addrinfo *result = NULL;

    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
      lua_pushboolean (L, FALSE);
      lua_pushstring(L, "WSAStartup() failed with an error.");
      return 2;
    }
    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    
    getaddrinfo(ip, NULL, &hints, &result);

    if (result != NULL){
      switch (result->ai_family) {
      case AF_INET:
	{
	  // IPv4 address from hostname
	  struct sockaddr_in *sockaddr_ipv4;
	  sockaddr_ipv4 = (struct sockaddr_in *) result->ai_addr;
	  char ip_buf[INET_ADDRSTRLEN];
	  if (inet_ntop(AF_INET, &(sockaddr_ipv4->sin_addr), ip_buf, sizeof(ip_buf))) {
	    lua_settop(L, 0);    // clear stack
	    lua_pushstring(L, ip_buf);
	    lua_pushinteger(L, (lua_Integer)myTimeout);
	    return luaping_ping(L);
	  }
	  else {
	    return luaL_error(L, "failed to convert IPv4 address");
	  }
	  break;
	}
      case AF_INET6:
	{
	  // IPv6 address from hostname
	  // LPSOCKADDR sockaddr_ipv6;
	  struct sockaddr_in6 *sockaddr_ipv6;
	  sockaddr_ipv6 = (struct sockaddr_in6 *) result->ai_addr;
	  char ip_buf[INET6_ADDRSTRLEN];
	  if (inet_ntop(AF_INET6, &(sockaddr_ipv6->sin6_addr), ip_buf, sizeof(ip_buf))) {
	    // put address on stack and recursively call luaping_ping(L) again
	    lua_settop(L, 0);	// clear stack
	    lua_pushstring(L, ip_buf);
	    lua_pushinteger(L, (lua_Integer)myTimeout);
	    return luaping_ping(L);
	  }
	  else {
	    lua_pushboolean(L, FALSE);
	    lua_pushstring(L, "inet_ntop (IPv6) failed with an error.");
	    return 2;
	  }
	  break;
	}
      }
    }
    freeaddrinfo(result);
    WSACleanup();

    // from here on not a valid hostname or not a valif IP address
    lua_pushboolean (L, FALSE);
    lua_pushstring(L, "No valid hostname or IPv4/IPv6 address.");
    return 2;
  }
}
#else
// FIXME - non-_WINDLL not yet implemented
#endif

static const struct luaL_Reg luaping_metamethods [] = {
  {"__call", luaping_ping},
  {"__call", luaping_timeout},
  {"__call", luaping_settimeout},
  {NULL, NULL}
};

static const struct luaL_Reg luaping_funcs [] = {
  {"ping", luaping_ping},
  {"timeout", luaping_timeout},
  {"settimeout", luaping_settimeout},
  {NULL, NULL}
};

LUALIB_API int luaopen_luaping(lua_State *L){
  luaL_newlib(L, luaping_funcs);
  luaL_newlib(L, luaping_metamethods);
  lua_setmetatable(L, -2);
  lua_pushliteral(L,LUAPING_VERSION);
  lua_setfield(L,-2,"_VERSION");
  return 1;
}
