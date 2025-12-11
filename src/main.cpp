#include "Config.h"
#include "DisplayManager.h"
#include "NetworkSlave.h"
#include "PrinterManager.h"
#include <Arduino.h>

// Estado da Aplicação
// 0 = Lista de Produtos (Carousel)
// 1 = Tela de Produção
int estadoAtual = 0;
int idProdutoAtual = 0;
int contadorProducao = 0;
Receita receitaAtiva; // Armazena a receita atual para impressão

void setup() {
  Serial.begin(115200);

  // 1. Inicializa Display (LVGL)
  setupDisplay();

  // 2. Inicializa Rede (ESP-NOW)
  setupNetworkSlave();

  // 3. Inicializa Impressora
  setupPrinter();

  Serial.println(">>> SLAVE INICIADO (CAROUSEL + PRINTER) <<<");
}

void loop() {
  // Mantém a UI responsiva
  loopDisplay();
  loopNetworkSlave();

  // Lógica de Navegação
  // Lógica de Navegação
  int acao = verificarToque();

  // GLOBAL: Debug e Tratamento de Serial (Scanner)
  if (Serial.available()) {
    char c = Serial.read();
    // Ignora white-spaces basicos se quiser, ou processa tudo
    if (c != '\n' && c != '\r') {

      if (estadoAtual == 1) { // Só conta se estiver na tela de produção
        if (contadorProducao >= receitaAtiva.quantidade) {
          Serial.println(">>> CAIXA CHEIA! LIMITE ATINGIDO! <<<");
          mostrarAlertaCaixaCheia(); // Chama o popup visual
        } else {
          contadorProducao++;
          atualizarContador(contadorProducao);
          Serial.printf(">>> SCANNER: Item Validado! Contagem: %d / %d <<<\n",
                        contadorProducao, receitaAtiva.quantidade);
          imprimirEtiqueta(receitaAtiva, contadorProducao);
          delay(50);
          while (Serial.available())
            Serial.read(); // Limpa buffer
        }
      } else {
        Serial.println(">>> AVISO: Recebido dado Serial fora da tela de "
                       "produção. Ignorado. <<<");
      }
    }
  }

  // --- 1. GLOBAL: Verifica atualizações de lista (Prioridade Máxima) ---
  if (novaListaDisponivel()) {
    Serial.println(">>> UPDATE: Lista atualizada pelo Master! <<<");
    std::vector<Receita> novaLista = getListaReceitas();

    Serial.printf("DEBUG: EstadoAtual=%d, IDProdutoAtual=%d, TamanhoLista=%d\n",
                  estadoAtual, idProdutoAtual, novaLista.size());

    bool produtoAtualExiste = false;

    // Se lista vazia, força saída
    if (novaLista.empty()) {
      Serial.println("DEBUG: Lista VAZIA! Forçando saída.");
      idProdutoAtual = 0;
      estadoAtual = 0;
    }
    // Se estamos em produção, verifica se o produto ainda existe
    else if (estadoAtual == 1 && idProdutoAtual > 0) {
      Serial.println("DEBUG: Verificando se produto atual ainda existe...");
      for (const auto &r : novaLista) {
        if (r.id == idProdutoAtual) {
          produtoAtualExiste = true;
          receitaAtiva = r;
          Serial.println("DEBUG: Produto ENCONTRADO na nova lista.");
          break;
        }
      }

      if (!produtoAtualExiste) {
        Serial.println("DEBUG: Produto NAO ENCONTRADO! Removido! Voltando...");
        idProdutoAtual = 0;
        estadoAtual = 0; // Força volta para carousel
      }
    } else {
      // Se o ID mudou ou algo assim, garantimos que não estamos em ID invalido
      if (estadoAtual == 1 && idProdutoAtual == 0)
        estadoAtual = 0;
    }

    // Atualiza UI se estiver no Carousel (ou foi forçado a voltar)
    if (estadoAtual == 0) {
      Serial.println("DEBUG: Atualizando Carousel UI.");
      mostrarCarouselSlave(novaLista, 0);
    }

    confirmarAtualizacaoLista();
  }
  // --------------------------------------------------------------------

  if (estadoAtual == 0) { // ESTADO: CAROUSEL

    // Se selecionou algum produto (ID > 0)
    if (acao > 0) {
      Serial.printf("Entrando no produto ID %d\n", acao);
      idProdutoAtual = acao;
      contadorProducao = 0; // Reseta contador

      // Busca os dados do produto
      std::vector<Receita> lista = getListaReceitas();
      for (const auto &r : lista) {
        if (r.id == idProdutoAtual) {
          receitaAtiva = r; // Salva para uso na impressão
          mostrarTelaProducao(r);
          estadoAtual = 1; // Muda para produção
          break;
        }
      }
    }
  } else if (estadoAtual == 1) { // ESTADO: PRODUÇÃO

    // Se pediu para voltar (ID -1)
    // Se pediu para voltar (ID -1)
    if (acao == -1) {
      Serial.println("Voltando para o carousel...");
      estadoAtual = 0;
      idProdutoAtual = 0;
      mostrarCarouselSlave(getListaReceitas(), 0);
    }
    // Se pediu PRÓXIMO PRODUTO (ID -2)
    else if (acao == -2) {
      Serial.println("Navegando para proximo produto...");

      std::vector<Receita> lista = getListaReceitas();
      int indexAtual = -1;

      // Encontra indice atual
      for (int i = 0; i < lista.size(); i++) {
        if (lista[i].id == idProdutoAtual) {
          indexAtual = i;
          break;
        }
      }

      // Calcula proximo indice (Circular)
      if (indexAtual != -1 && !lista.empty()) {
        int proximoIndex = (indexAtual + 1) % lista.size();
        Receita proxima = lista[proximoIndex];

        // Atualiza estado
        idProdutoAtual = proxima.id;
        receitaAtiva = proxima;
        contadorProducao = 0; // Reseta na troca

        mostrarTelaProducao(proxima);
        Serial.printf("Trocado para ID %d: %s\n", proxima.id,
                      proxima.descricao);
      }
    }

    // Lógica do Scanner/Contador (SIMULAÇÃO VIA SERIAL)
    // Lógica do Scanner (REAL)
    // Serial handling moved to global scope
  }

  // Pequeno delay para não fritar a CPU (opcional, mas bom para LVGL)
  delay(5);
}