#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include "ARQ.h"

class Protocolo {
public:
    Protocolo(ARQ & a);
    //Protocolo(const Protocolo& orig);
    //virtual ~Protocolo();
    
    void envia(char * buffer, int bytes);
    int recebe(char * buffer);
    
private:
  //char buffer_proto[4096]; 
  ARQ & arq;
  
};

#endif /* PROTOCOLO_H */

