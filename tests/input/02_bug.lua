local math = require("math")
local error, type = error, type

local luasimplex = require("luasimplex")
local abs = math.abs

-- Constants -------------------------------------------------------------------

local TOLERANCE: number = 1e-7
local NONBASIC_LOWER: integer = 1
local NONBASIC_UPPER: integer = -1
local NONBASIC_FREE: integer = 2
local BASIC: integer = 0


-- Computation parts -----------------------------------------------------------

local function compute_pi(M: table, I: table)
  -- pi = basic_costs' * Binverse
  local nrows: integer, pi: number[], Bi: number[], TOL: number = M.nrows, I.pi, I.Binverse, I.TOLERANCE
  local basic_costs: number[] = I.basic_costs
  for i = 1, nrows do pi[i] = 0 end
  for i = 1, nrows do
    local c: number = basic_costs[i]
    if abs(c) > TOL then
      for j = 1, nrows do
        pi[j] = pi[j] + c * Bi[(i-1)*nrows + j]
      end
    end
  end
end
