# luaping
The missing ping command for Lua - currently implemented in C for Windows platforms.

## Build for OneLuaPro

```cmd
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
cmake --install . --config Release
```

## API

```lua
local p=require "luaping"

local result, errmsg = p.ping("xxx.xxx.xxx.xxx")	-- add your IP4-address
-- result = true, if address is pingable, else false.
-- If result == false, errmsg provides error info, else errmsg = nil

local timeout = p.timeout()		-- returns current timeout in milliseconds

local result, errmsg = p.settimeout(50)		-- sets timeout to 50ms
-- result = true, if command succeeds, else fail
-- If result == false, errmsg provides error info, else errmsg = nil
```

## Testbench

```cmd
C:\misc\luaping\test>lua tb.lua
Version info:   luaping 1.0

Testing malformed IP address, must return false + errmsg:
Testing         xxx.xxx.xxx.xxx :       false   Argument is not an IP address.
Testing         12345678        :       false   Call to IcmpSendEcho failed.
Testing         localhost       :       false   Argument is not an IP address.

Testing 127.0.0.1, must return true + nil:
Testing         127.0.0.1       :       true    nil

Testing valid but unreachable IP address, must return false + errmsg:
Note: runtime measurement below 1000ms is not vey exact.
Timeout set to  50      ms.
   Testing      192.168.99.99   :       false   Call to IcmpSendEcho failed.
   Runtime :    63      ms.
Timeout set to  100     ms.
   Testing      192.168.99.99   :       false   Call to IcmpSendEcho failed.
   Runtime :    499     ms.
Timeout set to  1000    ms.
   Testing      192.168.99.99   :       false   Call to IcmpSendEcho failed.
   Runtime :    999     ms.
Timeout set to  5000    ms.
   Testing      192.168.99.99   :       false   Call to IcmpSendEcho failed.
   Runtime :    5000    ms.
Timeout set to  10000   ms.
   Testing      192.168.99.99   :       false   Call to IcmpSendEcho failed.
   Runtime :    10000   ms.

End of testbench.

C:\misc\luaping\test>
```

