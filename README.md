<h1>
  <p align="center" width="100%">
    <img width="30%" src="https://softex.br/wp-content/uploads/2024/09/EmbarcaTech_logo_Azul-1030x428.png">
  </p>
</h1>

# ✨Tecnologias
Esse projeto foi desenvolvido com as seguintes tecnologias.
- Placa Raspberry Pi Pico W
- Raspberry Pi Pico SDK
- WokWi
- C/C++

# 💻Projeto
Projeto Desenvolvido durante a residência em microcontrolados e sistemas embarcados para estudantes de nível superior ofertado pela CEPEDI e SOFTEX, polo Juazeiro-BA, na Universidade Federal do Vale do São Francisco (UNIVASF), que tem como objetivo simular um semáforo utilizando a placa BitDogLab com Raspberry PI-Pico, e fortalecer o aprendizado sobre tarefas multicore na plataforma supracitada.

# 🚀Como rodar
### **Softwares Necessários**
1. **VS Code** com a extensão **Raspberry Pi Pico** instalada.
2. **CMake** e **Ninja** configurados.
3. **SDK do Raspberry Pi Pico** corretamente configurado.

### **Clonando o Repositório**
Para começar, clone o repositório no seu computador:
```bash
git https://github.com/DevMaic/Semaforo-Raspberry_Pi_picoW
cd Semaforo-Raspberry_Pi_picoW
```
---


### **Execução na Placa BitDogLab**
#### **1. Coloque em Modo Reboot**
1. Aperte o botão **BOOTSEL** no microcontrolador Raspberry Pi Pico W.
2. Ao mesmo tempo, aperte o botão de **Reset**.
#### **2. Upload de Arquivo `PiscaLed.uf2`**
1. Importe o projeto utilizando a extensão do VSCode, e o compile.
2. Abra a pasta `build` que será gerada na compilação.
3. Mova o arquivo `PiscaLed.uf2` para a placa de desenvolvimento.
#### **4. Acompanhar Execução do Programa**
1. Esse projeto não exige configuração alguma por parte do usuário, assim que mover o projeto para a placa observe a simulação do semáforo no display, na matriz de leds, nos leds RGB e também o auxílio sonoro para deficientes auditivos, tudo sincronizado por meio de tarefas.
   