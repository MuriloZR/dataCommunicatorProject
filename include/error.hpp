#pragma once

#include <cstdint>
#include <vector>

// ============================================================
// Módulo de controle de erro — CRC-16 (polinômio 0x1021)
//
// Referência: Forouzan, "Data Communications and Networking",
//             5ª ed., Cap. 10, Seção 10.4 (Cyclic Codes).
//
// O CRC-16 trata o bloco de dados como um polinômio binário
// e o divide pelo polinômio gerador G(x) = x^16 + x^12 + x^5 + 1
// (representado como 0x1021 em hexadecimal).
// O resto dessa divisão — realizada em aritmética GF(2), onde
// subtração e adição são equivalentes a XOR — é o CRC.
// O receptor refaz o mesmo cálculo: se o resto for zero, o
// quadro chegou íntegro. Caso contrário, houve corrupção.
// ============================================================


// ============================================================
// crc16
// ============================================================
// Calcula o CRC-16 de um bloco de bytes usando o polinômio
// gerador 0x1021, segue o padrão do CRC-16/KERMIT.
//
// O cálculo é feito bit a bit sobre cada byte de `data`,
// processando do bit mais significativo (MSB) ao menos
// significativo (LSB). A cada iteração, o bit é incorporado
// ao registrador de 16 bits e o polinômio é aplicado via XOR
// quando o MSB do registrador é 1.
//
// Parâmetros:
//   data — bytes sobre os quais o CRC será calculado.
//          Deve cobrir todos os campos do quadro EXCETO
//          o próprio campo CRC e os bytes de flag (0x7E).
//
// Retorno:
//   uint16_t — valor do CRC de 16 bits.
//              Esse valor deve ser armazenado no campo `crc`
//              do Frame antes de serializar para envio.
// ============================================================
uint16_t crc16(const std::vector<uint8_t>& data);


// ============================================================
// verify_crc
// ============================================================
// Verifica se um bloco de bytes corresponde ao CRC recebido.
//
// Internamente, calcula o CRC-16 sobre o bloco codificado e
// verifica o valor da síndrome, caso seja 0, o bloco está íntegro
// caso não seja 0, algum erro ocorreu.
//
// Parâmetros:
//   data         — bytes do quadro recebido, excluindo os
//                  bytes de flag e o próprio campo CRC.
//   received_crc — valor do campo CRC extraído do quadro
//                  recebido após desserialização.
//
// Retorno:
//   true  — CRC confere; quadro provavelmente íntegro.
//   false — CRC diverge; quadro corrompido, deve ser
//           descartado. O receptor enviará NAK ou
//           simplesmente não enviará ACK, dependendo
//           da política adotada no módulo ARQ.
//
// Nota: CRC detecta erros mas NÃO os corrige. Um retorno
// false significa apenas que houve corrupção — não é possível
// recuperar o dado original a partir dessa informação.
// ============================================================
bool verify_crc(const std::vector<uint8_t>& data, uint16_t received_crc);
