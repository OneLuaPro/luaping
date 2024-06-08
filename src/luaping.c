/*
MIT License

Copyright (c) 2024 Kritzel Kratzel.

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
#include <iphlpapi.h>
#include <icmpapi.h>
#define DLL __declspec(dllexport)
#else
// #include <unistd.h>
#define DLL //empty
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define TRUE 1
#define FALSE 0
#define LUAPING_VERSION "luaping 1.0"

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

static int luaping_ping(lua_State *L) {
  // This implementation is based on the code provided at:
  // https://learn.microsoft.com/de-de/windows/win32/api/icmpapi/nf-icmpapi-icmpsendecho
  const char *ip = luaL_checkstring(L,1);

  unsigned long ipaddr = inet_addr(ip);
  if (ipaddr == INADDR_NONE) {
    lua_pushboolean (L, FALSE);
    lua_pushstring(L, "Argument is not an IP address.");
    return 2;
  }

  HANDLE hIcmpFile = IcmpCreateFile();
  if (hIcmpFile == INVALID_HANDLE_VALUE) {
    lua_pushboolean (L, FALSE);
    lua_pushstring(L, "Unable to open ICMP handle.");
    return 2;
  }

  char SendData[32] = "Data Buffer";
  DWORD ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
  LPVOID ReplyBuffer = (VOID*) malloc(ReplySize);
  if (ReplyBuffer == NULL) {
    lua_pushboolean (L, FALSE);
    lua_pushstring(L, "Unable to allocate ICMP memory.");
    return 2;
  }   

  DWORD dwRetVal = IcmpSendEcho(hIcmpFile, ipaddr, SendData, sizeof(SendData), 
				NULL, ReplyBuffer, ReplySize, timeout);

  if (dwRetVal != 0) {
    PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY) ReplyBuffer;
    const char *errmsg;
    int ping_result;

    switch(pEchoReply->Status) {
    case IP_SUCCESS :
      errmsg = "";
      ping_result = TRUE;
      break;
    case IP_BUF_TOO_SMALL :
      errmsg = "The reply buffer was too small.";
      ping_result = FALSE;
      break;
    case IP_DEST_NET_UNREACHABLE :
      errmsg = "The destination network was unreachable.";
      ping_result = FALSE;
      break;
    case IP_DEST_HOST_UNREACHABLE :
      errmsg = "The destination host was unreachable.";
      ping_result = FALSE;
      break;
    case IP_DEST_PROT_UNREACHABLE :
      errmsg = "The destination protocol was unreachable.";
      ping_result = FALSE;
      break;
    case IP_DEST_PORT_UNREACHABLE :
      errmsg = "The destination port was unreachable.";
      ping_result = FALSE;
      break;
    case IP_NO_RESOURCES :
      errmsg = "Insufficient IP resources were available.";
      ping_result = FALSE;
      break;
    case IP_BAD_OPTION :
      errmsg = "A bad IP option was specified.";
      ping_result = FALSE;
      break;
    case IP_HW_ERROR :
      errmsg = "A hardware error occurred.";
      ping_result = FALSE;
      break;
    case IP_PACKET_TOO_BIG :
      errmsg = "The packet was too big.";
      ping_result = FALSE;
      break;
    case IP_REQ_TIMED_OUT :
      errmsg = "The request timed out.";
      ping_result = FALSE;
      break;
    case IP_BAD_REQ :
      errmsg = "A bad request.";
      ping_result = FALSE;
      break;
    case IP_BAD_ROUTE :
      errmsg = "A bad route.";
      ping_result = FALSE;
      break;
    case IP_TTL_EXPIRED_TRANSIT :
      errmsg = "The time to live (TTL) expired in transit.";
      ping_result = FALSE;
      break;
    case IP_TTL_EXPIRED_REASSEM :
      errmsg = "The time to live expired during fragment reassembly.";
      ping_result = FALSE;
      break;
    case IP_PARAM_PROBLEM :
      errmsg = "A parameter problem.";
      ping_result = FALSE;
      break;
    case IP_SOURCE_QUENCH :
      errmsg = "Datagrams are arriving too fast to be processed and datagrams may have been discarded.";
      ping_result = FALSE;
      break;
    case IP_OPTION_TOO_BIG :
      errmsg = "An IP option was too big.";
      ping_result = FALSE;
      break;
    case IP_BAD_DESTINATION :
      errmsg = "A bad destination.";
      ping_result = FALSE;
      break;
    case IP_GENERAL_FAILURE :
      errmsg = "A general failure. This error can be returned for some malformed ICMP packets.";
      ping_result = FALSE;
      break;
    default :
      errmsg = "An unspecified error occurred.";
      ping_result = FALSE;
    }    
    lua_pushboolean (L, ping_result);
    if (errmsg[0] == '\0') {
      lua_pushnil(L);	// empty errmsg
    }
    else {
      lua_pushstring(L, errmsg);
    }
  }
  else {
    lua_pushboolean (L, FALSE);
    lua_pushstring(L, "Call to IcmpSendEcho failed.");
  }
  free(ReplyBuffer);
  return 2;
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

DLL int luaopen_luaping(lua_State *L){
  luaL_newlib(L, luaping_funcs);
  luaL_newlib(L, luaping_metamethods);
  lua_setmetatable(L, -2);
  lua_pushliteral(L,LUAPING_VERSION);
  lua_setfield(L,-2,"_VERSION");
  return 1;
}
