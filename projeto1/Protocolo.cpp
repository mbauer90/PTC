#include "Protocolo.h"

Protocolo::Protocolo(ARQ & a): arq(a) {
    //min_bytes = bytes_min;
    //max_bytes = bytes_max;
    //ARQ arq(porta,bytes_min,bytes_max);
}

void Protocolo::envia(char * buffer, int bytes){ 
    arq.envia(buffer,bytes);
    
}

int Protocolo::recebe(char * buffer){
    //ARQ arq(porta,min_bytes,max_bytes);
    int bytes = arq.recebe(buffer);
    return bytes;
}
