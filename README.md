# Inteli - Instituto de Tecnologia e Liderança

<p align="center">
<a href= "https://www.inteli.edu.br/"><img src="assets/inteli.png" alt="Inteli - Instituto de Tecnologia e Liderança" border="0" width=40% height=40%></a>
</p>

# Wall-I IOT

## Wall-I


## 👨‍🎓 Integrantes:

| <a href="https://www.linkedin.com/in/cecilslico/"><img src="./assets/membros/cecilia.jpg" alt="Cecília Coelho" width="120" height="120"></a> | <a href="https://www.linkedin.com/in/danielppdias/"><img src="./assets/membros/daniel.jpg" alt="Daniel Dias" width="120" height="120"></a> | <a href="https://www.linkedin.com/in/david-deodato-41b9b72b7/"><img src="./assets/membros/david.jpg" alt="David Deodato" width="120" height="120"></a> | <a href="https://www.linkedin.com/in/kauanmassuia/"><img src="./assets/membros/kauan.jpg" alt="Kauan Massuia" width="120" height="120"></a> |
| --------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Cecília Coelho                                                                                                                               | Daniel Dias                                                                                                                                  | David Deodato                                                                                                                                           | Kauan Massuia                                                                                                                                           |
| <a href="https://www.linkedin.com/in/milena-castro-vieira/"><img src="./assets/membros/milena.jpg" alt="Milena Castro" width="120" height="120"></a> | <a href="https://www.linkedin.com/in/otavio-vasc/"><img src="./assets/membros/otavio.jpg" alt="Otavio Vasconcelos" width="120" height="120"></a> | <a href="https://www.linkedin.com/in/thalyta-viana/"><img src="./assets/membros/thalyta.jpg" alt="Thalyta Viana" width="120" height="120"></a>        |
| Milena Castro                                                                                                                               | Otavio Vasconcelos                                                                                                                          | Thalyta Viana                                                                                                                                            |




## 👩‍🏫 Professores:

### Orientador(a)

- <a href="https://www.linkedin.com/in/marcelo-gon%C3%A7alves-phd-a550652/">🧭 Marcelo Gonçalves</a>

### Instrutores

- <a href="https://www.linkedin.com/in/egondaxbacher/">📈 Egon Daxbacher - Professor de Negócios</a>
- <a href="https://www.linkedin.com/in/andregodoichiovato/">👨‍💻 André Godoi - Professor de Programação</a>
- <a href="https://www.linkedin.com/in/francisco-escobar/">🎨 Francisco Escobar - Professor de Design</a>
- <a href="https://www.linkedin.com/in/geraldo-magela-severino-vasconcelos-22b1b220/">🧮 Geraldo Magela- Professor de Matemática</a>
- <a href="https://www.linkedin.com/in/michele-bazana-de-souza-69b77763/">👑 Michele Bazana - Professor de Liderança </a>

## 📜 Descrição

O projeto Wall-I IoT foi desenvolvido pelo grupo Wall-I para atender às necessidades do Instituto de Pesquisa e Tecnologia (IPT), oferecendo monitoramento em tempo real das máquinas utilizadas pela instituição, com foco na prensa e no compressor. Para cada máquina, foi projetado um módulo específico: o módulo da prensa inclui sensores ultrassônico e de temperatura, enquanto o do compressor conta com sensores de temperatura e um acelerômetro. Esses módulos permitem monitorar o estado e a temperatura das máquinas, identificando falhas antecipadamente e analisando padrões de comportamento para reduzir custos operacionais do IPT. Além disso, foi implementado um dashboard para visualizar os dados captados em tempo real, proporcionando maior controle e eficiência no gerenciamento das máquinas.

[Clicando aqui](https://www.youtube.com/watch?v=nsqAtR6hnvA) você encontrará um vídeo introdutório que explica o que você encontrará nos videos de utilização, e na descrição do vídeo você conseguirá acessar os videos individuais do protótipo do compressor e da prensa.

[Clicando aqui](https://youtu.be/ZJvtH9QrRI4?si=jiFKfljccng1jSak) você será redirecionado diretamente para o video de como utilizar o protótipo do compressor e o dashboard.

[Clicando aqui](https://youtu.be/WF2qAWOXGyU?si=FZbhlRlXUBnc9PYk) você será redirecionado diretamente para o video de como utilizar o protótipo da prensa e o dashboard.


## 📁 Estrutura de pastas

-  assets
-  documents
    - manuais
    - documentacao.md
- src 
  - firmware
    - olders_versions
- README.md
- LICENSE

Dentre os arquivos e pastas presentes na raiz do projeto, definem-se:

- assets: aqui estão os arquivos relacionados à parte gráfica do projeto, ou seja, as imagens e vídeos que os representam.

- documents: aqui está incluída toda a documentação do projeto.
    - manuais: onde estão presentes os manuais de instrução.

- src: Todo o código fonte criado para o desenvolvimento do projeto, incluindo o firmware.
    - firmware: inclui todos os códigos utilizados no projeto
      - olders_versions: inclui as versões antigas do código que são referenciadas na documentação.

- README.md: arquivo que serve como guia e explicação geral sobre o projeto (o mesmo que você está lendo agora).

- LICENSE: licença de utilização do projeto desenvolvido.

## 🔧 Instalação

Para iniciar a instalação do projeto é necessário realizar a instalação do arduino IDE para rodar o código, tal instalação e configuração poderá ser feita utilizando o link abaixo:

IDE para configuração do microcontrolador ESP32: [Arduino IDE](https://www.arduino.cc/en/software)

Após realizar a instalação, acesse o manual do modelo que você irá utilizar, caso vá manusear o da prensa entre no manual da prensa, ou se caso for utilizar o do compressor acesse o do compressor na pasta respectiva do github no link abaixo. Ao entrar no manual, você deverá seguir cada passo à passo com muita atenção sem perder nenhum detalhe, pois caso falte algum passo poderá ocasionar uma falha no modelo.

Acesse aqui os manuais para saber como iniciar e utilizar este projeto: [Manuais de instrução](https://github.com/Inteli-College/2024-2B-T12-IN04-G01/tree/main/documents/manuais).

## 🗃 Histórico de lançamentos

- 0.1.0 - 25/10/2023
  - Preenchimento da documentação com o entendimento do negócio, fundamentos da experiência do usuário com as personas, requisitos funcionais e prototipação inicial no Wokwi.
- 0.2.0 - 08/11/2023
  - Protótipo físico do projeto, levantamento de requisitos não funcionais e funcionais, e jornada do usuário com os storyboards.
- 0.3.0 - 22/11/2023
  - Protótipo do projeto com MQTT e I2C, metodologia e arquitetura da solução
- 0.4.0 - 06/12/2023
  - Protótipo físico do projeto com o online, refatoração do código, desenvolvimento do manual de instruções, arquitetura do protótipo e desenvolvimento das placas ilhadas e cases
- 1.0.0 - 19/12/2023
  - Lançamento da primeira versão final do protótipo.

## 📋 Licença/License

 <img style="height:22px!important;margin-left:3px;vertical-align:text-bottom;" src="https://mirrors.creativecommons.org/presskit/icons/cc.svg?ref=chooser-v1"> <img style="height:22px!important;margin-left:3px;vertical-align:text-bottom;" src="https://mirrors.creativecommons.org/presskit/icons/by.svg?ref=chooser-v1">

<p xmlns:cc="http://creativecommons.org/ns#" xmlns:dct="http://purl.org/dc/terms/">
  <a property="dct:title" rel="cc:attributionURL" href="https://github.com/Inteli-College/2024-2B-T12-IN04-G01">Wall-I</a> 
  by 
  <a rel="cc:attributionURL dct:creator" property="cc:attributionName" href="https://www.inteli.edu.br/">Inteli</a>, 
  <a href="https://www.linkedin.com/in/cecilslico/">Cecília Coelho</a>, 
  <a href="https://www.linkedin.com/in/danielppdias/">Daniel Dias</a>, 
  <a href="https://www.linkedin.com/in/david-deodato-41b9b72b7/">David Deodato</a>, 
  <a href="https://www.linkedin.com/in/kauanmassuia/">Kauan Massuia</a>, 
  <a href="https://www.linkedin.com/in/milena-castro-vieira/">Milena Castro</a>, 
  <a href="https://www.linkedin.com/in/otavio-vasc/">Otavio Vasconcelos</a>, 
  <a href="https://www.linkedin.com/in/thalyta-viana/">Thalyta Viana</a>. 
  is licensed under 
  <a href="http://creativecommons.org/licenses/by/4.0/?ref=chooser-v1" target="_blank" rel="license noopener noreferrer" style="display:inline-block;">
    Attribution 4.0 International
  
  </a>
</p>

