local utils = require('utils')

function var_dump(data, max_level, prefix)  
    if type(prefix) ~= "string" then  
        prefix = "" 
    end  
    if type(data) ~= "table" then  
        print(prefix .. tostring(data))  
    else 
        print(data)  
        if max_level ~= 0 then  
            local prefix_next = prefix .. "    " 
            print(prefix .. "{")  
            for k,v in pairs(data) do 
                local tagK = type(k) == "userdata" and "ud" or k
                io.stdout:write(prefix_next .. tagK .. " = ")  
                if type(v) ~= "table" or (type(max_level) == "number" and max_level <= 1) then  
                    print(v)  
                else 
                    if max_level == nil then  
                        var_dump(v, 5, prefix_next)  
                    else 
                        var_dump(v, max_level - 1, prefix_next)  
                    end  
                end  
            end  
            print(prefix .. "}")  
        end  
    end  
end  

local calls, total, this, inout = {}, {}, {}, {}
debug.sethook(function(event)
    local i = debug.getinfo(2, "Sln")
    if i.what ~= 'Lua' then return end
    local func = ''
    if i.name then
        func = i.short_src .. ':' .. i.linedefined .. ':' .. i.name
    else
        func = i.source .. ':' .. i.linedefined
    end
    if event == 'call' then
        this[func] = os.clock()
        inout[func] = (inout[func] or 0) + 1
    else
        if this[func] then
            local time = os.clock() - this[func]
            total[func] = (total[func] or 0) + time
            calls[func] = (calls[func] or 0) + 1
        end
        if inout[func] then
            inout[func] = inout[func] - 1
        end
    end
end, "cr")
debug.sethook()

function debugHook()
    utils.log("################### debug hook ####################")
    for f, time in pairs(total) do
        local avg = time / calls[f]
        -- if avg > 0.001 or inout[f] ~= 0 then
        if inout[f] ~= 0 then
            utils.log(("Function %s took %.3f seconds after %d calls, avg %.3f, inout %d"):format(f, time, calls[f], avg, inout[f]))
        end
    end
    utils.log("###################################################")
end

return true
