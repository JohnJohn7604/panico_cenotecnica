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
