# panico_cenotecnica
Um botao de panico para a luz cenotecnica, feito com o microcontrolador ESP32 se comunincando em sACN via rede.

Projeto: Botão de Pânico sACN - Cenotécnica 

Hardware e Proposta
O projeto foi prototipado utilizando o microcontrolador ESP32 na versão Wi-Fi. Caso seja aprovado pela gestão e pelo suporte, a versão final de produção utilizará a placa industrial WT32-ETH01, que integra o ESP32 a uma porta de rede RJ45, garantindo máxima estabilidade física.

Comunicação
O sistema comunica-se com todos os endereços (1 ao 512) do Universo 7 utilizando o protocolo nativo sACN, através da conexão direta com a VLAN de iluminação do estúdio.

Funcionamento
Uma vez conectado à rede (Wi-Fi ou Ethernet), basta acionar o botão para definir o Output de todo o universo para o valor 129 (~50% de intensidade), assumindo o controle independentemente de haver faders levantados na mesa.

Lógica de Prioridades: Ao ser ligado, o sistema assume prioridade 200 (sobrepondo-se à mesa). Ao ser desligado, ele zera as luzes e reduz a prioridade para 50, devolvendo o controle da rede imediatamente para o console.

Validação e Testes de Bancada

Antes do comissionamento na rede oficial (VLAN) de iluminação do estúdio, o *firmware* do ESP32 foi rigorosamente testado em um ambiente de rede local (WLAN) isolado. 

Para auditar o tráfego de rede e confirmar a integridade dos pacotes gerados em baixo nível, utilizamos o software de monitoramento **sACN View**. 

Durante a validação em bancada, os seguintes parâmetros foram homologados com sucesso:

* **Transmissão de Pacotes:** Confirmação do encapsulamento correto dos dados sACN (pacotes exatos de 638 bytes) trafegando via protocolo UDP.
* **Roteamento de Universo:** Verificação de que o sinal estava sendo direcionado com precisão para o **Universo 7**, conforme exigido pela arquitetura do estúdio.
* **Manipulação de Payload (Canais de Dimmer):** Comprovação em tempo real da alteração simultânea dos 512 endereços DMX. Ao acionar o sistema, o *sACN View* registrou os canais subindo para o valor `129` (~50% de intensidade).
* **Gestão de Prioridades:** Validação da máquina de estados, comprovando a injeção da Prioridade `200` ao ativar o botão (para assumir o controle) e a queda para a Prioridade `50` com valor de canal `0` ao desativar (para devolver o controle).

Nota: No protótipo atual, estamos usando o botão 'Boot' da placa para demonstração do teste da máquina de estados.

Benefícios Operacionais

Baixo custo de implementação.

Independência Operacional: O sistema pode ficar ligado 24h. Pela manhã, assim que o eletricista bater os disjuntores da cenotécnica e os Nodes iniciarem, basta a equipe de cenografia/arte acionar o botão para acender os cenários, sem depender de alguém para ligar a mesa de luz.

Versão Final de Produção (Se Aprovado)

Implementação da placa WT32-ETH01 via cabo de rede.

Eletrônica embarcada com fonte 5V dedicada.

Substituição do botão de teste por uma botoeira industrial vermelha com iluminação integrada (o próprio botão acende para indicar que o pânico está ativado).

Pendências Técnicas (Alinhamento com Suporte)

Confirmar com a equipe de redes o IP Unicast (específico) do Node do Universo 7 e uma faixa de IP livre para o ESP32. Esse é o plano de roteamento principal caso a VLAN bloqueie o tráfego de IP Multicast (Universal).

Verificar se a rede fornece IP automático (DHCP) ou se precisaremos fixar o endereço no código.
