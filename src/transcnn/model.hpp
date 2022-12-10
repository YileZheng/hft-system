
#ifndef __TRANSCNN_HPP__
#define __TRANSCNN_HPP__

#include "hls_math.h"

#define INPUT_LENGTH 24
#define OUTPUT_LENGTH 6
#define INPUT_SIZE 4

#define COUT 4
#define KERNEL_SIZE 7
#define NUM_HEAD 4
#define FEED_FORWARD_SIZE 4
#define HEAD_DIM INPUT_SIZE/NUM_HEAD
#define ATTEN_QSCALING powf(HEAD_DIM, -0.5)
#define LAYERNORM_EPS 1e-5

#define LENGTHWISE_UNROLL 4

#define ENCODERLAYER(n, in, out) \
	encoderlayer(in[n], out[n],\
			te##n##attninw,  \
			te##n##attninb,  \
			te##n##attnoutw,  \
			te##n##attnoutb,  \
			te##n##norm1w,  \
			te##n##norm1b,  \
			te##n##norm2w,  \
			te##n##norm2b,  \
			te##n##linear1w,  \
			te##n##linear1b,  \
			te##n##linear2w,  \
			te##n##linear2b  \
			)

typedef float pricebase_t;

void model(
    pricebase_t price[INPUT_LENGTH][INPUT_SIZE],    // ACP
    pricebase_t prediction[OUTPUT_LENGTH]       // ACE
);

void encoder_pricesplit(
	pricebase_t tout[COUT][INPUT_LENGTH][INPUT_SIZE],
	pricebase_t tin[INPUT_LENGTH][INPUT_SIZE]
);

void encoder_multilayer(
	pricebase_t tout[COUT][INPUT_LENGTH][INPUT_SIZE],
	pricebase_t tin[COUT][INPUT_LENGTH][INPUT_SIZE]
);

void crop_pred(
	pricebase_t tout[OUTPUT_LENGTH],
	pricebase_t tin[1][INPUT_LENGTH-KERNEL_SIZE+1+OUTPUT_LENGTH]
);

void encoder(
	pricebase_t price[INPUT_LENGTH][INPUT_SIZE],
	pricebase_t tout[COUT][INPUT_LENGTH][INPUT_SIZE]
);

void encoderlayer(
	pricebase_t price[INPUT_LENGTH][INPUT_SIZE],
	pricebase_t tout[INPUT_LENGTH][INPUT_SIZE],
	pricebase_t teattninw[3*INPUT_SIZE][INPUT_SIZE],
	pricebase_t teattninb[3*INPUT_SIZE],
	pricebase_t teattnoutw[INPUT_SIZE][INPUT_SIZE],
	pricebase_t teattnoutb[INPUT_SIZE],
	pricebase_t tenorm1w[INPUT_SIZE],
	pricebase_t tenorm1b[INPUT_SIZE],
	pricebase_t tenorm2w[INPUT_SIZE],
	pricebase_t tenorm2b[INPUT_SIZE],
	pricebase_t telinear1w[INPUT_SIZE][INPUT_SIZE],
	pricebase_t telinear1b[INPUT_SIZE],
	pricebase_t telinear2w[INPUT_SIZE][INPUT_SIZE],
	pricebase_t telinear2b[INPUT_SIZE]
);

void feed_forward(
    pricebase_t tout[INPUT_LENGTH][INPUT_SIZE], 
    pricebase_t tin[INPUT_LENGTH][INPUT_SIZE], 
    pricebase_t w1[FEED_FORWARD_SIZE][INPUT_SIZE],
    pricebase_t b1[FEED_FORWARD_SIZE],
    pricebase_t w2[INPUT_SIZE][FEED_FORWARD_SIZE],
    pricebase_t b2[INPUT_SIZE]
);

void multihead_attn(
    pricebase_t tout[INPUT_LENGTH][INPUT_SIZE], 
    pricebase_t tin[INPUT_LENGTH][INPUT_SIZE], 
    pricebase_t win[3*INPUT_SIZE][INPUT_SIZE], 
    pricebase_t bin[3*INPUT_SIZE], 
    pricebase_t wout[INPUT_SIZE][INPUT_SIZE], 
    pricebase_t bout[INPUT_SIZE]
);

void multiattn_xv(
	pricebase_t mmv[INPUT_LENGTH][INPUT_SIZE],
	pricebase_t qks[NUM_HEAD][INPUT_LENGTH][INPUT_LENGTH],
	pricebase_t v[INPUT_LENGTH][INPUT_SIZE]
);

void multiattn_qxk(
	pricebase_t qk[NUM_HEAD][INPUT_LENGTH][INPUT_LENGTH],
	pricebase_t qs[INPUT_LENGTH][INPUT_SIZE],
	pricebase_t k[INPUT_LENGTH][INPUT_SIZE]
);

void multiattn_qscaling(
	pricebase_t qs[INPUT_LENGTH][INPUT_SIZE],
	pricebase_t q[INPUT_LENGTH][INPUT_SIZE],
	pricebase_t scaling
);

void multiattn_seperate_qkv(
	pricebase_t q[INPUT_LENGTH][INPUT_SIZE],
	pricebase_t k[INPUT_LENGTH][INPUT_SIZE],
	pricebase_t v[INPUT_LENGTH][INPUT_SIZE],
	pricebase_t tin[INPUT_LENGTH][3*INPUT_SIZE]
);

#endif
