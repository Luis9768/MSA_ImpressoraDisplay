#include "PrinterManager.h"
#include "Config.h"

HardwareSerial printerSerial(2);

void setupPrinter() {
    // Inicializa Serial 2 para a Impressora (TX=27)
    printerSerial.begin(PRINTER_BAUD, SERIAL_8N1, -1, PRINTER_TX_PIN);
    Serial.println(">>> PrinterManager: Serial Iniciada (TX=27, 9600)");
}

void imprimirEtiqueta(Receita r, int contador) {
    if (r.id == 0) return;

    // Dados Simulados para teste (já que o Slave não tem tela de RE/Data)
    // Em um cenário real, esses dados viriam de variaveis globais ou da Receita
    int reValor = 1234;
    int dia = 10, mes = 12, ano = 25;
    unsigned long codigoProd = atol(r.codigo);

    // Buffer para o comando ZPL
    constexpr size_t ZPL_BUFFER_SIZE = 2048;
    char zplBuffer[ZPL_BUFFER_SIZE];

    int escrito = snprintf(
        zplBuffer,
        ZPL_BUFFER_SIZE,
        "CT~~CD,~CC^~CT~\r\n"
        "^XA~TA000~JSN^LT0^MNW^MTT^PON^PMN^LH0,0^JMA^PR2,2~SD25^JUS^LRN^CI0^XZ\r\n"
        "^XA\r\n"
        "^CI28\r\n"
        "^MMT\r\n"
        "^POI\r\n"
        "^PW400\r\n"
        "^LL0599\r\n"
        "^LS0\r\n"
        "^FO0,0^GFA,01920,01920,00020,:Z64:"
        "eJzt1LFrU0EcB/Df9TT3moY8M4S0EH0hASkO+opLQeRlEVcHsykUFFxTirSD5Z0VNEMHR0ELBRfJX+B4oliHDi7iovSmztepb3jN+fvde6+0qYPQ0skfgVy++by7e793CcD/Ol1dcq/j1QLw3GATynnEFEAzHzTzjEuAaTfI37EE0JgxGsw5D1DyYPPjANZk1ZttfZgte3idaN7Z23t3d99caT5YfJY2pj8BTHlf23OD3VZYrpQ6A8nevAAIZn4uz13+1Q8bzcbShqzXPpNbv9rxvrXCtne9MymnXq8B1Grbj5dmtvvhcuXLkxn1tLYF0Hk72H1/a12HHW/QDnX7JrrFjZ2tpZ0t1X+0erBv+vHtV3i7njcBA4AF3LmWoTf58rATFxVmXegDa3SLzJeYSa6BR7LI6thFoVgXeP30j+Rf6uHv8VJgT5SE+ER29m5EUldxVxWwCbcK+xJTZuikNMGmDDOcz7lp56KE5Q5n/J65mLIYncRMzecOP5JTlK04F5nCXRsOJbkqRLpwDA/fiqCNHTpFHVvxXXboXOZcoLuxc5qyhJyO9EIkY3QGI5aQM4Hqx3LM+UqPOx1gRgtYUx4OeYJf3NC+REf9M8KmLmuY4LkJGLmkyDi5AMilmIkUM4HIBHQT1NukcMz4hUvEAS3uHCOXovNHlOkqT6qZY9b49kfvPl7IU9+5kcusHXGDruScxSyg54pOpCJzeMvOMVPh6UTmMAuy7IJIS85RFrn5Ek+MJljmevfqqz1aF8QIMidxb/OuYeBTRk4dyeyY487xYw6bE1jMyKkjzh5xdF5854RzOnd4qiIr+Tn9Pv7meieqe9Z/EedRfwAl7WoT:0C11\r\n"
        "^FT375,547^A0I,32,31^FH\\^FDCODIGO: %06lu^FS\r\n"
        "^FT375,523^A0I,20,19^FH\\^FD%s^FS\r\n"
        "^BY3,2,85^FT329,423^BEI,,Y,N\r\n"
        "^FD%s^FS\r\n" 
        "^FO13,385^GB371,0,8^FS\r\n"
        "^FT356,330^A0I,31,31^FH\\^FDFAB:^FS\r\n"
        "^SL0\r\n"
        "^FT287,330^A0I,31,31\r\n"
        "^FC%%,{,#\r\n"
        "^FD%02u/%02u/%02u^FS\r\n"
        "^FT341,287^A0I,31,31^FH\\^FDRE:^FS\r\n"
        "^FT287,287^A0I,31,31^FH\\^FD%04u^FS\r\n"
        "^FT372,233^A0I,31,31^FH\\^FDSEQUENCIAL:^FS\r\n"
        "^FT188,233^A0I,31,31^FH\\^FD%03lu^FS\r\n"
        "^PQ1,0,1,Y^XZ\r\n",
        (unsigned long)codigoProd,
        r.descricao,
        r.barcode[0] ? r.barcode : "0000000000000",
        (unsigned int)dia,
        (unsigned int)mes,
        (unsigned int)ano,
        reValor,
        (unsigned long)contador);

    if (escrito > 0 && escrito < (int)ZPL_BUFFER_SIZE) {
        // Envia para impressora real
        printerSerial.write(reinterpret_cast<const uint8_t *>(zplBuffer), (size_t)escrito);
        printerSerial.flush();

        // SIMULACAO NO TERMINAL
        Serial.println("\n>>> [SIMULACAO IMPRESSORA] ZPL ENVIADO: <<<");
        Serial.println(zplBuffer);
        Serial.println(">>> [FIM ZPL] <<<\n");
    } else {
        Serial.println("Erro ao montar ZPL!");
    }
}
