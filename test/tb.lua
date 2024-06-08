-- luaping test bench
local lp = require "luaping"
local socket = require "socket"

print("Version info: ",lp._VERSION)
print()

print("Testing malformed IP address, must return false + errmsg:")
local stuff0 = {"xxx.xxx.xxx.xxx",12345678,"localhost"}
for i,v in ipairs(stuff0) do
   print("Testing ",v,":", lp.ping(v))
end
print()

print("Testing 127.0.0.1, must return true + nil:")
local lh="127.0.0.1"
print("Testing ",lh,":", lp.ping(lh))
print()

print("Testing valid but unreachable IP address, must return false + errmsg:")
print("Note: runtime measurement below 1000ms is not vey exact.")
local ur = "192.168.99.99" 	-- may vary
local timeouts = {50, 100, 1000, 5000, 10000}	-- in milliseconds
for _,v in ipairs(timeouts) do
   lp.settimeout(v)
   print("Timeout set to",lp.timeout(),"ms.")
   local startTime = socket.gettime()*1000        -- milliseconds
   print("   Testing ",ur,":", lp.ping(ur))
   local stopTime = socket.gettime()*1000        -- milliseconds
   print("   Runtime :",math.floor(stopTime-startTime),"ms.")
end
print()

print("End of testbench.")
os.exit(0)
