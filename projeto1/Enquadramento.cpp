#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <string>
#include <iostream>
#include <assert.h>
#include <memory.h>
#include <queue>
#include <bitset>

#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>

#include "Enquadramento.h"

using namespace std;

Enquadramento::Enquadramento(Serial & dev, int bytes_max) : porta(dev) {
    //min_bytes = bytes_min;
    max_bytes = bytes_max;
    estado = Ocioso;
}

void Enquadramento::envia(char * buffer, int bytes) {
    char c, d, e, preenc;
    c = 0x7e;
    d = 0x7d;
    preenc = 0x00;
    string squadro;

    //CRC
    gen_crc(buffer, bytes);
    //buffer[1] = 0x42;
    bytes = bytes + 2;

    squadro = c;

    for (int i = 0; i < bytes; i++) {
        if ((buffer[i] == 0x7e) | (buffer[i] == 0x7d)) {
            e = (buffer[i] xor 0x20);
            squadro = squadro + d + e;
        } else {
            squadro = squadro + buffer[i];
        }

    }

    squadro = squadro + c;
    //cout << squadro << endl;

    //WRITE IN SERIAL
    for (char & c : squadro) {
        porta.write(&c, 1);
    }
}

char Enquadramento::recv_byte(int tout_ms) {
    char byte;
    // cria um conjunto de descritores
    fd_set r;
    // inicia o conjunto de descritores, e nele
    // acrescenta fd
    FD_ZERO(&r);
    FD_SET(porta.get(), &r);

    timeval timeout = {tout_ms / 1000, (tout_ms % 1000)*1000};
    int n = select(porta.get() + 1, &r, NULL, NULL, &timeout);

    if (n > 0) { // algo foi digitado dentro do prazo 
        porta.read(&byte, 1, true);
        return byte;
    } else {
        cout << "Timeout" << endl;
        throw -1;
    }
}

// tout: timeout em segundos

int Enquadramento::recebe(char * buffer, int tout) {
    char byte;
    timeout_bytes = false;
    timeout_ms = tout * 1000;
    int tam_byte;

    try {
        byte = recv_byte(tout * 1000);
        handle(byte);
    } catch (...) {
        return 0; // timeout
    }

    while (true) {
        try {
            byte = recv_byte(1000);
        } catch (...) {
            return 0; // timeout
        }
        if (handle(byte)) {
            buffer_maq[cont_buffer] = '\0';
            buffer_maq[cont_buffer - 1] = '\0';
            cont_buffer = cont_buffer - 2;

            memcpy(buffer, buffer_maq, cont_buffer);

            return cont_buffer;
        }
    }
}

bool Enquadramento::handle(char byte) {
    switch (estado) {
        case Ocioso: // estado Ocioso

            cont_buffer = 0;
            memset(buffer_maq, '\0', sizeof (buffer_maq));

            if (byte == 0x7e) {
                estado = RX; // muda para RX
            }
            return false;
            break;
        case RX: // estado RX
            if (byte == 0x7d) {
                estado = ESC; // muda para ESC
                return false;
            } else if ((byte == 0x7e) and (cont_buffer == 0)) { //(byte == 0x7d) e len == 0
                estado = RX;
                return false;
            } else if (((byte == 0x7e) and (cont_buffer != 0)) or (timeout_bytes)) { //PRECISA COLOCAR TIMEOUT
                estado = Ocioso;

                if (timeout_bytes) {
                    timeout_bytes = false;
                    return false;
                }

                //if ((cont_buffer<min_bytes) | (!check_crc(buffer_maq,cont_buffer))){
                if ((!check_crc(buffer_maq, cont_buffer))) {
                    return false;
                } else {
                    return true;
                }

            } else {
                if (cont_buffer > max_bytes) {
                    estado = Ocioso;
                } else {
                    estado = RX;
                    buffer_maq[cont_buffer] = byte;
                    cont_buffer++;
                }
                return false;
            }
            break;
        case ESC: // estado ESC
            if (((byte == 0x5e) or (byte == 0x5d)) and (timeout_bytes == false)) { //PRECISA COLOCAR TIMEOUT/DESCARTAR
                estado = RX; // muda para RX
                buffer_maq[cont_buffer] = (byte xor 0x20);
                cont_buffer++;
                return false;
            } else {
                estado = Ocioso; // muda para Ocioso
                timeout_bytes = false;
                return false;
            }
            break;
    }
}

// verifica o CRC do conteúdo contido em "buffer". Os dois últimos 
// bytes desse buffer contém o valor de CRC

bool Enquadramento::check_crc(char * buffer, int len) {
    uint16_t crcteste;
    uint8_t parte1, parte2, parte1teste, parte2teste;

    parte1 = buffer[len - 1];
    parte2 = buffer[len - 2];

    //cout << bitset<8>(parte1)  << endl;
    //cout << bitset<8>(parte2)  << endl;

    crcteste = pppfcs16(PPPINITFCS16, buffer, len - 2);

    parte1teste = ((crcteste >> 8) & 0x00ff);
    parte2teste = (crcteste & 0x00ff); /* least significant byte first */

    //cout << bitset<8>(parte1teste)  << endl;
    //cout << bitset<8>(parte2teste)  << endl;

    if ((parte1 == parte1teste) and (parte2 == parte2teste)) {
        return true;
    } else {
        return false;
    }

}

// gera o valor de CRC dos bytes contidos em buffer. O valor de CRC
// é escrito em buffer[len] e buffer[len+1]

void Enquadramento::gen_crc(char * buffer, int len) {
    uint16_t crcgerado = pppfcs16(PPPINITFCS16, buffer, len);
    buffer[len] = (crcgerado & 0x00ff); /* least significant byte first */
    buffer[len + 1] = ((crcgerado >> 8) & 0x00ff);

}

// calcula o valor de CRC dos bytes contidos em "cp".
// "fcs" deve ter o valor PPPINITFCS16
// O resultado é o valor de CRC (16 bits)
// OBS: adaptado da RFC 1662 (enquadramento no PPP)

uint16_t Enquadramento::pppfcs16(uint16_t fcs, char * cp, int len) {
    assert(sizeof (u16) == 2);
    assert(((u16) - 1) > 0);
    while (len--)
        fcs = (fcs >> 8) ^ fcstab[(fcs ^ *cp++) & 0xff];
    return (fcs);
}


