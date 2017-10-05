#include <string>
#include <iostream>
#include <memory.h>
#include <queue>
#include "ARQ.h"
#include <sys/time.h>

#define MAX 6000
#define MIN 2000
#define TIME_QUAD 5000

using namespace std;



ARQ::ARQ(Enquadramento & enq, int bytes_min) : enquadra(enq) {
    N = 0;
    M = 1;
    min_bytes = bytes_min;
    estado = EST0;
}

void ARQ::envia(char * buffer, int bytes) {
    char preenc;
    preenc = 0x00;
    //Enquadramento enquadra(porta, max_bytes);
    Evento e;


    if (bytes < min_bytes) {
        int tam = min_bytes - bytes;
        for (int i = 0; i < tam; i++) {
            buffer[bytes + i] = preenc;
        }
        bytes = 8;
    }


    e.tipo = Payload;
    e.ptr = buffer;
    e.num_bytes = bytes;
    e.reenvio_bytes=bytes;

    handle(e);

    bool teste = true;
    while (teste) {
        int bytes_enq;
        memset(buffer, '\0', e.num_bytes);

        bytes_enq = enquadra.recebe(buffer, TIME_QUAD);
        
        //e.num_bytes = bytes_enq;
        if (bytes_enq == 0) {
            e.tipo = Timeout;
        } else {
            e.tipo = Payload;
        }
        e.num_bytes = bytes_enq;
        e.ptr = buffer;

        //handle(e);
        
        if (handle(e)) {
            teste = false;
        }

        if (handle(e)) {
            teste = false;
        }




        //        for (int i = 0; i < 2; i++) {
        //            if (handle(e)) {
        //                teste = false;
        //            }
        //        }



    }

}

int ARQ::recebe(char * buffer) {
    Evento e;
    S_Quadro * quad;
    int bytes_enq;

    while (!recebido.empty()) {

        quad = recebido.front();
        recebido.pop();

        memcpy(buffer, quad->q_ptr, quad->q_len);
       // cout<<"estou aqui: ";
        //imprimeHexa(quad->q_ptr,10);
       // cout<<"Quad LEN:"<<quad->q_len<<endl;
        e.tipo = Quadro;
        e.ptr = quad->q_ptr;
        e.num_bytes = quad->q_len;
        // cout << "retira fila" << endl;
        if (handle(e)) {
            retiraCabecalho(buffer, quad->q_len);
            quad->q_len--;

            return quad->q_len;
        }
    }
    //  cout << "CHEGOU4" << endl;
    while (true) {
        bytes_enq = enquadra.recebe(buffer, TIME_QUAD);
        if (bytes_enq == 0) {
            e.tipo = Timeout;
        } else {
            e.tipo = Quadro;
            e.ptr = buffer;
            e.num_bytes = bytes_enq;
        }

        if (handle(e)) {
            retiraCabecalho(buffer, bytes_enq);
            bytes_enq--;
            return bytes_enq;
        }
        return 0;

    }

}

// primeiro bit = tipo de quadro - 0 mensagem / 1 ack
// segundo bit = numero de sequencia
//0x00 = mensagem/sequencia zero
//0x01 = mensagem/sequencia um
//0x02 = ack/sequencia zero
//0x03 =  ack/sequencia um

bool ARQ::handle(Evento e) {
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
                        cout<<"Enviou ACK"<<returnNumSeq(e.ptr[0])<<endl;
                        enquadra.envia(buff, 1);
                        return false;
                    } else {
                        criaACK(e.ptr[0]);
                        cout<<"Enviou ACK"<<returnNumSeq(e.ptr[0])<<endl;
                        enquadra.envia(buff, 1);
                        M = returnNumSeq(e.ptr[0]);
                        return true;
                    }
                }
                return false;
            } else if (e.tipo == Timeout) {
                return false;
            }
            break;

        case EST1: // estado 1
            if (e.tipo == Payload) {
                if (AckOuMensagem(e.ptr[0])) {//verifica se é ACK
                    if (returnNumSeq(e.ptr[0]) == N) {// verifica se Numero de sequencia é correto
                        cout << "Recebeu ACK " << N << endl;
                        N = not(N);
                        estado = EST2;
                        //estado = EST0;

                        time_backoff = MAX; // ALTERAR ESSES TIME_BACKOFF
                        cout<<"TIMEBACKOFF: "<<time_backoff<<endl;
                        return true;
                    } else {
                        estado = EST3;
                        time_backoff = rand()%(MAX - MIN +1)+MIN;
                        cout<<"TIMEBACKOFF: "<<time_backoff<<endl;
                        return false;
                    }
                    return false;
                } else { // Caso receba dados quando esta esperando ack------------------------------------------------------------

                    char * buf;
                    buf = new char [4096];
                    S_Quadro *qq;
                    qq = new S_Quadro;
                    memcpy(buf, e.ptr, e.num_bytes);

                    qq->q_ptr = buf;
                    qq->q_len = e.num_bytes;

                   // cout << "Enfileirou" << endl;
                    recebido.push(qq);

                    criaACK(e.ptr[0]);
                    cout<<"Enviou ACK"<<returnNumSeq(e.ptr[0])<<endl;
                    enquadra.envia(buff, 1);
                    //  M = returnNumSeq(e.ptr[0]);

                    return false;
                }
                return false;

            } else if (e.tipo == Timeout) {
                estado = EST3;
                time_backoff = rand()%(MAX - MIN +1)+MIN;
                cout<<"TIMEBACKOFF: "<<time_backoff<<endl;
                return false;

            } else if (e.tipo == Quadro) {
                // RESPONDER ACK COM NUMERO DE SEQUENCIA RECEBIDO

                if (!AckOuMensagem(e.ptr[0])) {//verifica se é quadro de mensagem

                    if (M == returnNumSeq(e.ptr[0])) {
                        criaACK(e.ptr[0]);
                        cout<<"Enviou ACK"<<returnNumSeq(e.ptr[0])<<endl;
                        enquadra.envia(buff, 1);
                        return false;
                    } else {
                        criaACK(e.ptr[0]);
                        cout<<"Enviou ACK"<<returnNumSeq(e.ptr[0])<<endl;
                        enquadra.envia(buff, 1);
                        M = returnNumSeq(e.ptr[0]);
                        return true;
                    }
                }
            }

            break;

        case EST2:
            estado = EST0;
            faz_backoff();

            return false;

            break;

        case EST3:
           // int bytes_receb;
           // struct timeval tv;
            //long int tInicio, tFim, tDecorrido;
            faz_backoff();

            if (e.tipo == Payload) {
                cout << "REENVIANDO POR ACK ERRADO" << endl;
                e.reenvio_bytes++;
                memset(e.ptr, '\0', e.reenvio_bytes);
                memcpy(e.ptr, buffer_reenvio, e.reenvio_bytes);
                enquadra.envia(e.ptr, e.reenvio_bytes);
                estado = EST1;
                return false;
            } else if (e.tipo == Timeout) {
                cout << "REENVIANDO POR TIMEOUT" << endl;
                e.reenvio_bytes++;
                memset(e.ptr, '\0', e.reenvio_bytes);
                memcpy(e.ptr, buffer_reenvio, e.reenvio_bytes);
                enquadra.envia(e.ptr, e.reenvio_bytes);
                estado = EST1;
                return false;
            }
            //            } else if (e.tipo == Quadro) {
            //                // RESPONDER ACK COM NUMERO DE SEQUENCIA RECEBIDO
            //
            //                if (!AckOuMensagem(e.ptr[0])) {//verifica se é quadro de mensagem
            //
            //                    if (M == returnNumSeq(e.ptr[0])) {
            //                        criaACK(e.ptr[0]);
            //                        enquadra.envia(buff, 1);
            //                        return false;
            //                    } else {
            //                        criaACK(e.ptr[0]);
            //                        enquadra.envia(buff, 1);
            //                        M = returnNumSeq(e.ptr[0]);
            //                        return true;
            //                    }
            //                }
            //            }
            break;
    }
}

void ARQ::retiraCabecalho(char * buffer, int bytes) {
    //memset(buffer_arq, '\0', bytes);

    //memcpy(buffer, buffer+1, bytes);

    for (int i = 0; i < bytes - 1; i++) {
        buffer_arq[i] = buffer[i + 1];
    }

    memcpy(buffer, buffer_arq, bytes - 1);
}

void ARQ::mudaPayload(char * buffer, int bytes, bool N) {
    //memset(buffer_arq, '\0', sizeof (buffer_arq));
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
    //  memset(buff, '\0', sizeof (buff));

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
        printf("%02X: ", line * 16);

        for (int n = 0; n < 16 and m < len; n++, m++) {
            int x = (unsigned char) buffer[m];
            printf("%02X ", x);
        }
        puts("");
        line++;
    }
}

void ARQ::faz_backoff(){
            int bytes_receb;
            struct timeval tv;
            long int tInicio, tFim, tDecorrido;
            tDecorrido = 0;
            gettimeofday(&tv, NULL);
            tInicio = tv.tv_sec * 1000 + tv.tv_usec / 1000;

            while (tDecorrido < time_backoff) {
                char * buffer_backup;
                buffer_backup = new char [4096];

                S_Quadro *q;
                q = new S_Quadro;

                bytes_receb = enquadra.recebe(buffer_backup, time_backoff - tDecorrido);

                gettimeofday(&tv, NULL);
                tFim = tv.tv_sec * 1000 + tv.tv_usec / 1000;
                tDecorrido = tFim - tInicio;


                if (bytes_receb > 0) {
                    if (!AckOuMensagem(buffer_backup[0])) { // Caso receba dados quando esta esperando ack
                       // cout << "Buffer backup: ";
                      //  imprimeHexa(buffer_backup, bytes_receb);

                        q->q_ptr = buffer_backup;
                        q->q_len = bytes_receb;
                        cout << "COLOCOU NA FILA" << endl;
                        //  cout << "Endereço fila:" << &q->q_ptr<<endl;
                        recebido.push(q);

                        //    imprimeHexa(recebido.front()->q_ptr,recebido.front()->q_len);

                        criaACK(buffer_backup[0]);
                        cout<<"Enviou ACK"<<returnNumSeq(buffer_backup[0])<<endl;
                        enquadra.envia(buff, 1);

                    }
                    //return false;
                }
            }
}