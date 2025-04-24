
// Ao compilar, executar ./ex02 N_THREADS N_LINHAS N_COLUNAS

#define _POSIX_C_SOURCE 199309L

#include<stdio.h>
#include<pthread.h>
#include<ctype.h>
#include<stdlib.h>  
#include<time.h>
#include<math.h>

typedef struct { // struct de matriz
    int linhas;
    int colunas;
    int** dados;
} Matriz;


struct chunk_data { // chunk de linhas
    int inicio;
    int fim;
    int linha;
    int colunas;
    int **dados;
    double *res;
};

struct col_chunk_data { // chunk de colunas
    int inicio;
    int fim;
    int coluna;
    int linhas;
    int **dados;
    double *res;
};

void gera_matriz(const char *nome_arquivo, int M, int N){

    FILE *f = fopen(nome_arquivo, "w"); // Cria o arquivo

    fprintf(f, "%dx%d\n", M, N); // Escreve o tamanho da matriz na primeira linha do arquivo,
                                 // semelhantemente ao exemplo.
    for(int i = 0; i < M; i++){
        for(int j = 0; j < N; j++){

            int valor = rand() % 100 + 1; // Valor entre 0 e 100
            fprintf(f, "%d ", valor);
        }
        fprintf(f, "\n");
    }

    fclose(f);

}

int **alocar_matriz(int linhas, int colunas) {
    int **mat = malloc(linhas * sizeof(int *));
    for (int i = 0; i < linhas; i++) {
        mat[i] = malloc(colunas * sizeof(int));
    }
    return mat;
}

Matriz le_matriz(const char *nome_arquivo){

    FILE *f = fopen(nome_arquivo, "r"); // abre o arquivo em modo leitura
    if (!f){
        perror("Erro ao abrir o arquivo!");
        exit(1);
    }

    Matriz matriz; // "instancia" uma matriz
    fscanf(f, "%dx%d", &matriz.linhas, &matriz.colunas); // lê dimensões da primeira linha do arquivo em formato "MxN"
    matriz.dados = alocar_matriz(matriz.linhas, matriz.colunas); // aloca o tamanho da matriz de acordo com o que foi lido
    for (int i = 0; i < matriz.linhas; i++){
        for (int j = 0; j < matriz.colunas; j++){
            fscanf(f, "%d", &matriz.dados[i][j]); // le os dados do arquivo e coloca em sua respectiva posição na matriz
        }
    }

    fclose(f); // fecha o arquivo
    return matriz; // retorna a struct da matriz
    
}
void *thread_media_aritmetica(void* param){ // faz o calculo da media aritmetica dos valores da linha da respecitva thread

    struct chunk_data *data = (struct chunk_data *) param; // recebe os valores passados por parametro (dentro da struct data_chunk)

    for (int i = data->inicio; i < data->fim; i++){

        int soma = 0; // inicializa a variavel LOCAL de soma para a média
        for(int j = 0; j < data->colunas; j++){
            soma += data->dados[i][j]; // faz a soma de todos os valores da linha
        }
        data->res[i] = (double) soma / data->colunas; // divide pela quantidade de valores e adiciona no vetor de respostas exatamente na posição relativa à linha
        
    }

    pthread_exit(NULL); // termina a thread

}

void *thread_media_geometrica(void* param){

    struct col_chunk_data *data = (struct col_chunk_data *) param; // recebe os valores passados por parametro (dentro da struct col_data_chunk)

    for(int j = data->inicio; j < data->fim; j++){
        double produto = 1.0; // variavel local que guardará o valor do produtório dentro da thread
        
        for(int i = 0; i < data->linhas; i++){
            produto *= data->dados[i][j]; // faz o produto de todos os valores da coluna
            
        }
        
        data->res[j] = pow(produto, 1.0/data->linhas); // armazena o valor da raiz do produtório na respectiva posição relativa à coluna no vetor de resultados 
    }

    pthread_exit(NULL); // termina a thread
}

int main(int argc, char *argv[]) { // 

    struct timespec start, end; // inicializa as variáveis para contagem de tempo
    clock_gettime(CLOCK_MONOTONIC, &start); // inicializa a contagem de tempo

    if (argc < 2){
        printf("Escreva no formato ./ex02 NUM_THREADS NUM_LINHAS NUM_COLUNAS\n", argv[0]);
    }

    int num_threads = atoi(argv[1]); // obtem a quantidade de threads via terminal

    int M = atoi(argv[2]); // obtem a quantidade de linhas (M) e colunas (N) via terminal
    int N = atoi(argv[3]);

    printf("Quantidade de threads usada: %d", num_threads);

    gera_matriz("matriz.txt", M, N); // gera a matriz aleatoria

    Matriz m = le_matriz("matriz.txt"); // le a matriz gerada e armazena na struct

    double resLinhas[m.linhas]; // cria o vetor de respostas
    double resColunas[m.colunas];

    pthread_t tLinhas[num_threads]; // cria o vetor para armazenar as threads,
    pthread_t tColunas[num_threads];

    struct chunk_data dataLinhas[num_threads]; // cria o vetor para armazenar os data_chunks que serão passados por parametro para cada thread
    struct col_chunk_data dataColunas[num_threads];

    int linhas_por_thread = m.linhas / num_threads; // calcula a quantidade de linhas por thread
    int resto_linhas = m.linhas % num_threads; // calcula se há resto de linhas (quando a quantidade de linhas não é divisivel pela quantidade de threads)

    int colunas_por_thread = m.colunas / num_threads; // calcula a quantidade de colunas por thread
    int resto_colunas = m.colunas % num_threads; // calcula se há resto de colunas

    int inicio = 0; // primeira thread inicia no começo da matriz
    for(int i = 0; i < num_threads; i++){

        int fim = inicio + linhas_por_thread + (i < resto_linhas ? 1 : 0); // calcula até onde essa thread irá iterar
        dataLinhas[i] = (struct chunk_data){.inicio = inicio, .fim = fim, .colunas = m.colunas, .dados = m.dados, .res = resLinhas}; // configura os dados da thread
        pthread_create(&tLinhas[i], NULL, thread_media_aritmetica, (void *)&dataLinhas[i]); // cria a thread com as configurações
        inicio = fim; // configura o proximo inicio da proxima thread
    }

    inicio = 0; // zera o inicio para que possa calcular as threads de colunas
    for(int i = 0; i < num_threads; i++){
        int fim = inicio + colunas_por_thread + (i < resto_colunas ? 1 : 0); // calcula até onde essa thread irá iterar
        dataColunas[i] = (struct col_chunk_data){.inicio = inicio, .fim = fim, .linhas = m.linhas, .dados = m.dados, .res = resColunas}; // configura os dados da thread
        pthread_create(&tColunas[i], NULL, thread_media_geometrica, (void *)&dataColunas[i]); // cria a thread com as configurações
        inicio = fim;
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(tLinhas[i], NULL); // faz os joins entre as threads para que a main só continue executando quando todas as threads terminarem
        pthread_join(tColunas[i], NULL);
    }


    FILE *f = fopen("resultado.txt", "w");
    fprintf(f, "\nMédias Aritméticas das Linhas:\n");
    for (int i = 0; i < m.linhas; i++) {
        fprintf(f, "Linha %d: %.2f\n", i, resLinhas[i]);
    }


    fprintf(f, "\nMédias Geométricas das Colunas:\n");
    for (int j = 0; j < m.colunas; j++) {
        fprintf(f, "Coluna %d: %.2f\n", j, resColunas[j]);
    }
    fclose(f);

    // printf("Matriz %dx%d:\n", m.linhas, m.colunas); // exibe matriz lida
    // for (int i = 0; i < m.linhas; i++) {
    //     for (int j = 0; j < m.colunas; j++) {
    //         printf("%d ", m.dados[i][j]);
    //     }
    //     printf("\n");
    // }
    
    
    for (int i = 0; i < m.linhas; i++) { // liberar memória
        free(m.dados[i]);
    }
    free(m.dados); // libera a matriz

    clock_gettime(CLOCK_MONOTONIC, &end); // termina a contagem de tempo

    double tempo_em_segundos = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9; // calcula o tempo em segundos

    printf("\n");
    printf("Execução com %d threads.\n", num_threads);
    printf("Tempo de execução de %.5f segundos", tempo_em_segundos);

    return 0;
}




