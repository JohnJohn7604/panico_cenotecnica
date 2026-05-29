#include <WiFi.h>
#include <WiFiUdp.h>

// --- CONFIGURAÇÕES DA REDE WI-FI ---
const char* ssid = "ACER78";
const char* password = "123456789";

// IP de destino (Calculado dinamicamente com base no Universo)
IPAddress ipDestino(239, 255, 0, 7); 
unsigned int localPort = 5568;

WiFiUDP udp;

// --- CONFIGURAÇÕES DE BOTÕES/ESTÚDIO ---
const int BOTAO_PANICO = 0;       // Botão BOOT do ESP32
const int BOTAO_UP = 25;
const int BOTAO_DOWN = 14;
const int BOTAO_CONFIRM = 26;

const int PINO_LED_VERDE = 12;    // LED de Pânico Ativado
const int PINO_LED_VERMELHO = 27; // LED de Pânico Desativado (Controle da Mesa)

// --- VARIÁVEIS DE CONTROLE DE ESTADO ---
int UNIVERSO = 7;                // Começa no 7 por padrão
bool universoConfirmado = false; // Bloqueia o pânico até que confirme o universo
bool estadoLuz = false; 
byte seqNum = 0;

unsigned long tempoDesligamento = 0;
unsigned long tempoUltimoCliqueBotoes = 0; // Debounce dos botões de seleção
unsigned long ultimoClique = 0;            // Debounce do botão de pânico
unsigned long tempoPiscaLED = 0;           // Controla o tempo do pisca do LED Vermelho

bool emTransicaoOff = false;
byte prioridadeAtual = 50;       // Começa entregando o controle pra mesa
bool mensagemEnviada = false;    // Controla para o print rodar apenas uma vez
bool estadoLedPisca = LOW;       // Guarda se o LED Vermelho está HIGH ou LOW

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
    pinMode(BOTAO_UP, INPUT_PULLUP);
    pinMode(BOTAO_DOWN, INPUT_PULLUP);
    pinMode(BOTAO_CONFIRM, INPUT_PULLUP);

    pinMode(PINO_LED_VERDE, OUTPUT);
    pinMode(PINO_LED_VERMELHO, OUTPUT);

    digitalWrite(PINO_LED_VERDE, LOW);
    digitalWrite(PINO_LED_VERMELHO, LOW);

    Serial.print("Conectando ao Wi-Fi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    
    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print("."); 
        tentativas++;
        if(tentativas > 30) { 
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

    Serial.println("=============================================");
    Serial.println("SELEÇÃO DE UNIVERSO");
    Serial.print("Use UP/DOWN. Universo atual: "); Serial.println(UNIVERSO);
    Serial.println("Pressione UP, DOWN ou CONFIRM para iniciar o sistema.");
    Serial.println("=============================================\n");

    // Enquanto nenhum botão for pressionado, fica travado aqui E pisca o LED de configuração
    while (digitalRead(BOTAO_UP) == HIGH && 
           digitalRead(BOTAO_DOWN) == HIGH && 
           digitalRead(BOTAO_CONFIRM) == HIGH) {
        
        // Faz o LED piscar mesmo travado aqui dentro no início
        unsigned long m = millis();
        if (m - tempoPiscaLED >= 250) {
            estadoLedPisca = !estadoLedPisca;
            digitalWrite(PINO_LED_VERMELHO, estadoLedPisca);
            tempoPiscaLED = m;
        }
        delay(10); 
    }
    
    Serial.println(">>> Interacao detectada! Modo Selecao Liberado. <<<\n");
}

int estadoAnteriorBotao = HIGH;

void loop() {
    unsigned long tempoAtual = millis();

    // --- MÁQUINA DE ESTADOS: MODO SELEÇÃO (TROCA DE UNIVERSO) ---
    if (!universoConfirmado) {
        digitalWrite(PINO_LED_VERDE, LOW);

        // LED Vermelho pisca rápido indicando "Modo Troca / Configuração"
        if (tempoAtual - tempoPiscaLED >= 250) {
            estadoLedPisca = !estadoLedPisca;
            digitalWrite(PINO_LED_VERMELHO, estadoLedPisca);
            tempoPiscaLED = tempoAtual;
        }

        // Botão SUBIR Universo
        if (digitalRead(BOTAO_UP) == LOW && (tempoAtual - tempoUltimoCliqueBotoes > 250)) {
            UNIVERSO++;
            if (UNIVERSO > 63999) UNIVERSO = 63999; 
            Serial.print("[CONFIG] Universo Selecionado: "); Serial.println(UNIVERSO);
            tempoUltimoCliqueBotoes = tempoAtual;
        }

        // Botão DESCER Universo
        if (digitalRead(BOTAO_DOWN) == LOW && (tempoAtual - tempoUltimoCliqueBotoes > 250)) {
            UNIVERSO--;
            if (UNIVERSO < 1) UNIVERSO = 1; 
            Serial.print("[CONFIG] Universo Selecionado: "); Serial.println(UNIVERSO);
            tempoUltimoCliqueBotoes = tempoAtual;
        }

        // Botão CONFIRMAR Universo (TRAVA O SISTEMA E PARA DE PISCAR)
        if (digitalRead(BOTAO_CONFIRM) == LOW && (tempoAtual - tempoUltimoCliqueBotoes > 300)) {
            universoConfirmado = true;
            
            // Injeta o universo selecionado no cabeçalho sACN
            pacote_sACN[113] = (UNIVERSO >> 8) & 0xFF;
            pacote_sACN[114] = UNIVERSO & 0xFF;

            // Recalcula dinamicamente o IP Multicast alvo
            ipDestino = IPAddress(239, 255, (UNIVERSO >> 8) & 0xFF, UNIVERSO & 0xFF);
            
            // LED Fica ACESO FIXO indicando pronto/trava realizada
            digitalWrite(PINO_LED_VERMELHO, HIGH); 
            tempoUltimoCliqueBotoes = tempoAtual;
        }
    } 
    
    // --- MÁQUINA DE ESTADOS: MODO OPERAÇÃO (SISTEMA ARMADO) ---
    else {
        // PRINT ÚNICO: Mostra a mensagem uma única vez e para
        if (!mensagemEnviada) {
            Serial.print("\n[OK] UNIVERSO TRAVADO EM: "); Serial.println(UNIVERSO);
            Serial.print("[REDE] IP Alvo Multicast reconfigurado para: "); Serial.println(ipDestino);
            Serial.println(">>> SINAL LIBERADO: Pânico pronto para operar! <<<\n");
            mensagemEnviada = true; 
        }

        // Se quiser trocar o universo de novo, pressiona CONFIRM para voltar ao modo troca
        if (digitalRead(BOTAO_CONFIRM) == LOW && (tempoAtual - tempoUltimoCliqueBotoes > 300)) {
            universoConfirmado = false;
            mensagemEnviada = false; // Permite printar de novo na próxima confirmação
            estadoLuz = false; 
            prioridadeAtual = 50;    // Devolve a linha para a mesa antes de sair
            Serial.println("\n[RESET] Modo Selecao Ativo. Escolha o universo...");
            tempoUltimoCliqueBotoes = tempoAtual;
            return; 
        }

        // 1. Leitura do Botão de Pânico (BOOT)
        int leituraAtual = digitalRead(BOTAO_PANICO); 
        if (leituraAtual == LOW && estadoAnteriorBotao == HIGH) {
            if (tempoAtual - ultimoClique > 300) { 
                estadoLuz = !estadoLuz; 
                
                if (estadoLuz) {
                    // AÇÃO: LIGAR O PÂNICO
                    prioridadeAtual = 200; 
                    digitalWrite(PINO_LED_VERDE, HIGH);    
                    digitalWrite(PINO_LED_VERMELHO, LOW);  // Desliga o vermelho enquanto o pânico atua
                    emTransicaoOff = false; 
                    for (int i = 1; i <= 512; i++) pacote_sACN[125 + i] = 129; 
                    Serial.println("[ALERTA] PANICO ATIVADO NO ESTÚDIO!");
                } else {
                    // AÇÃO: DESLIGAR O PÂNICO
                    prioridadeAtual = 200; 
                    digitalWrite(PINO_LED_VERDE, LOW);     
                    digitalWrite(PINO_LED_VERMELHO, HIGH); // Restaura o vermelho fixo (Sistema em espera)
                    emTransicaoOff = true; 
                    tempoDesligamento = tempoAtual; 
                    for (int i = 1; i <= 512; i++) pacote_sACN[125 + i] = 0; 
                    Serial.println("Panico liberado. Forçando blackout...");
                }
                ultimoClique = tempoAtual;
            }
        }
        estadoAnteriorBotao = leituraAtual;

        // 2. Cronômetro de Meio Segundo (500ms) para limpar a rede e liberar para a mesa
        if (emTransicaoOff && (tempoAtual - tempoDesligamento >= 500)) {
            prioridadeAtual = 50; 
            emTransicaoOff = false; 
            Serial.println("[Linha devolvida com sucesso para a mesa. Panico desativado");
        }

        // 3. Aplica a Prioridade Dinâmica no Pacote sACN
        pacote_sACN[108] = prioridadeAtual;

        // 4. O Cronômetro de Meio Segundo (500ms) para limpar a rede e devolver pra mesa
        if (emTransicaoOff && (tempoAtual - tempoDesligamento >= 500)) {
            prioridadeAtual = 50; 
            emTransicaoOff = false; 
            
            // MÁGICA AQUI: Libera para o print rodar novamente na tela!
            mensagemEnviada = false; 
            
            Serial.println("[Linha devolvida com sucesso para a mesa. Panico desativado");
        }

        // 5. Heartbeat DMX (40fps)
        delay(25); 
    }
}
