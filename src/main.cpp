#include <Arduino.h>
#include "Config.h"
#include "DisplayManager.h"
#include "NetworkSlave.h"

// Estado da Aplicação
// 0 = Lista de Produtos
// 1 = Tela de Produção
int estadoAtual = 0;
int idProdutoAtual = 0;
int contadorProducao = 0;

void setup() {
  Serial.begin(115200);
  
  // 1. Inicializa Display (LVGL)
  setupDisplay();
  
  // 2. Inicializa Rede (ESP-NOW)
  setupNetworkSlave();
  
  Serial.println(">>> SLAVE INICIADO <<<");
}

void loop() {
  // Mantém a UI responsiva
  loopDisplay();
  loopNetworkSlave();

  // Lógica de Navegação
  int acao = verificarToque();

  if (estadoAtual == 0) { // ESTADO: LISTA
    
    // Se tocou em algum produto (ID > 0)
    if (acao > 0) {
      Serial.printf("Entrando no produto ID %d\n", acao);
      idProdutoAtual = acao;
      contadorProducao = 0; // Reseta contador
      
      // Busca os dados do produto
      std::vector<Receita> lista = getListaReceitas();
      for (const auto& r : lista) {
        if (r.id == idProdutoAtual) {
          mostrarTelaProducao(r);
          estadoAtual = 1; // Muda para produção
          break;
        }
      }
    }
    
    // Se houve atualização na lista vinda do Master
    if (novaListaDisponivel()) {
      Serial.println("Lista atualizada pelo Master!");
      mostrarListaSlave(getListaReceitas());
      confirmarAtualizacaoLista();
    }

  } else if (estadoAtual == 1) { // ESTADO: PRODUÇÃO
    
    // Se pediu para voltar (ID -1)
    if (acao == -1) {
      Serial.println("Voltando para a lista...");
      estadoAtual = 0;
      idProdutoAtual = 0;
      mostrarListaSlave(getListaReceitas());
    }
    
    // AQUI: Lógica do Scanner/Contador
    if (Serial.available()) {
      char c = Serial.read();
      // Ignora quebra de linha
      if (c != '\n' && c != '\r') {
        contadorProducao++;
        atualizarContador(contadorProducao);
        Serial.printf("Contagem: %d\n", contadorProducao);
      }
    }
  }
  
  // Pequeno delay para não fritar a CPU (opcional, mas bom para LVGL)
  delay(5); 
}