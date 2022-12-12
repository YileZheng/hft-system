#include "model.hpp"
#include "parameters.hpp"



template<int D0, int D1>
void split(
	pricebase_t tin[D0][D1],
	pricebase_t tout0[D0][D1],
	pricebase_t tout1[D0][D1]
){
	for ( int y = 0; y < D0; y++){
		for (int x = 0; x < D1; x++){
			tout0[y][x] = tin[y][x];
			tout1[y][x] = tin[y][x];
		}
	}
}

template<int D0, int D1>
void add_norm(
    pricebase_t tout[D0][D1],
    pricebase_t tin1[D0][D1],
    pricebase_t tin2[D0][D1],
    pricebase_t w[D1],
    pricebase_t b[D1]
    ){

    for (int y = 0; y < D0; y++){
//#pragma HLS UNROLL factor=4
////#pragma HLS DATAFLOW
#pragma HLS PIPELINE off
        pricebase_t mn = 0, sd = 0;
        pricebase_t tmpv[D1], tmpvv[D1];
        for (int x = 0; x < D1; x++){
            pricebase_t v = tin1[y][x] + tin2[y][x];
            mn += v;
            tmpv[x] = v;
        }
        mn = mn / D1;
        for (int xx = 0; xx < D1; xx++){
            pricebase_t vv = tmpv[xx];
            sd += hls::powf((vv - mn), 2);
            tmpvv[xx] =  vv - mn;

        }
        sd = hls::sqrtf(sd / D1);

        for (int xxx = 0; xxx < D1; xxx++){
            pricebase_t vvv = tmpvv[xxx];
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
#pragma HLS INLINE off
    int XO = XI - KX + 1;
    int YO = YI - KY + 1;
    for (int co = 0; co < KCO; co++){
#pragma HLS UNROLL factor=4
#pragma HLS PIPELINE off
        for (int xo = 0; xo < XO; xo++){
#pragma HLS UNROLL factor=4
#pragma HLS PIPELINE off
            for (int yo = 0; yo < YO; yo++){
#pragma HLS PIPELINE off
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
#pragma HLS INLINE off
    int XO = XI-KX+1;
    for (int co = 0; co < KCO; co++){
#pragma HLS UNROLL factor=4
#pragma HLS PIPELINE off
        for (int xo = 0; xo < XO; xo++){
#pragma HLS PIPELINE off
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
#pragma HLS INLINE off
    /*
    shape:
    tin1: (d0, dc)
    tin2: (d1, dc)
    tout: (d0, d1)
    tin1 (d0 X dc) x tin2.T (dc X d1) = tout (d0, d1)
    */
    for (int iy = 0; iy < D0; iy++){
#pragma HLS PIPELINE off
// pragma HLS UNROLL factor=4
        for (int ix = 0; ix < D1; ix++){
#pragma HLS UNROLL factor=4
#pragma HLS PIPELINE off
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
//#pragma HLS INLINE
    for (int iy = 0; iy < D0; iy++){
//#pragma HLS UNROLL factor=4
        for (int ix = 0; ix < D1; ix++){
//#pragma HLS UNROLL factor=4
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
//#pragma HLS INLINE
    for (int iy = 0; iy < D0; iy++){
        for (int ix = 0; ix < DC; ix++){
// pragma HLS UNROLL factor=4
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
//#pragma HLS INLINE
    for (int iy = 0; iy < D0; iy++){
        for (int ix = 0; ix < D1; ix++){
#pragma HLS UNROLL factor=4
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
#pragma HLS PIPELINE off
    pricebase_t tmp[D1];

    for (int c = 0; c < D0; c++){
		SOFTMAX_Y:
		for (int y = 0; y < D1; y++){
#pragma HLS UNROLL factor=4
			pricebase_t sum=0;

			SOFTMAX_EXP_SUM:
			for (int x = 0; x < D2; x++){
#pragma HLS PIPELINE
				pricebase_t v = hls::expf(tin[c][y][x]);
				sum += v;
				tmp[x] = v;
			}

			SOFTMAX_DIV:
			for (int xx = 0; xx < D2; xx++){
#pragma HLS PIPELINE
				tout[c][y][xx] = tmp[xx] / sum;
			}
		}
    }
}

void squeeze_padding(
	pricebase_t tout[COUT][INPUT_LENGTH+OUTPUT_LENGTH],
	pricebase_t tin[COUT][INPUT_LENGTH][1]
){
    PADDING:
    for (int pc = 0; pc < COUT; pc++){
#pragma HLS PIPELINE off
    	for (int pi = 0; pi < INPUT_LENGTH; pi++){
    		tout[pc][pi] = tin[pc][pi][0];
    	}
    	for (int pii = 0; pii < OUTPUT_LENGTH; pii++){
    		tout[pc][INPUT_LENGTH+pii] = 0;
    	}
    }

}


void model(
    pricebase_t price[INPUT_LENGTH*INPUT_SIZE],    // ACP
    pricebase_t prediction[OUTPUT_LENGTH]       // ACE
){

#pragma HLS INTERFACE m_axi depth=4096 max_read_burst_length=16 max_write_burst_length=16  bundle=gmem0 port=price offset=slave 
#pragma HLS INTERFACE m_axi depth=4096 max_read_burst_length=16 max_write_burst_length=16 bundle=gmem1 port=prediction offset=slave

#pragma HLS INTERFACE s_axilite port=price 	bundle=control
#pragma HLS INTERFACE s_axilite port=prediction bundle=ccontrol

// #pragma HLS DATAFLOW

    pricebase_t outChannels[COUT][INPUT_LENGTH][INPUT_SIZE];
#pragma HLS ARRAY_PARTITION variable=outChannels complete dim=1
#pragma HLS ARRAY_PARTITION variable=outChannels complete dim=3

    // encoder
    encoder(price, outChannels);

    // decoder
    pricebase_t ctmp0pad[COUT][INPUT_LENGTH+OUTPUT_LENGTH], ctmp1[COUT][INPUT_LENGTH-KERNEL_SIZE+1+OUTPUT_LENGTH];
    pricebase_t ctmp0phy[COUT][INPUT_LENGTH][1];
    pricebase_t out_pred[1][INPUT_LENGTH-KERNEL_SIZE+1+OUTPUT_LENGTH];
#pragma HLS ARRAY_PARTITION variable=ctmp0pad cyclic factor=7 dim=2
#pragma HLS ARRAY_PARTITION variable=ctmp1 cyclic factor=4 dim=1

    conv2d<COUT, INPUT_SIZE, INPUT_LENGTH, COUT, 1, INPUT_SIZE, 1>(ctmp0phy, outChannels, compressw, compressb);
    squeeze_padding(ctmp0pad, ctmp0phy);

    conv1d<COUT, INPUT_LENGTH+OUTPUT_LENGTH, COUT, 1, KERNEL_SIZE>(ctmp1, ctmp0pad, transfer1w, transfer1b);
    conv1d<COUT, INPUT_LENGTH-KERNEL_SIZE+1+OUTPUT_LENGTH, 1, COUT, 1>(out_pred, ctmp1, transfer2w, transfer2b);

    crop_pred(prediction, out_pred);
}


void crop_pred(
	pricebase_t tout[OUTPUT_LENGTH],
	pricebase_t tin[1][INPUT_LENGTH-KERNEL_SIZE+1+OUTPUT_LENGTH]
){
    for (int io = 0; io < OUTPUT_LENGTH; io++){
#pragma HLS PIPELINE
        tout[io] = tin[0][INPUT_LENGTH-KERNEL_SIZE+1+io];
    }
}


void encoder(
	pricebase_t price[INPUT_LENGTH * INPUT_SIZE],
	pricebase_t tout[COUT][INPUT_LENGTH][INPUT_SIZE]
){
#pragma HLS DATAFLOW

	pricebase_t price_split[COUT][INPUT_LENGTH][INPUT_SIZE];
#pragma HLS ARRAY_PARTITION variable=price_split complete dim=1
#pragma HLS ARRAY_PARTITION variable=price_split complete dim=3

	encoder_pricesplit(price_split, price);
	encoder_multilayer(tout, price_split);

}

void encoder_pricesplit(
	pricebase_t tout[COUT][INPUT_LENGTH][INPUT_SIZE],
	pricebase_t tin[INPUT_LENGTH * INPUT_SIZE]
){
#pragma HLS PIPELINE off
	for (int y = 0; y < INPUT_LENGTH; y++){
//#pragma PIPELINE off
		for (int x = 0; x < INPUT_SIZE; x++){
			pricebase_t v = tin[y * INPUT_SIZE + x];
//#pragma HLS UNROLL
			for (int c = 0; c < COUT; c++){
				tout[c][y][x] = v;
			}
		}
	}

}

void encoder_multilayer(
	pricebase_t tout[COUT][INPUT_LENGTH][INPUT_SIZE],
	pricebase_t tin[COUT][INPUT_LENGTH][INPUT_SIZE]
){
    
    ENCODERLAYER(0, tin, tout);
    ENCODERLAYER(1, tin, tout);
    ENCODERLAYER(2, tin, tout);
    ENCODERLAYER(3, tin, tout);
//     for (int ci = 0; ci < COUT; ci++){
// //#pragma HLS UNROLL factor=4
//         encoderlayer(tin[ci], tout[ci], ci);
//     }
}


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
){

#pragma HLS DATAFLOW
	pricebase_t tmp0[INPUT_LENGTH][INPUT_SIZE], tmp1[INPUT_LENGTH][INPUT_SIZE], tmp2[INPUT_LENGTH][INPUT_SIZE];
	pricebase_t tmpb01[INPUT_LENGTH][INPUT_SIZE], tmpb02[INPUT_LENGTH][INPUT_SIZE];
	pricebase_t tmpb11[INPUT_LENGTH][INPUT_SIZE], tmpb12[INPUT_LENGTH][INPUT_SIZE];
//#pragma HLS ARRAY_PARTITION variable=tmpb11 cyclic factor=4 dim=1
#pragma HLS ARRAY_PARTITION variable=tmpb11 cyclic factor=4 dim=2
#pragma HLS ARRAY_PARTITION variable=tmpb01 cyclic factor=4 dim=2
#pragma HLS ARRAY_PARTITION variable=tmpb12 cyclic factor=4 dim=2
#pragma HLS ARRAY_PARTITION variable=tmpb02 cyclic factor=4 dim=2
#pragma HLS ARRAY_PARTITION variable=tmp0 cyclic factor=4 dim=2
#pragma HLS ARRAY_PARTITION variable=tmp2 cyclic factor=4 dim=2
#pragma HLS ARRAY_PARTITION variable=tmp1 cyclic factor=4 dim=2

	split<INPUT_LENGTH, INPUT_SIZE>(price, tmpb01, tmpb02);
	multihead_attn(tmp0, tmpb01, teattninw, teattninb, teattnoutw, teattnoutb);
	add_norm<INPUT_LENGTH, INPUT_SIZE>(tmp1, tmp0, tmpb02, tenorm1w, tenorm1b);
	split<INPUT_LENGTH, INPUT_SIZE>(tmp1, tmpb11, tmpb12);
	feed_forward(tmp2, tmpb11, telinear1w, telinear1b, telinear2w, telinear2b);
	add_norm<INPUT_LENGTH, INPUT_SIZE>(tout, tmp2, tmpb12, tenorm2w, tenorm2b);
}


void multihead_attn(
    pricebase_t tout[INPUT_LENGTH][INPUT_SIZE],
    pricebase_t tin[INPUT_LENGTH][INPUT_SIZE],
    pricebase_t win[3*INPUT_SIZE][INPUT_SIZE],
    pricebase_t bin[3*INPUT_SIZE],
    pricebase_t wout[INPUT_SIZE][INPUT_SIZE],
    pricebase_t bout[INPUT_SIZE]
){
#pragma HLS INLINE 

    static pricebase_t scaling = powf(HEAD_DIM, -0.5);

#pragma HLS DATAFLOW

    pricebase_t ttmp0[INPUT_LENGTH][3*INPUT_SIZE];
    pricebase_t qs[INPUT_LENGTH][INPUT_SIZE], q[INPUT_LENGTH][INPUT_SIZE], k[INPUT_LENGTH][INPUT_SIZE], v[INPUT_LENGTH][INPUT_SIZE];
    pricebase_t qk[NUM_HEAD][INPUT_LENGTH][INPUT_LENGTH], qks[NUM_HEAD][INPUT_LENGTH][INPUT_LENGTH];
    pricebase_t mmv[INPUT_LENGTH][INPUT_SIZE];

//#pragma HLS ARRAY_PARTITION variable=q block factor=4 dim=2
//#pragma HLS ARRAY_PARTITION variable=qs block factor=4 dim=2
#pragma HLS ARRAY_PARTITION variable=k cyclic factor=4 dim=1
#pragma HLS ARRAY_PARTITION variable=qk cyclic factor=4 dim=3  
#pragma HLS ARRAY_PARTITION variable=qk cyclic factor=4 dim=2
#pragma HLS ARRAY_PARTITION variable=qks cyclic factor=4 dim=1
#pragma HLS ARRAY_PARTITION variable=qks cyclic factor=4 dim=2
// #pragma HLS ARRAY_PARTITION variable=qks cyclic factor=8 dim=3
// #pragma HLS ARRAY_PARTITION variable=v complete dim=1
#pragma HLS ARRAY_PARTITION variable=mmv block factor=4 dim=2
// #pragma HLS ARRAY_PARTITION variable=ttmp0 block factor=3 dim=2
#pragma HLS ARRAY_PARTITION variable=ttmp0 complete dim=2

    // Input projection
    linearT<INPUT_LENGTH, 3*INPUT_SIZE, INPUT_SIZE>(ttmp0, tin, win, bin);
    multiattn_seperate_qkv(q, k, v, ttmp0);

    // multihead scale dot-product
    multiattn_qscaling(qs, q, scaling);
    multiattn_qxk(qk, qs, k);

    // QK softmax
    softmax<NUM_HEAD, INPUT_LENGTH, INPUT_LENGTH>(qks, qk);

    // QK multiplied by V
    multiattn_xv(mmv, qks, v);

    // Out projection
    linearT<INPUT_LENGTH, INPUT_SIZE, INPUT_SIZE>(tout, mmv, wout, bout);
}


void feed_forward(
    pricebase_t tout[INPUT_LENGTH][INPUT_SIZE],
    pricebase_t tin[INPUT_LENGTH][INPUT_SIZE],
    pricebase_t w1[FEED_FORWARD_SIZE][INPUT_SIZE],
    pricebase_t b1[FEED_FORWARD_SIZE],
    pricebase_t w2[INPUT_SIZE][FEED_FORWARD_SIZE],
    pricebase_t b2[INPUT_SIZE]
){
#pragma HLS INLINE 
//#pragma HLS DATAFLOW
    pricebase_t ttmp0[INPUT_LENGTH][FEED_FORWARD_SIZE];
    pricebase_t ttmp1[INPUT_LENGTH][FEED_FORWARD_SIZE];
#pragma HLS ARRAY_PARTITION variable=ttmp1 complete dim=2
#pragma HLS ARRAY_PARTITION variable=ttmp0 cyclic factor=4 dim=2
    linearT<INPUT_LENGTH, FEED_FORWARD_SIZE, INPUT_SIZE>(ttmp0, tin, w1, b1);
    activation<INPUT_LENGTH, FEED_FORWARD_SIZE>(ttmp1, ttmp0);
    linearT<INPUT_LENGTH, INPUT_SIZE, FEED_FORWARD_SIZE>(tout, ttmp1, w2, b2);
}


void multiattn_seperate_qkv(
	pricebase_t q[INPUT_LENGTH][INPUT_SIZE],
	pricebase_t k[INPUT_LENGTH][INPUT_SIZE],
	pricebase_t v[INPUT_LENGTH][INPUT_SIZE],
	pricebase_t tin[INPUT_LENGTH][3*INPUT_SIZE]
){
	MULTIHEAD_QKV:
	for (int y = 0; y < INPUT_LENGTH; y++){
#pragma HLS PIPELINE off
		for (int x = 0; x < INPUT_SIZE; x++){
#pragma HLS PIPELINE
			q[y][x] = tin[y][x];
		}
		for (int xx = 0; xx < INPUT_SIZE; xx++){
#pragma HLS PIPELINE
			k[y][xx] = tin[y][xx + INPUT_SIZE];
		}
		for (int xxx = 0; xxx < INPUT_SIZE; xxx++){
#pragma HLS PIPELINE
			v[y][xxx] = tin[y][xxx + 2*INPUT_SIZE];
		}
	}
}

void multiattn_qscaling(
	pricebase_t qs[INPUT_LENGTH][INPUT_SIZE],
	pricebase_t q[INPUT_LENGTH][INPUT_SIZE],
	pricebase_t scaling
){

    MULTIHEAD_QSCALE:
    for (int ih = 0; ih < NUM_HEAD; ih++){
//#pragma HLS UNROLL factor=4
        for (int il = 0; il < INPUT_LENGTH; il++){
// #pragma HLS UNROLL factor=4
#pragma HLS PIPELINE
            for (int ihd = 0; ihd < HEAD_DIM; ihd++){
                qs[il][ih*HEAD_DIM+ihd] = q[il][ih*HEAD_DIM+ihd] * scaling;
            }
        }
    }
}

void multiattn_qxk(
	pricebase_t qk[NUM_HEAD][INPUT_LENGTH][INPUT_LENGTH],
	pricebase_t qs[INPUT_LENGTH][INPUT_SIZE],
	pricebase_t k[INPUT_LENGTH][INPUT_SIZE]
){
    MULTIHEAD_QK:
    for (int ih0 = 0; ih0 < NUM_HEAD; ih0++){
//#pragma HLS UNROLL factor=4
        // unroll by the number of heads
        for (int iy0 = 0; iy0 < INPUT_LENGTH; iy0++){
            for (int ix0 = 0; ix0 < INPUT_LENGTH; ix0++){
#pragma HLS UNROLL factor=4
#pragma HLS PIPELINE
                pricebase_t vv = 0;
                for (int ihd0 = 0; ihd0 < HEAD_DIM; ihd0++){
                    vv += qs[iy0][ih0*HEAD_DIM+ihd0] * k[ix0][ih0*HEAD_DIM+ihd0];
                }
                qk[ih0][iy0][ix0] = vv;
            }
        }
    }
}

void multiattn_xv(
	pricebase_t mmv[INPUT_LENGTH][INPUT_SIZE],
	pricebase_t qks[NUM_HEAD][INPUT_LENGTH][INPUT_LENGTH],
	pricebase_t v[INPUT_LENGTH][INPUT_SIZE]
){
#pragma HLS PIPELINE off

    MULTIHEAD_MMV:
    for (int ih1 = 0; ih1 < NUM_HEAD; ih1++){
// #pragma HLS UNROLL factor=4
        for (int iy1 = 0; iy1 < INPUT_LENGTH; iy1++){
#pragma HLS UNROLL factor=4
            for (int ix1 = 0; ix1 < HEAD_DIM; ix1++){
// //#pragma HLS UNROLL factor=1
#pragma HLS PIPELINE off
                pricebase_t v1 = 0;
                for (int ic1 = 0; ic1 < INPUT_LENGTH; ic1++){
                    v1 += qks[ih1][iy1][ic1] * v[ic1][ih1*HEAD_DIM+ix1];
                }
                mmv[iy1][ih1*HEAD_DIM+ix1] = v1;
            }
        }
    }
}
