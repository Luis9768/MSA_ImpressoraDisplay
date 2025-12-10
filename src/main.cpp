#include <Arduino.h>
#include "Config.h"
#include "DisplayManager.h"
#include "NetworkSlave.h"
#include "PrinterManager.h"

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
  int acao = verificarToque();

  if (estadoAtual == 0) { // ESTADO: CAROUSEL
    
    // Se selecionou algum produto (ID > 0)
    if (acao > 0) {
      Serial.printf("Entrando no produto ID %d\n", acao);
      idProdutoAtual = acao;
      contadorProducao = 0; // Reseta contador
      
      // Busca os dados do produto
      std::vector<Receita> lista = getListaReceitas();
      for (const auto& r : lista) {
        if (r.id == idProdutoAtual) {
          receitaAtiva = r; // Salva para uso na impressão
          mostrarTelaProducao(r);
          estadoAtual = 1; // Muda para produção
          break;
        }
      }
    }
    
    // Se houve atualização na lista vinda do Master
    if (novaListaDisponivel()) {
      Serial.println(">>> UPDATE: Lista atualizada pelo Master! <<<");
      std::vector<Receita> novaLista = getListaReceitas();
      
      Serial.printf("DEBUG: EstadoAtual=%d, IDProdutoAtual=%d, TamanhoLista=%d\n", estadoAtual, idProdutoAtual, novaLista.size());

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
          for (const auto& r : novaLista) {
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
         Serial.println("DEBUG: Não estava em produção ou ID invalido. Nada a fazer além de atualizar carousel.");
      }

      // Atualiza UI se voltou para carousel ou se estava nele
      if (estadoAtual == 0) {
          Serial.println("DEBUG: Atualizando Carousel UI.");
          mostrarCarouselSlave(novaLista, 0); 
      }
      
      confirmarAtualizacaoLista();
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
        Serial.printf("Trocado para ID %d: %s\n", proxima.id, proxima.descricao);
      }
    }
    
    // Lógica do Scanner/Contador (SIMULAÇÃO VIA SERIAL)
    if (Serial.available()) {
      char c = Serial.read();
      // Ignora quebra de linha
      if (c != '\n' && c != '\r') {
        contadorProducao++;
        atualizarContador(contadorProducao);
        
        Serial.printf(">>> SCANNER SIMULADO: Contagem %d <<<\n", contadorProducao);
        
        // Imprime Etiqueta
        imprimirEtiqueta(receitaAtiva, contadorProducao);
      }
    }
  }
  
  // Pequeno delay para não fritar a CPU (opcional, mas bom para LVGL)
  delay(5); 
}