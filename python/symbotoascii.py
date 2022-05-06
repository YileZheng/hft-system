

symbol_ls = ["AAPL", "AMZN", "GOOG",  "INTC", "MSFT", 
			 "SPY", "TSLA", "NVDA", "AMD", "QCOM"]

def convert_string_ascii(symbol: str):
	dec = 0
	for c in symbol:
		dec = dec*256 + ord(c)

	pad = 8-len(symbol)
	if pad > 0:
		for _ in range(pad):
			dec = dec*256 + ord(' ')
	
	return dec, hex(dec)

if __name__ == "__main__":
	converted = list(map(convert_string_ascii, symbol_ls))
	hexs = list(zip(*converted))[1]

	for tok in list(zip(symbol_ls, converted)):
		print(f"{tok[0]}: {tok[1]} ")
	print(f"for C array init: {', '.join(hexs)}")