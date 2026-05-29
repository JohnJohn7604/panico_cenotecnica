#include <WiFi.h>
#include <WiFiUdp.h>

// --- CONFIGURAÇÕES DA REDE WI-FI ---
const char* ssid = "ACER78";
const char* password = "123456789";

// IP de destino
IPAddress ipDestino(239, 255, 0, 7); // IP UNIVERSAL DO UNIVERSO 7 MULTICAST 
unsigned int localPort = 5568;

WiFiUDP udp;

// --- CONFIGURAÇÕES DO ESTÚDIO ---
const int BOTAO_PANICO = 0;       // Configurado para o botão BOOT
const int PINO_LED_VERDE = 12;    // LED de Pânico Ativado
const int PINO_LED_VERMELHO = 27; // LED de Pânico Desativado (Controle da Mesa)
const int UNIVERSO = 7;

bool estadoLuz = false; 
byte seqNum = 0;
unsigned long tempoDesligamento = 0;
bool emTransicaoOff = false;
byte prioridadeAtual = 50;        // Começa entregando o controle pra mesa

// --- VARIÁVEIS DO PISCA DO LED VERMELHO ---
unsigned long tempoPiscaVermelho = 0; // Armazena o último instante que o LED mudou de estado
bool estadoLedVermelho = HIGH;        // Guarda o estado atual do LED Vermelho

byte pacote_sACN[638]; 

void setup_sACN() {
    memset(pacote_sACN, 0, 638);
    pacote_sACN[0] = 0x00; pacote_sACN[1] = 0x10; pacote_sACN[2] = 0x00; pacote_sACN[3] = 0x00;
    byte pid[] = {0x41, 0x53, 0x43, 0x2d, 0x45, 0x31, 0x2e, 0x31, 0x37, 0x00, 0x00, 0x00};
    memcpy(&pacote_sACN[4], pid, 12);
    pacote_sACN[16] = 0x72; pacote_sACN[17] = 0x6e;
    pacote_sACN[18] = 0x00; pacote_sACN[19] = 0x00; pacote_sACN[20] = 0x00; pacote_sACN[21] = 0x04;
    for(int i=22; i<38; i++) pacote_sACN[i] = 0x01; 
    pacote_sACN[38] = 0x72; pacote_sACN[39] = 0x58;
    pacote_sACN[40] = 0x00; pacote_sACN[41] = 0x00; pacote_sACN[42] = 0x00; pacote_sACN[43] = 0x02;
    strcpy((char*)&pacote_sACN[44], "Botao Panico K");
    pacote_sACN[108] = prioridadeAtual; 
    pacote_sACN[113] = (UNIVERSO >> 8) & 0xFF;
    pacote_sACN[114] = UNIVERSO & 0xFF;
    pacote_sACN[115] = 0x72; pacote_sACN[116] = 0x0B;
    pacote_sACN[117] = 0x02; pacote_sACN[118] = 0xA1;
    pacote_sACN[119] = 0x00; pacote_sACN[120] = 0x00;
    pacote_sACN[121] = 0x00; pacote_sACN[122] = 0x01;
    pacote_sACN[123] = 0x02; pacote_sACN[124] = 0x01;
    pacote_sACN[125] = 0x00; 
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n--- Iniciando Sistema de Panico Est K---");

    pinMode(BOTAO_PANICO, INPUT_PULLUP);

    pinMode(PINO_LED_VERDE, OUTPUT);
    pinMode(PINO_LED_VERMELHO, OUTPUT);

    // Estado inicial: Pânico desligado (Vermelho começa aceso)
    digitalWrite(PINO_LED_VERDE, LOW);
    digitalWrite(PINO_LED_VERMELHO, HIGH);

    Serial.print("Conectando ao Wi-Fi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    
    int tentatives = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print("."); 
        tentatives++;
        if(tentatives > 30) { 
            Serial.println("\n[ERRO] Nao foi possivel conectar ao Wi-Fi. Verifique SSID/Senha.");
            break;
        }
    }

    if(WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[SUCESSO] Wi-Fi Conectado!");
        Serial.print("IP do ESP32: ");
        Serial.println(WiFi.localIP()); 
    }

    udp.begin(localPort);
    setup_sACN();
}

unsigned long ultimoClique = 0;
int estadoAnteriorBotao = HIGH;

void loop() {
    unsigned long tempoAtual = millis();

    // 1. Leitura do Botão (O pino 0 é o 'Boot' do ESP32)
    int leituraAtual = digitalRead(BOTAO_PANICO); 
    if (leituraAtual == LOW && estadoAnteriorBotao == HIGH) {
        if (tempoAtual - ultimoClique > 300) { // Debounce
            estadoLuz = !estadoLuz; 
            
            if (estadoLuz) {
                // AÇÃO: LIGAR O PÂNICO
                prioridadeAtual = 100;                 
                digitalWrite(PINO_LED_VERDE, HIGH);    // Acende Verde fixo
                digitalWrite(PINO_LED_VERMELHO, LOW);  // Apaga Vermelho (para de piscar)
                emTransicaoOff = false;                // Cancela qualquer transição de desligamento
                for (int i = 1; i <= 512; i++) pacote_sACN[125 + i] = 129; // Output 50%
            } else {
                // AÇÃO: DESLIGAR O PÂNICO (Início da janela de carência)
                prioridadeAtual = 100;                 // Mantém prioridade alta enviando zero para limpar a rede
                digitalWrite(PINO_LED_VERDE, LOW);     // Apaga Verde
                emTransicaoOff = true;                 // Inicia o cronômetro de devolução
                tempoDesligamento = tempoAtual;        // Grava o milissegundo exato do clique
                for (int i = 1; i <= 512; i++) pacote_sACN[125 + i] = 0; // Força zero nos 512 canais
            }
            ultimoClique = tempoAtual;
        }
    }
    estadoAnteriorBotao = leituraAtual;

    // 2. O Cronômetro de Meio Segundo (500ms) para liberar a rede
    if (emTransicaoOff && (tempoAtual - tempoDesligamento >= 500)) {
        prioridadeAtual = 50;    // Derruba a prioridade para 50 para a mesa assumir
        emTransicaoOff = false;  // Finaliza a transição
    }

    // 3. Lógica do Pisca do LED Vermelho (Funciona apenas se o pânico estiver DESLIGADO)
    if (!estadoLuz) {
        // A cada 500ms (meio segundo), inverte o estado do pino do LED Vermelho
        if (tempoAtual - tempoPiscaVermelho >= 500) {
            estadoLedVermelho = !estadoLedVermelho;
            digitalWrite(PINO_LED_VERMELHO, estadoLedVermelho);
            tempoPiscaVermelho = tempoAtual;
        }
    }

    // 4. Aplica a Prioridade Dinâmica no Pacote sACN
    pacote_sACN[108] = prioridadeAtual;

    // 5. Atualiza a sequência e envia via UDP
    pacote_sACN[111] = seqNum++;
    udp.beginPacket(ipDestino, 5568);
    udp.write(pacote_sACN, 638);
    udp.endPacket();

    // 6. Heartbeat DMX (40fps)
    delay(25); 
}
