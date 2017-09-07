#include <string>
#include <iostream>
#include <memory.h>

#include "ARQ.h"

using namespace std;

ARQ::ARQ(Enquadramento & enq, int bytes_min): enquadra(enq){
    N = 0;
    M = 1;
    min_bytes = bytes_min;
}

void ARQ::envia(char * buffer, int bytes) {
    char preenc;
    preenc=0x00;
    //Enquadramento enquadra(porta, max_bytes);
    Evento e;
    
        if(bytes<min_bytes){
            int tam=min_bytes - bytes;
            for(int i=0; i<tam; i++){
                buffer[bytes+i] = preenc;
            }
            bytes = 8;
        }
    
    e.tipo = Payload;
    e.ptr = buffer;
    e.num_bytes = bytes;

    handle(e);
    
    int bytes_enq;
    bytes_enq = enquadra.recebe(buffer);
    e.num_bytes = bytes_enq;
    e.quadro_recebido = buffer;

    handle(e);

}

int ARQ::recebe(char * buffer) {
    //Enquadramento enquadra(porta, max_bytes);
    Evento e;
    int bytes_enq;
    
    
    while (true) {
        bytes_enq = enquadra.recebe(buffer);

        e.tipo = Quadro;
        e.ptr = buffer;
        e.num_bytes = bytes_enq;

        if(handle(e)){
           retiraCabecalho(buffer,bytes_enq); 
           return bytes_enq-1;     
        }

    }
}

// primeiro bit = tipo de quadro - 0 mensagem / 1 ack
// segundo bit = numero de sequencia
//0x00 = mensagem/sequencia zero
//0x01 = mensagem/sequencia um
//0x02 = ack/sequencia zero
//0x03 =  ack/sequencia um
bool ARQ::handle(Evento e) {
    //Enquadramento enquadra(porta, max_bytes);

    switch (estado) {
        case EST0: // estado 0
            if (e.tipo == Payload) {
                mudaPayload(e.ptr, e.num_bytes, N);
                enquadra.envia(e.ptr, e.num_bytes+1);
                estado = EST1;
            } else if (e.tipo == Quadro) {
                // RESPONDER ACK COM NUMERO DE SEQUENCIA RECEBIDO
                if (!AckOuMensagem(e.ptr[0])) {//verifica se é quadro de mensagem

                    if (M == returnNumSeq(e.ptr[0])) {
                        criaACK(e.ptr[0]);
                        enquadra.envia(buff, 1);
                        return false;
                    } else {
                        criaACK(e.ptr[0]);
                        enquadra.envia(buff, 1);
                        M = returnNumSeq(e.ptr[0]);
                        return true;
                    }
                }
            }
            break;

        case EST1: // estado 1
            if (e.tipo == Payload) {
                if (AckOuMensagem(e.quadro_recebido[0])) {//verifica se é ACK
                    if (returnNumSeq(e.quadro_recebido[0]) == N) {// verifica se Numero de sequencia é correto
                        cout << "ACK " << N << endl;
                        N = not(N);
                        estado = EST0;
                    } else {
                        enquadra.envia(e.ptr, e.num_bytes+1);
                    }
                    return false;
                }
                return false;


            } else if (e.tipo == Quadro) {
                // RESPONDER ACK COM NUMERO DE SEQUENCIA RECEBIDO

                if (!AckOuMensagem(e.ptr[0])) {//verifica se é quadro de mensagem

                    if (M == returnNumSeq(e.ptr[0])) {
                        criaACK(e.ptr[0]);
                        enquadra.envia(buff, 1);
                        return false;
                    } else {
                        criaACK(e.ptr[0]);
                        enquadra.envia(buff, 1);
                        M = returnNumSeq(e.ptr[0]);
                        return true;
                    }
                }
            }

            break;
    }
}

void ARQ::retiraCabecalho(char * buffer,int bytes) {
    memset(buffer_arq, '\0', sizeof(buffer_arq));
    
    for (int i = 0; i < bytes-1; i++) {
        buffer_arq[i] = buffer[i+1];
    }

    memcpy(buffer, buffer_arq, bytes-1);
}

void ARQ::mudaPayload(char * buffer, int bytes, bool N) {

    if (N == 0) {
        buffer_arq[0] = 0x00;
    } else {
        buffer_arq[0] = 0x01;
    }

    for (int i = 0; i < bytes; i++) {
        buffer_arq[i + 1] = buffer[i];
    }

    memcpy(buffer, buffer_arq, bytes + 1);
}

void ARQ::criaACK(char byte) {
    if (returnNumSeq(byte)) {
        buff[0] = 0x03;
    } else {
        buff[0] = 0x02;
    }

}

bool ARQ::AckOuMensagem(char byte) {
    if ((byte >> 1)&1 == 1) {
        return true;
    } else {
        return false;
    }
}

bool ARQ::returnNumSeq(char byte) {
    if (byte & 1 == 1) {
        return true;
    } else {
        return false;
    }
}

