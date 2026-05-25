#include <WiFi.h>
#include <WiFiUdp.h>

// --- CONFIGURAÇÕES DA REDE WI-FI ---
const char* ssid = "COLOCAR SSID";
const char* password = "#dmx-kl#";

// IP de destino
IPAddress ipDestino(239, 255, 0, 7); 
unsigned int localPort = 5568;

WiFiUDP udp;

// --- CONFIGURAÇÕES DO ESTÚDIO ---
const int BOTAO_PANICO = 0; // Configurado para o botão BOOT
const int PINO_LED_VERDE = 12;    // LED de Pânico Ativado
const int PINO_LED_VERMELHO = 27; // LED de Pânico Desativado (Controle da Mesa)
const int UNIVERSO = 7;
bool estadoLuz = false; 
byte seqNum = 0;
unsigned long tempoDesligamento = 0;
bool emTransicaoOff = false;
byte prioridadeAtual = 50; // Começa entregando o controle pra mesa

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
    pacote_sACN[108] = 200; // Prioridade Máxima
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
    // Abre a comunicação de diagnóstico com o PC
    Serial.begin(115200);
    delay(500);
    Serial.println("\n--- Iniciando Sistema de Panico Est K---");

    pinMode(BOTAO_PANICO, INPUT_PULLUP);

    // Configura os LEDs como saída
    pinMode(PINO_LED_VERDE, OUTPUT);
    pinMode(PINO_LED_VERMELHO, OUTPUT);

    // Estado inicial: Sistema desligado, controle na mesa
    digitalWrite(PINO_LED_VERDE, LOW);
    digitalWrite(PINO_LED_VERMELHO, HIGH);

    // --- SE A REDE EXIGIR IP FIXO, BASTA DESCOMENTAR E PREENCHER AQUI ---
    // IPAddress ipESP(10, 10, 7, 200);      // IP que a equipe de redes liberar pra você
    // IPAddress gateway(10, 10, 7, 1);      // Gateway da rede de luz
    // IPAddress subnet(255, 255, 255, 0);   // Máscara de sub-rede
    // WiFi.config(ipESP, gateway, subnet);
    // --------------------------------------------------------------------

    // Conecta no Wi-Fi
    Serial.print("Conectando ao Wi-Fi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    
    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print("."); // Vai imprimindo pontinhos enquanto tenta conectar
        tentativas++;
        if(tentativas > 30) { // Se passar de 15 segundos sem conectar, avisa
            Serial.println("\n[ERRO] Nao foi possivel conectar ao Wi-Fi. Verifique SSID/Senha.");
            break;
        }
    }

    if(WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[SUCESSO] Wi-Fi Conectado!");
        Serial.print("IP do ESP32: ");
        Serial.println(WiFi.localIP()); // Mostra o IP que a placa ganhou
    }

    udp.begin(localPort);
    setup_sACN();
}

unsigned long ultimoClique = 0;
int estadoAnteriorBotao = HIGH;

void loop() {
    // 1. Leitura do Botão (O pino 0 é o 'Boot' do ESP32)
    int leituraAtual = digitalRead(0); 
    if (leituraAtual == LOW && estadoAnteriorBotao == HIGH) {
        if (millis() - ultimoClique > 300) { //Debounce
            estadoLuz = !estadoLuz; 
            
            if (estadoLuz) {
                // AÇÃO: LIGAR O PÂNICO
                prioridadeAtual = 200; // Assume o controle na marra
                digitalWrite(PINO_LED_VERDE, HIGH);    // Acende Verde
                digitalWrite(PINO_LED_VERMELHO, LOW);  // Apaga Vermelho
                emTransicaoOff = false; // Cancela qualquer transição
                for (int i = 1; i <= 512; i++) pacote_sACN[125 + i] = 129; // Output 50%
            } else {
                // AÇÃO: DESLIGAR O PÂNICO (Início dos 3 segundos)
                prioridadeAtual = 50; // Mantém a prioridade alta para forçar o apagão
                digitalWrite(PINO_LED_VERDE, LOW);     // Apaga Verde
                digitalWrite(PINO_LED_VERMELHO, HIGH); // Acende Vermelho
                emTransicaoOff = true; // Inicia o cronômetro
                tempoDesligamento = millis(); // Grava a hora exata do clique
                for (int i = 1; i <= 512; i++) pacote_sACN[125 + i] = 0; // Força zero
            }
            ultimoClique = millis();
        }
    }
    estadoAnteriorBotao = leituraAtual;

    // 2. O Cronômetro de Meio Segundo (500ms)
    if (emTransicaoOff && (millis() - tempoDesligamento >= 500)) {
        // Passaram os 500ms cravados
        prioridadeAtual = 50; // Devolve o controle pra mesa
        emTransicaoOff = false; // Finaliza a transição
    }

    // 3. Aplica a Prioridade Dinâmica no Pacote sACN
    pacote_sACN[108] = prioridadeAtual;

    // 4. Atualiza a sequência e envia via Wi-Fi/Ethernet
    pacote_sACN[111] = seqNum++;
    udp.beginPacket(ipDestino, 5568);
    udp.write(pacote_sACN, 638);
    udp.endPacket();

    // 5. Heartbeat DMX (40fps)
    delay(25); 
}