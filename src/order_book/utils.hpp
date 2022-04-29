
#ifndef __UTILS_H__
#define __UTILS_H__

bool is_after(
	price_t &price_in,
	price_t &price_onchain,
	ap_uint<1> &bid
);

template <typename T, int SIZE>
class storage{
	T mem[SIZE];
	int top=0;
	public:
	bool empty(){return top==0;}
	// bool full(){return top==SIZE;}
	T read(){return mem[--top];}
	void write(T data){return mem[top++] = data;}
};

#endif