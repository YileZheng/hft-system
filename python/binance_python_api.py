from binance.client import Client
from binance import ThreadedDepthCacheManager
from binance import ThreadedWebsocketManager
from binance import AsyncClient, BinanceSocketManager
import time
import asyncio
from datetime import datetime

# url https://python-binance.readthedocs.io/en/latest/websockets.html

api_key = "5fNyacwdzx7QRXfsoPq1kYs6wCNvX2Ijsk3VPmqrcwA72FMQ9d6PqfXXyifyFPaJ"
api_secret = "IVPmECK6pHUsUhSQ6IP5BLtrxUkIz3K6YS1HJEeSeFsCY5Do7CTAkokf2z7Sfc2E"
client = Client(api_key, api_secret, testnet=True)

# Get Market Depth
depth = client.get_order_book(symbol='BNBBTC')
# Get Recent Trades
trades = client.get_recent_trades(symbol='BNBBTC')
# Get Historical Trades
trades = client.get_historical_trades(symbol='BNBBTC')
# Get Aggregate Trades
trades = client.get_aggregate_trades(symbol='BNBBTC')

# Aggregate Trade Iterator
## Iterate over aggregate trades for a symbol from a given date or a given order id.
agg_trades = client.aggregate_trade_iter(symbol='ETHBTC', start_str='30 minutes ago UTC')
for trade in agg_trades:
    print(trade)
    # do something with the trade data
## example using last_id value
agg_trades = client.aggregate_trade_iter(symbol='ETHBTC', last_id=23380478)
agg_trade_list = list(agg_trades)

# Get Kline/Candlesticks
candles = client.get_klines(symbol='BNBBTC', interval=Client.KLINE_INTERVAL_30MINUTE)
# Get Historical Kline/Candlesticks
## Fetch klines for any date range and interval
### fetch 1 minute klines for the last day up until now
klines = client.get_historical_klines("BNBBTC", Client.KLINE_INTERVAL_1MINUTE, "1 day ago UTC")
### fetch 30 minute klines for the last month of 2017
klines = client.get_historical_klines("ETHBTC", Client.KLINE_INTERVAL_30MINUTE, "1 Dec, 2017", "1 Jan, 2018")
### fetch weekly klines since it listed
klines = client.get_historical_klines("NEOBTC", Client.KLINE_INTERVAL_1WEEK, "1 Jan, 2017")

# Get Historical Kline/Candlesticks using a generator
## Fetch klines using a generator
for kline in client.get_historical_klines_generator("BNBBTC", Client.KLINE_INTERVAL_1MINUTE, "1 day ago UTC"):
	print(kline)
	# do something with the kline

# Get average price for a symbol
avg_price = client.get_avg_price(symbol='BNBBTC')
# Get 24hr Ticker
tickers = client.get_ticker()
# Get All Prices
## Get last price for all markets.
prices = client.get_all_tickers()
# Get Orderbook Tickers
## Get first bid and ask entry in the order book for all markets.
tickers = client.get_orderbook_tickers()


def webSocket():

	symbol = 'BNBBTC'

	twm = ThreadedWebsocketManager(api_key=api_key, api_secret=api_secret)
	# start is required to initialise its internal loop
	twm.start()

	def handle_socket_message(msg):
		print(f"message type: {msg['e']}")
		print(msg)

	twm.start_kline_socket(callback=handle_socket_message, symbol=symbol)

	# multiple sockets can be started
	twm.start_depth_socket(callback=handle_socket_message, symbol=symbol)

	# or a multiplex socket can be started like this
	# see Binance docs for stream names
	# streams = ['bnbbtc@miniTicker', 'bnbbtc@bookTicker']
	streams = ['<symbol>@depth<levels>@100ms']
	twm.start_multiplex_socket(callback=handle_socket_message, streams=streams)

	twm.join()

def depthCache():

    dcm = ThreadedDepthCacheManager()
    # start is required to initialise its internal loop
    dcm.start()

    def handle_depth_cache(depth_cache):
        print(f"symbol {depth_cache.symbol}")
        print("top 5 bids")
        print(depth_cache.get_bids()[:5])
        print("top 5 asks")
        print(depth_cache.get_asks()[:5])
        print("last update time {}".format(depth_cache.update_time))

    dcm_name = dcm.start_depth_cache(handle_depth_cache, symbol='BNBBTC')

    # multiple depth caches can be started
    dcm_name = dcm.start_depth_cache(handle_depth_cache, symbol='ETHBTC')

    dcm.join()

	# dcm.stop_socket(dcm_name)	# stop individual stream
	# dcm.stop()   				# stop all

# async
async def main():
	client = await AsyncClient.create()
	bm = BinanceSocketManager(client)
	# start any sockets here, i.e a trade socket
	ts = bm.trade_socket('BNBBTC')
	# pass a list of stream names
	ms = bm.multiplex_socket(['bnbbtc@aggTrade', 'neobtc@ticker'])
	# depth diff response
	ds = bm.depth_socket('BNBBTC')
	# partial book response
	ds = bm.depth_socket('BNBBTC', depth=BinanceSocketManager.WEBSOCKET_DEPTH_5)
	# from binance.enums import *
	# ks = bm.kline_socket('BNBBTC', interval=KLINE_INTERVAL_30MINUTE)

	# then start receiving messages
	async with ts as tscm:
		while True:
			res = await tscm.recv()
			print(res)

	await client.close_connection()

if __name__ == "__main__":

    loop = asyncio.get_event_loop()
    loop.run_until_complete(main())


if __name__ == "__main__":
   pass