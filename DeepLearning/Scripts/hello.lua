function PRE()
   return 0
end

function EPOCH(index)
   return 0
end

function POST()
   return 0
end

function TEST()
   SetSymbol("AAPL")
   return GetHigh()[0]
end