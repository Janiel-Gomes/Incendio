# FireGuard IoT - Sistema Inteligente de Detecção de Incêndio

Este projeto consiste em um sistema de monitoramento de incêndio ponta-a-ponta, utilizando Internet das Coisas (IoT) e Inteligência Artificial para detecção precoce de riscos.

## 🚀 Arquitetura do Sistema

O sistema é composto por três camadas principais:

1.  **Hardware (Embedded)**: Utiliza uma **Raspberry Pi Pico W** para coletar dados de sensores de temperatura, umidade e presença de chama.
2.  **Backend (Cloud/Edge)**: Um servidor **FastAPI** que recebe os dados via Wi-Fi, armazena o histórico e executa um modelo de IA para predição de risco.
3.  **Frontend (Dashboard)**: Uma interface web moderna e responsiva para monitoramento em tempo real dos sensores e do status de alerta.

## 🛠️ Componentes Utilizados

- **Raspberry Pi Pico W** (Microcontrolador com Wi-Fi)
- **Sensor AHT10** (Temperatura e Umidade)
- **Sensor de Chama** (Infravermelho)
- **Buzzer Ativo** (Alarme Sonoro)
- **LED Indicador** (Feedback Visual)

## 🧠 Inteligência Artificial

O sistema utiliza um modelo **Random Forest Classifier** treinado com dados históricos de incêndios florestais e residenciais. 
- **Entradas**: Temperatura e Umidade.
- **Saída**: Probabilidade de incêndio (0-100%).
- **Override de Segurança**: Se o sensor de chama físico for ativado, o sistema entra em estado de alerta imediato, independentemente da temperatura/umidade.

## 📂 Estrutura do Projeto

```text
incendio/
├── cloud/              # Backend FastAPI e Dashboard HTML
│   ├── ai/             # Scripts de treinamento e modelo salvo
│   ├── templates/      # Dashboard (interface do usuário)
│   └── app.py          # Servidor principal (Produção)
├── firmware/           # Bibliotecas do SDK da Raspberry Pi Pico
├── incendio.c          # Código fonte principal do microcontrolador (FreeRTOS)
├── aht10.c/h           # Driver para o sensor de temperatura/umidade
└── README.md           # Documentação do projeto
```

## ⚙️ Instalação e Execução

### Backend
1. Navegue até a pasta `cloud/`:
   ```bash
   pip install fastapi uvicorn joblib pandas scikit-learn
   python app.py
   ```
2. O dashboard estará disponível em `http://localhost:5000`.

### Firmware
1. Configure o seu SSID e Senha do Wi-Fi no arquivo `incendio.c`.
2. Configure o `SERVER_IP` com o endereço do computador rodando o backend.
3. Compile o projeto usando a extensão da Raspberry Pi Pico no VS Code e carregue para a placa.

## 👨‍💻 Desenvolvedor
Projeto desenvolvido para simulação de sistemas de segurança industrial e residencial inteligentes.
