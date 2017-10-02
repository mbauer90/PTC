#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <cstring>

#include "Protocolo.h"
#include "ARQ.h"
#include "Enquadramento.h"

using namespace std;

void dump(char * buffer, int len) {
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

int main(int argc, char* argv[]) {
    Serial dev("/dev/ttyUSB0", B9600);
    
    Enquadramento enq(dev, 255);
    ARQ arq(enq, 8);
    Protocolo proto(arq);
    char quadro[400];

    if (argc == 2) {
        if (!strcmp(argv[1], "1")) {
            //while (true){
                cout << "Digite algo para enviar: " << endl;
                cin >> quadro;

                proto.envia(quadro, strlen(quadro));
          //  }
                cout << "Digite algo para enviar: " << endl;
                cin >> quadro;

                proto.envia(quadro, strlen(quadro));
                
               int bytes = proto.recebe(quadro);
                dump(quadro, bytes);

                bytes = proto.recebe(quadro);
                dump(quadro, bytes);
//                
                bytes = proto.recebe(quadro);
                dump(quadro, bytes);
                

            
        } else {
            //            int bytes=1;
            //	    while(bytes>0){
            //		 bytes = proto.recebe(quadro);
            //		dump(quadro, bytes);
            //	    }

            while (true) {
                int bytes = proto.recebe(quadro);
                dump(quadro, bytes);
            }
        }
    } else {
        cout << "Passe como argumento 1 para enviar ou 2 para receber" << endl;
    }
}
