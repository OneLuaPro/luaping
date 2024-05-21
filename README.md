# luaping
The missing ping command for Lua - implemented in C for Windows.

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

