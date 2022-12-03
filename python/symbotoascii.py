

symbol_ls = ["AAPL", "AMZN", "GOOG",  "INTC", "MSFT", 
			 "SPY", "TSLA", "NVDA", "AMD", "QCOM"]

def bin(num: int):
	if num>1:
		return bin(num//2)+str(num%2)
	else: 
		return str(num)
	
def bin2dec(bin: str):
	dec = 0
	for i in bin:
		dec = dec*2 + int(i)
	return dec

def convert_string_ascii(symbol: str):
	size = 64
	dec = 0
	for c in symbol:
		dec = dec*256 + ord(c)

	pad = 8-len(symbol)
	if pad > 0:
		for _ in range(pad):
			dec = dec*256 + ord(' ')
	
	bincode = bin(dec)
	return dec, hex(dec), bin2dec(bincode[::-1]+''.join(['0']*(64-len(bincode))))

if __name__ == "__main__":
	converted = list(map(convert_string_ascii, symbol_ls))
	hexs = list(zip(*converted))[1]

	for tok in list(zip(symbol_ls, converted)):
		print(f"{tok[0]}: {tok[1]} ")
	print(f"for C array init: {', '.join(hexs)}")
