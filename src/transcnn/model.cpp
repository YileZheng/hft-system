#include "model.hpp"
#include "parameters.hpp"


void model(
    pricebase_t price[INPUT_LENGTH][INPUT_SIZE],    // ACP
    pricebase_t prediction[OUTPUT_LENGTH]       // ACE
){

#pragma HLS INTERFACE m_axi depth=4096 bundle=gmem0 port=price offset=slave
#pragma HLS INTERFACE m_axi depth=4096 bundle=gmem1 port=prediction offset=slave

#pragma HLS INTERFACE s_axilite port=price 	bundle=control
#pragma HLS INTERFACE s_axilite port=prediction bundle=control

// #pragma HLS ARRAY_PARTITION variable=wx complete dim=2
// #pragma HLS ARRAY_PARTITION variable=wh complete dim=2
// #pragma HLS ARRAY_PARTITION variable=wx complete dim=2

// #pragma HLS BIND_STORAGE variable=wx type=rom_1p
// #pragma HLS BIND_STORAGE variable=d0 type=rom_2p
// #pragma HLS BIND_STORAGE variable=d1 type=rom_2p

    pricebase_t outChannels[COUT][INPUT_LENGTH][INPUT_SIZE];

#pragma HLS DATAFLOW
    // encoder
    for (int ci = 0; ci < COUT; ci++){
#pragma HLS UNROLL factor=4
        pricebase_t tmp0[INPUT_LENGTH][INPUT_SIZE], tmp1[INPUT_LENGTH][INPUT_SIZE], tmp2[INPUT_LENGTH][INPUT_SIZE];
        multihead_attn(tmp0, price, teattninw[ci], teattninb[ci], teattnoutw[ci], teattnoutb[ci], ci);
        add_norm<INPUT_LENGTH, INPUT_SIZE>(tmp1, tmp0, price, tenorm1w[ci], tenorm1b[ci]);
        feed_forward(tmp2, tmp1, telinear1w[ci], telinear1b[ci], telinear2w[ci], telinear2b[ci]);
        add_norm<INPUT_LENGTH, INPUT_SIZE>(outChannels[ci], tmp2, tmp1, tenorm2w[ci], tenorm2b[ci]);
    }


    // decoder
    pricebase_t ctmp0pad[COUT][INPUT_LENGTH+OUTPUT_LENGTH] = {0}, ctmp1[COUT][INPUT_LENGTH-KERNEL_SIZE+1+OUTPUT_LENGTH];
    pricebase_t ctmp0phy[COUT][INPUT_LENGTH][1];
    pricebase_t out_pred[1][INPUT_LENGTH-KERNEL_SIZE+1+OUTPUT_LENGTH];

    conv2d<COUT, INPUT_SIZE, INPUT_LENGTH, COUT, 1, INPUT_SIZE, 1>(ctmp0phy, outChannels, compressw, compressb);
    PADDING:
    for (int pc = 0; pc < COUT; pc++){
    	for (int pi = 0; pi < INPUT_LENGTH; pi++){
    		ctmp0pad[pc][pi] = ctmp0phy[pc][pi][0];
    	}
    }

    conv1d<COUT, INPUT_LENGTH+OUTPUT_LENGTH, COUT, 1, KERNEL_SIZE>(ctmp1, ctmp0pad, transfer1w, transfer1b);
    conv1d<COUT, INPUT_LENGTH-KERNEL_SIZE+1+OUTPUT_LENGTH, 1, COUT, 1>(out_pred, ctmp1, transfer2w, transfer2b);

    for (int io = 0; io < OUTPUT_LENGTH; io++){
        prediction[io] = out_pred[0][INPUT_LENGTH-KERNEL_SIZE+1+io];
    }

}


void multihead_attn(
    pricebase_t tout[INPUT_LENGTH][INPUT_SIZE], 
    pricebase_t tin[INPUT_LENGTH][INPUT_SIZE], 
    pricebase_t win[3*INPUT_SIZE][INPUT_SIZE], 
    pricebase_t bin[3*INPUT_SIZE], 
    pricebase_t wout[INPUT_SIZE][INPUT_SIZE], 
    pricebase_t bout[INPUT_SIZE],
	int instan
){
#pragma HLS FUNCTION_INSTANTIATE variable=instan

    static pricebase_t scaling = powf(HEAD_DIM, -0.5);

#pragma HLS DATAFLOW

    pricebase_t ttmp0[INPUT_LENGTH][3*INPUT_SIZE];
    pricebase_t q[INPUT_LENGTH][INPUT_SIZE]; 
    pricebase_t qk[NUM_HEAD][INPUT_LENGTH][INPUT_LENGTH], qks[NUM_HEAD][INPUT_LENGTH][INPUT_LENGTH]; 
    pricebase_t mmv[INPUT_LENGTH][INPUT_SIZE];

    // Input projection
    linearT<INPUT_LENGTH, 3*INPUT_SIZE, INPUT_SIZE>(ttmp0, tin, win, bin);

    // multihead scale dot-product
    MULTIHEAD_QSCALE:
    for (int ih = 0; ih < NUM_HEAD; ih++){
        for (int il = 0; il < INPUT_LENGTH; il++){
#pragma HLS UNROLL factor=4
            for (int ihd = 0; ihd < HEAD_DIM; ihd++){
                q[il][ih*HEAD_DIM+ihd] = ttmp0[il][ih*HEAD_DIM+ihd] * scaling;
            }
        }
    }

    MULTIHEAD_QK:
    for (int ih0 = 0; ih0 < NUM_HEAD; ih0++){
        // unroll by the number of heads
#pragma HLS UNROLL factor=4
        for (int iy0 = 0; iy0 < INPUT_LENGTH; iy0++){
#pragma HLS UNROLL factor=4
            for (int ix0 = 0; ix0 < INPUT_LENGTH; ix0++){
                pricebase_t v = 0;
                for (int ihd0 = 0; ihd0 < HEAD_DIM; ihd0++){
                    v += q[iy0][ih0*HEAD_DIM+ihd0] * ttmp0[ix0][ih0*HEAD_DIM+ihd0+INPUT_SIZE];
                }
                qk[ih0][iy0][ix0] = v;
            }
        }
    }

    // QK softmax
    softmax<NUM_HEAD, INPUT_LENGTH, INPUT_LENGTH>(qks, qk);

    // QK multiplied by V
    MULTIHEAD_MMV:
    for (int ih1 = 0; ih1 < NUM_HEAD; ih1++){
#pragma HLS UNROLL factor=4
        for (int iy1 = 0; iy1 < INPUT_LENGTH; iy1++){
            for (int ix1 = 0; ix1 < HEAD_DIM; ix1++){
#pragma HLS UNROLL factor=1
                pricebase_t v1 = 0;
                for (int ic1 = 0; ic1 < INPUT_LENGTH; ic1++){
                    v1 += qks[ih1][iy1][ic1] * ttmp0[ic1][ih1*HEAD_DIM+ix1+2*INPUT_SIZE];
                }
                mmv[iy1][ih1*HEAD_DIM+ix1] = v1;
            }
        }
    }

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
#pragma HLS DATAFLOW
    pricebase_t ttmp0[INPUT_LENGTH][FEED_FORWARD_SIZE];
    pricebase_t ttmp1[INPUT_LENGTH][FEED_FORWARD_SIZE];
    linearT<INPUT_LENGTH, FEED_FORWARD_SIZE, INPUT_SIZE>(ttmp0, tin, w1, b1);
    activation<INPUT_LENGTH, FEED_FORWARD_SIZE>(ttmp1, ttmp0);
    linearT<INPUT_LENGTH, INPUT_SIZE, FEED_FORWARD_SIZE>(tout, ttmp1, w2, b2);
}
