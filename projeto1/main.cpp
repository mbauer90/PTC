#include <iostream>
#include <fstream>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "Protocolo.h"
#include "ARQ.h"
#include "Enquadramento.h"

using namespace std;

void dump(char * buffer, int len) {
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

int main(int argc, char * argv[]) {
  Serial dev("/dev/ttyUSB0", B9600);
  
  Enquadramento enq(dev, 255);
  ARQ arq(enq,8);
  Protocolo proto(arq);
  char quadro[400];
  
//  strcpy(quadro, "0123456789");
//  proto.envia(quadro, 10);
////  
//  strcpy(quadro, "0124");
//  proto.envia(quadro, 4);
////  
////  //strcpy(quadro, "012345678998765432105432101234567899876543210543210123456789987654321054321012345678998765432105432101234567899876543210543210123456789987654321054321012345678998765432105432101234567899876543210543210123456789987654321054321012345678998765432105432101234567899876543210543210123456789987654321054321");
////  //proto.envia(quadro, 300);
////  
//  strcpy(quadro, "987654321098");
//  proto.envia(quadro, 12);
//  
//  
//  strcpy(quadro, "987654322298");
//  proto.envia(quadro, 12);
  //quadro[5] = 0x7e;
  //quadro[3] = 0x7d;
  //proto.envia(quadro, 10);
  //proto.envia("567~5678}0", 10);
//
    while(true){
        int bytes = proto.recebe(quadro);
        dump(quadro, bytes);
    }
  
}