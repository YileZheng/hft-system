import asyncio
from mimetypes import init
import time
import traceback
from datetime import datetime
from binance.client import Client
from binance import ThreadedDepthCacheManager
from binance import ThreadedWebsocketManager
from binance import AsyncClient, BinanceSocketManager
from multiprocessing import ProcessError, Queue, Process


class binance_book():
	def __init__(self) -> None:
		api_key = "5fNyacwdzx7QRXfsoPq1kYs6wCNvX2Ijsk3VPmqrcwA72FMQ9d6PqfXXyifyFPaJ"
		api_secret = "IVPmECK6pHUsUhSQ6IP5BLtrxUkIz3K6YS1HJEeSeFsCY5Do7CTAkokf2z7Sfc2E"

		# self.client = Client(api_key, api_secret, testnet=False)
		# print("Client connected.")
		self.symbol='SHIBUSDT'
		self.stat = True
		self.cum_upd = 0

	# Get Market Depth
	def book_construct(self, q:Queue):
		symbol = self.symbol

		depth = q.get()
		self.book_bid = {x[0]:float(x[1]) for x in depth['bids']} 
		self.book_ask = {x[0]:float(x[1]) for x in depth['asks']} 
		
		
		initUpdId = depth['lastUpdateId']
		lstUpdId = depth['lastUpdateId']
		reorder_cache = {}
		f = open(f'data/{symbol}_partial.txt', 'w')
		f.writelines(list(map(lambda x: 'a A '+x[0]+' '+str(x[1])+'\n', self.book_ask.items())))
		f.writelines(list(map(lambda x: 'b A '+x[0]+' '+str(x[1])+'\n', self.book_bid.items())))
		
		try:
			while self.stat: 
				res = q.get()
				if lstUpdId+1 <= res['lastUpdateId']:
					self.update_book(res, f)
					lstUpdId = res['lastUpdateId']
					self.cum_upd = res['lastUpdateId'] - initUpdId
					self.stat = False if self.cum_upd>1000 else True
		except Exception as e:
			print(e)
			track = traceback.format_exc()
			print(track)
		else:
			self.snapshot()
		finally:
			f.close()

	def snapshot(self):
		symbol = self.symbol
		f = open(f'data/{symbol}_partial_a_{self.cum_upd}.txt', 'w')
		f.writelines(list(map(lambda x: x[0]+' '+str(x[1])+'\n', self.book_ask.items())))
		f.close()
		f = open(f'data/{symbol}_partial_b_{self.cum_upd}.txt', 'w')
		f.writelines(list(map(lambda x: x[0]+' '+str(x[1])+'\n', self.book_bid.items())))
		f.close()

	def update_book(self, res, f):
		if len(res['asks']):
			latest = [x[0] for x in res['asks']]
			for price in list(self.book_ask.keys()):
				if price not in latest:
					diff = -self.book_ask.pop(price, None)
					f.write('a R '+price+' '+str(diff)+'\n')
			for price, qty in res['asks']:
				qty = float(qty)
				pre_qty = 0 if self.book_ask.get(price) is None else self.book_ask[price]
				diff = qty - pre_qty
				if diff == 0:
					continue
				elif pre_qty == 0:
					self.book_ask[price] = qty
					f.write('a A '+price+' '+str(diff)+'\n')
				else:
					self.book_ask[price] = qty
					f.write('a E '+price+' '+str(diff)+'\n')
		if len(res['bids']):
			latest = [x[0] for x in res['bids']]
			for price in list(self.book_bid.keys()):
				if price not in latest:
					diff = -self.book_bid.pop(price, None)
					f.write('b R '+price+' '+str(diff)+'\n')
			for price, qty in res['bids']:
				qty = float(qty)
				pre_qty = 0 if self.book_bid.get(price) is None else self.book_bid[price]
				diff = qty - pre_qty
				if diff == 0:
					continue
				elif pre_qty == 0:
					self.book_bid[price] = qty
					f.write('b A '+price+' '+str(diff)+'\n')
				else:
					self.book_bid[price] = qty
					f.write('b E '+price+' '+str(diff)+'\n')

	async def SockerThread(self, q:Queue):
		symbol = self.symbol
		client = await AsyncClient.create()
		bm = BinanceSocketManager(client)
		# ds = bm.depth_socket(symbol)
		ds = bm.multiplex_socket(['bnbbtc@depth5@100ms'])
		# start any sockets here, i.e a trade socket
		# ts = bm.trade_socket('BNBBTC')
		# then start receiving messages
		print("WebSocket connected.")
		async with ds as tscm:
			while self.stat:
				res = await tscm.recv()
				q.put(res['data'])
				print(res['data']['lastUpdateId'])

		await client.close_connection()

	def run_socket(self, q):
		loop = asyncio.get_event_loop()
		loop.run_until_complete(self.SockerThread(q))

	def run_book_manage(self):
		q = Queue()
		p1 = Process(target=self.run_socket, args=(q,))
		p2 = Process(target=self.book_construct, args=(q,))
		p1.start()
		p2.start()
		p2.join()
		p1.kill()


'''
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
	# twm.start_depth_socket(callback=handle_socket_message, symbol=symbol)

	# or a multiplex socket can be started like this
	# see Binance docs for stream names
	# streams = ['bnbbtc@miniTicker', 'bnbbtc@bookTicker']
	# streams = ['BNBBTC@depth20@100ms']
	# twm.start_multiplex_socket(callback=handle_socket_message, streams=streams)

	twm.join()

def depthCache():

    dcm = ThreadedDepthCacheManager(api_key=api_key, api_secret=api_secret)
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

'''
if __name__ == "__main__":
	bb = binance_book()
	bb.run_book_manage()