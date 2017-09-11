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
    
    bool teste=true;
    while(teste){
        int bytes_enq;
        memset(buffer, '\0', sizeof(buffer));

        bytes_enq = enquadra.recebe(buffer);
        //e.num_bytes = bytes_enq;
        e.ptr = buffer;

        //handle(e);
     
        if(handle(e)){
            teste = false;
        }
    }

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
           bytes_enq--;
           return bytes_enq;     
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
                e.num_bytes++;
                memcpy(buffer_reenvio, e.ptr, e.num_bytes);
                enquadra.envia(e.ptr, e.num_bytes);
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
                if (AckOuMensagem(e.ptr[0])) {//verifica se é ACK
                    if (returnNumSeq(e.ptr[0]) == N) {// verifica se Numero de sequencia é correto
                        cout << "ACK " << N << endl;
                        N = not(N);
                        estado = EST0;
                        return true;
                    } else {
                        cout << "REENVIANDO" <<endl;
                        e.num_bytes++;
                        memset(e.ptr, '\0', sizeof(e.ptr));
                        memcpy(e.ptr, buffer_reenvio, e.num_bytes);
                        enquadra.envia(e.ptr, e.num_bytes);
                    
                        return false;
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
     memset(buffer_arq, '\0', sizeof(buffer_arq));
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
    memset(buff, '\0', sizeof(buff));
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

void ARQ::imprimeHexa(char * buffer, int len) {
   int m = 0, line = 0;
 
    while (m < len) {
        printf("%02X: ", line*16);
 
        for (int n=0; n < 16 and m < len; n++, m++) {
            int x = (unsigned char)buffer[m];
            printf("%02X ", x);
        }
        puts("");
        line++;
    }        
}

