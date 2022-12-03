#include "lstm.hpp"
#include "parameter.hpp"


int lstm(
    pricebase_t price[LENGTH][INPUT_SIZE],    // ACP
    pricebase_t prediction[OUTPUT_SIZE]       // ACE
){

#pragma HLS INTERFACE m_axi depth=4096 bundle=gmem0 port=price offset=slave
#pragma HLS INTERFACE m_axi depth=4096 bundle=gmem1 port=prediction offset=slave

#pragma HLS INTERFACE s_axilite port=price 	bundle=control
#pragma HLS INTERFACE s_axilite port=prediction bundle=control

#pragma HLS ARRAY_PARTITION variable=wx complete dim=2
#pragma HLS ARRAY_PARTITION variable=wh complete dim=2
#pragma HLS ARRAY_PARTITION variable=wx complete dim=2

#pragma HLS BIND_STORAGE variable=wx type=rom_1p
#pragma HLS BIND_STORAGE variable=d0 type=rom_2p
#pragma HLS BIND_STORAGE variable=d1 type=rom_2p


    pricebase_t cell[HIDDEN_SIZE] = {0};
    pricebase_t hidden[HIDDEN_SIZE] = {0};

    
    timestep:
    for (int i = 0; i < LENGTH; i++){
#pragma HLS DATAFLOW
        matmul_x();
        matmul_h();
        sum_xhb();
        activation_tanh();
        activation_sigmoid();
        activation_sigmoid();
        dotmul_add();
        activation_sigmoid();
        activation_tanh();
        dotmat();
    }
    fc();
    fc();

}




void matmul_x(
    pricebase_t x[INPUT_SIZE], 
    pricebase_t w[4 * HIDDEN_SIZE][INPUT_SIZE], 
    pricebase_t b[4 * HIDDEN_SIZE], 
    pricebase_t o[4*HIDDEN_SIZE]
    ){
    
    pricebase_t intm[INPUT_SIZE];
    for (int i = 0; i < 4 * HIDDEN_SIZE; i++){
#pragma HLS DATAFLOW
#pragma UNROLL factor=3
        for (int j = 0; j < INPUT_SIZE; j++){
#pragma HLS UNROLL
            intm[j] = w[i][j] * x[j];
        }

        pricebase_t sum = 0;
        for (int jj = 0; jj < INPUT_SIZE; jj++){
#pragma HLS UNROLL
            sum += intm[jj];
        }
        o[i] = sum + b[i];
    }
}


void matmul_h(
    pricebase_t x[HIDDEN_SIZE], 
    pricebase_t w[4 * HIDDEN_SIZE][HIDDEN_SIZE], 
    pricebase_t b[4 * HIDDEN_SIZE], 
    pricebase_t o[4*HIDDEN_SIZE]
    ){
    
    pricebase_t intm[HIDDEN_SIZE];
    for (int i = 0; i < 4 * HIDDEN_SIZE; i++){
#pragma HLS DATAFLOW
        for (int j = 0; j < HIDDEN_SIZE; j++){
#pragma HLS UNROLL factor=5
            intm[j] = w[i][j] * x[j];
        }

        pricebase_t sum = 0;
        for (int jj = 0; jj < HIDDEN_SIZE; jj++){
#pragma HLS UNROLL
            sum += intm[jj];
        }
        o[i] = sum + b[i];
    }

}

void sum_xhb(){

};
void activation_tanh(){

};
void activation_sigmoid(){

};
void activation_sigmoid(){

};
void dotmul_add(){

};
void activation_sigmoid(){

}
void activation_tanh(){

}
void dotmat(){

}

void fc(
    x[HIDDEN_SIZE],
    w[HIDDEN_SIZE_FC][HIDDEN_SIZE],
    b[HIDDEN_SIZE_FC],
    o[HIDDEN_SIZE_FC]
    ){

#pragma HLS DATAFLOW

    for (int ib = 0; ib < HIDDEN_SIZE_FC; ib++){
        o[ib] = b[ib];
    }

    for (int i = 0; i < HIDDEN_SIZE; i++){
#pragma HLS UNROLL factor=5
        for (int j = 0; j < HIDDEN_SIZE_FC; j++){
            o[j] += w[j][i] * x[i];
        }
    }
}