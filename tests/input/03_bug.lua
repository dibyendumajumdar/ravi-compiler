-- #64
local t = { name_ = "ravi" }
function t:name(optional)
return self.name_, optional
end