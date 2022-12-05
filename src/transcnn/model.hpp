
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

typedef float pricebase_t;

void model(
    pricebase_t price[INPUT_LENGTH][INPUT_SIZE],    // ACP
    pricebase_t prediction[OUTPUT_LENGTH]       // ACE
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
    pricebase_t bout[INPUT_SIZE],
	int instan
);

template<int D0, int D1>
void add_norm(
    pricebase_t tout[D0][D1], 
    pricebase_t tin1[D0][D1],
    pricebase_t tin2[D0][D1],
    pricebase_t w[D1],
    pricebase_t b[D1]
    ){

    for (int y = 0; y < D0; y++){
        pricebase_t mn = 0, sd = 0;
        pricebase_t tmpv[D1];
#pragma HLS DATAFLOW
#pragma HLS UNROLL factor=4
        for (int x = 0; x < D1; x++){
            pricebase_t v = tin1[y][x] + tin2[y][x];
            mn += v;
            tmpv[x] = v;
        }
        mn = mn / D1;
        for (int xx = 0; xx < D1; xx++){
            pricebase_t vv = tmpv[xx];
            sd += hls::powf((vv - mn), 2);
            tmpv[xx] -= mn;

        }
        sd = hls::sqrtf(sd / D1);

        for (int xxx = 0; xxx < D1; xxx++){
            pricebase_t vvv = tmpv[xxx];
            tout[y][xxx] = vvv / (sd + LAYERNORM_EPS) * w[xxx] + b[xxx]; 
            // TODO: tout partition by y by factor of unroll cyclically
        }
    }
}


template <int CI, int XI, int YI, int KCO, int KCI, int KX, int KY>
void conv2d(
    pricebase_t tout[KCO][YI-KY+1][XI-KX+1], 
    pricebase_t tin[CI][YI][XI], 
    pricebase_t k[KCO][KCI][KY][KX],
    pricebase_t b[KCO]
){
    int XO = XI - KX + 1;
    int YO = YI - KY + 1;
    for (int co = 0; co < KCO; co++){
        for (int xo = 0; xo < XO; xo++){
            for (int yo = 0; yo < YO; yo++){
                pricebase_t tmp = b[co];
                for (int kci = 0; kci < KCI; kci++){
                    for (int kx = 0; kx < KX; kx++){
                        for (int ky = 0; ky < KY; ky++){
                            tmp += tin[(KCI*co)%CI+kci][yo+ky][xo+kx] * k[co][kci][ky][kx]; 
                        }
                    }
                }
                tout[co][yo][xo] = tmp;
            }
        }
    }
}


template <int CI, int XI, int KCO, int KCI, int KX>
void conv1d(
    pricebase_t tout[KCO][XI-KX+1], 
    pricebase_t tin[CI][XI], 
    pricebase_t k[KCO][KCI][KX], 
    pricebase_t b[KCO]
){
    int XO = XI-KX+1;
    for (int co = 0; co < KCO; co++){
        for (int xo = 0; xo < XO; xo++){
            pricebase_t tmp = b[co];
            for (int kci = 0; kci < KCI; kci++){
                for (int kx = 0; kx < KX; kx++){
                    tmp += tin[(KCI*co)%CI+kci][xo+kx] * k[co][kci][kx]; 
                }
            }
            tout[co][xo] = tmp;
        }
    }
}


template <int D0, int D1, int DC>
void linearT(
    pricebase_t tout[D0][D1], 
    pricebase_t tin1[D0][DC], 
    pricebase_t tin2[D1][DC],
    pricebase_t b[D1]
){
    /*
    shape:
    tin1: (d0, dc)
    tin2: (d1, dc)
    tout: (d0, d1)
    tin1 (d0 X dc) x tin2.T (dc X d1) = tout (d0, d1)
    */
#pragma HLS INLINE 
    for (int iy = 0; iy < D0; iy++){
# pragma HLS UNROLL factor=4
        for (int ix = 0; ix < D1; ix++){
# pragma HLS UNROLL factor=4
            pricebase_t tmp = b[ix];
            for (int i = 0; i < DC; i++){
                tmp += tin1[iy][i] * tin2[ix][i];
            }
            tout[iy][ix] = tmp;
        }
    }
}


template <int D0, int D1, int DC> 
void matmul(
    pricebase_t tout[D0][D1], 
    pricebase_t tin1[D0][DC], 
    pricebase_t tin2[DC][D1]
){
    /*
    Matrix multiplication
    shape:
    tin1: (d0, dc)
    tin2: (dc, d1)
    tout: (d0, d1)
    tin1 (d0 X dc) x tin2 (dc X d1) = tout (d0, d1)
    */
#pragma HLS INLINE 
    for (int iy = 0; iy < D0; iy++){
# pragma HLS UNROLL factor=4
        for (int ix = 0; ix < D1; ix++){
# pragma HLS UNROLL factor=4
            pricebase_t tmp = 0;
            for (int i = 0; i < DC; i++){
                tmp += tin1[iy][i] * tin2[i][ix];
            }
            tout[iy][ix] = tmp;
        }
    }
}


template <int D0, int DC> 
void elelinear(
    pricebase_t tout[D0][DC],
    pricebase_t tin[D0][DC], 
    pricebase_t w[DC], 
    pricebase_t b[DC]
){
    /*
    Element-wise Matrix-Vector multiplication
    */
#pragma HLS INLINE 
    for (int iy = 0; iy < D0; iy++){
        for (int ix = 0; ix < DC; ix++){
# pragma HLS UNROLL factor=4
            tout[iy][ix] = tin[iy][ix] * w[ix] + b[ix];
        }
    }
}


template <int D0, int D1> 
void activation(
    pricebase_t tout[D0][D1], 
    pricebase_t tin[D0][D1]
){
    /*
    ReLU 
    */
#pragma HLS INLINE 
    for (int iy = 0; iy < D0; iy++){
        for (int ix = 0; ix < D1; ix++){
# pragma HLS UNROLL factor=4
            pricebase_t v = tin[iy][ix];
            tout[iy][ix] = (v > 0)? v : 0;
        }
    }
}

template<int D0, int D1, int D2>
void softmax(
    pricebase_t tout[D0][D1][D2],
    pricebase_t tin[D0][D1][D2]
){
    pricebase_t tmp[D1];

    for (int c = 0; c < D0; c++){
		SOFTMAX_Y:
		for (int y = 0; y < D1; y++){
			pricebase_t sum=0;

			SOFTMAX_EXP_SUM:
			for (int x = 0; x < D2; x++){
				pricebase_t v = hls::expf(tin[c][y][x]);
				sum += v;
				tmp[x] = v;
			}

			SOFTMAX_DIV:
			for (int xx = 0; xx < D2; xx++){
				tout[c][y][xx] = tmp[xx] / sum;
			}
		}
    }
}


#endif
