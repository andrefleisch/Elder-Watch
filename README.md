# Elder Watch

Um dispositivo que fica com a pessoa idosa e avisa a família pelo Telegram quando algo acontece. Ele percebe sozinho se a pessoa caiu, tem um botão de emergência para acionamento manual e ainda funciona como lembrete de remédio.

A ideia veio de um problema simples: quando um idoso cai sozinho em casa, o tempo até alguém perceber pode ser o que separa um susto de algo grave. Este projeto tenta encurtar esse tempo.

---

## O que ele faz

**Percebe quedas automaticamente.** O sensor de movimento acompanha a aceleração e a posição do corpo o tempo todo. Quando identifica o padrão característico de uma queda, dispara o alerta sem que ninguém precise fazer nada.

**Tem um botão de pânico.** Se a pessoa se sentir mal, tonta ou insegura, um toque no botão envia o aviso imediatamente.

**Escuta o ambiente.** Um sensor de som verifica se houve barulho alto no momento da queda, como um grito ou o impacto. Essa informação vai junto na mensagem e ajuda quem recebe a entender a gravidade da situação.

**Avisa pelo Telegram.** A mensagem chega no celular do cuidador com o tipo de ocorrência e o horário exato.

**Lembra dos remédios.** Dá para cadastrar até cinco alarmes com nome e horário. No horário marcado, um LED acende e o buzzer toca por alguns segundos.

**Mostra tudo numa página web.** Basta abrir o endereço do dispositivo no navegador para ver um gráfico do movimento em tempo real, o registro da última queda e a lista de alarmes.

---

## Como ele reconhece uma queda

O desafio aqui é distinguir uma queda de verdade de um movimento brusco qualquer. O dispositivo usa dois caminhos diferentes para isso.

O primeiro imita o que acontece fisicamente numa queda: por uma fração de segundo o corpo entra em queda livre e o sensor registra uma aceleração muito baixa; logo depois vem o impacto contra o chão, um pico brusco. É a **combinação dos dois na sequência certa** que confirma a queda. Se a queda livre acontece mas o impacto não vem logo em seguida, o alerta é cancelado, porque foi só um movimento rápido.

O segundo caminho é a inclinação. Se o dispositivo detecta que a pessoa está deitada ou muito inclinada e continua assim por vários segundos, ele entende que ela pode ter caído e não conseguiu se levantar.

Antes de qualquer decisão, os dados do sensor passam por um filtro que suaviza as leituras. Isso evita que uma vibração isolada seja confundida com um acidente.

---

## O que é preciso para montar

- Uma placa **ESP32** (é ela que faz tudo e já vem com Wi-Fi)
- Um sensor de movimento **MPU6050**
- Um sensor de som
- Um buzzer e dois LEDs
- Um botão

A pinagem de cada componente e os valores de sensibilidade estão comentados no início do código, junto com as bibliotecas necessárias.

---

## Colocando para funcionar

1. Monte o circuito seguindo os pinos indicados no código.
2. Preencha o nome e a senha da sua rede Wi-Fi.
3. Crie um bot no Telegram conversando com o **@BotFather** e cole o token e o seu ID de conversa no código.
4. Grave o programa na placa e abra o monitor serial para ver o endereço de IP que apareceu.
5. Digite esse endereço no navegador de qualquer celular ou computador da mesma rede.

Se o Wi-Fi não conectar em quinze segundos, a placa reinicia sozinha e tenta de novo.

> As senhas e o token ficam escritos direto no código. Antes de publicar o repositório, confira se estão apagados. E se o token verdadeiro já chegou a ser enviado alguma vez, gere um novo pelo BotFather.

---

## Ajustando a sensibilidade

Os limites que definem o que conta como queda estão logo no início do arquivo, todos agrupados e comentados. Diminuir o valor de impacto deixa o aparelho mais sensível, mas também aumenta a chance de alarme falso. Vale testar com o dispositivo na posição real em que será usado, porque a resposta muda bastante conforme onde ele fica preso no corpo.

---

## Limitações

Alguns pontos honestos sobre o estado atual:

- **Os alarmes somem se a placa reiniciar.** Eles ficam guardados só na memória temporária.
- **O aparelho fica alguns segundos "surdo" depois de um alerta.** Nesse intervalo o gráfico congela e uma segunda queda não seria detectada.
- **A localização não aparece na mensagem.** O código chega a consultar a posição aproximada, mas ela acaba não sendo incluída no texto enviado.
- **A posição seria imprecisa de qualquer forma.** A consulta é feita pelo endereço de internet, o que indica a região do provedor e não onde a pessoa está. Para uso de verdade, seria necessário um módulo de GPS.
- **A página web não tem senha.** Qualquer pessoa conectada à mesma rede consegue criar ou apagar alarmes.
