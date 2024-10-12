# luaping
The missing ping command for Lua - currently implemented in C for Windows platforms. Supports IPv4, IPv6, hostname resolving, global and individual timeout settings. The ping message length is 32 bytes.

## API

```lua
local p=require "luaping"

-- ping using IPv4 address
local result, errmsg = p.ping("xxx.xxx.xxx.xxx")
-- ping using IPv4 address with individual timeout settings in milliseconds
local result, errmsg = p.ping("xxx.xxx.xxx.xxx",5000)
-- IPv6 ping to Google's public DNS with individual timeout value of 50 ms
local result, errmsg = p.ping("2001:4860:4860::8888",50)
-- ping a host
local result, errmsg = p.ping("dns.google.com")

-- On success:
result = true
errmsg = nil
-- On failure:
result = false
errmsg = "<Detailed error message>"

-- returns current global timeout in milliseconds
local timeout = p.timeout()

-- sets global timeout to 50ms
local result, errmsg = p.settimeout(50)

-- get version info
print(p._VERSION)
```

## License

See https://github.com/OneLuaPro/luaping/blob/main/LICENSE.
