#ifndef ARQ_H
#define ARQ_H

#include <cstdint>
#include "Serial.h"
#include "Enquadramento.h"
#include <queue>

class ARQ {
public:
    ARQ(Enquadramento & enq, int bytes_min);
    //ARQ(const ARQ& orig);
    //virtual ~ARQ();
    
    void envia(char * buffer, int bytes);
    int recebe(char * buffer);
    
private:
    char buff[10];
    char buffer_arq[4096]; // quadros no maximo de 4 kB (hardcoded)
    char buffer_reenvio[4096]; // quadros no maximo de 4 kB (hardcoded)
    enum Estados {EST0,EST1,EST2,EST3};
    enum TipoEvento {Timeout,Quadro,Payload};
    int estado; // estado atual da MEF
    int min_bytes; // tamanhos mínimo e máximo de quadro
    bool N,M;
    Enquadramento & enquadra;
      
    struct Evento{
        TipoEvento tipo;
        char * ptr;
        char * quadro_recebido;
        int num_bytes;
    };
    
    bool handle(Evento e);
    void mudaPayload(char * buffer, int bytes, bool N); 
    void retiraCabecalho(char * buffer,int bytes);
    bool AckOuMensagem(char byte);
    bool returnNumSeq(char byte);
    void criaACK(char byte);
    void imprimeHexa(char * buffer, int len);
};

#endif /* ARQ_H */
