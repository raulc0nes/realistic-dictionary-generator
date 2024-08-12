#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#define NUM_THREADS 3
#define YEAR_START 1850
#define YEAR_END 2350
#define COMMON_NUM_COUNT 150
#define LEEET_VARIANTS 20
#define MAX_WORDS 5
#define MAX_COMBINATION_LENGTH 256
#define HASH_TABLE_SIZE 1000003

typedef struct Node {
    char combination[MAX_COMBINATION_LENGTH];
    struct Node *next;
} Node;

typedef struct {
    char words[MAX_WORDS][128];
    int word_count;
    FILE *file;
    long total_combinations;
    long processed_combinations;
    Node *hash_table[HASH_TABLE_SIZE];
} ThreadData;

uint32_t hash_function(const char *str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash % HASH_TABLE_SIZE;
}

bool is_combination_unique(ThreadData *data, const char *combination) {
    char normalized[MAX_COMBINATION_LENGTH];
    for (int i = 0; i < strlen(combination); i++) {
        normalized[i] = tolower(combination[i]);
    }
    normalized[strlen(combination)] = '\0';

    uint32_t hash = hash_function(normalized);
    Node *current = data->hash_table[hash];
    while (current) {
        if (strcmp(current->combination, normalized) == 0) {
            return false;
        }
        current = current->next;
    }

    Node *new_node = malloc(sizeof(Node));
    strcpy(new_node->combination, normalized);
    new_node->next = data->hash_table[hash];
    data->hash_table[hash] = new_node;

    return true;
}

void generateUniqueCombinations(ThreadData *data, const char *str, int pos, char *result) {
    if (pos == strlen(str)) {
        if (is_combination_unique(data, result)) {
            fprintf(data->file, "%s\n", result);
            data->processed_combinations++;
        }
        return;
    }

    // Lowercase
    result[pos] = tolower(str[pos]);
    generateUniqueCombinations(data, str, pos + 1, result);

    // Uppercase
    result[pos] = toupper(str[pos]);
    generateUniqueCombinations(data, str, pos + 1, result);
}

void leetSpeakVariants(ThreadData *data, char *str) {
    char buffer[MAX_COMBINATION_LENGTH];
    char leetVariants[LEEET_VARIANTS][2] = {
        {'a', '4'}, {'e', '3'}, {'i', '1'}, {'o', '0'}, {'s', '5'}, {'t', '7'},
        {'a', '@'}, {'i', '!'}, {'g', '9'}, {'b', '8'}, {'z', '2'}, {'h', '#'},
        {'l', '1'}, {'o', '*'}, {'s', '$'}, {'e', '&'}, {'t', '+'}, {'m', '^'},
        {'n', '%'}, {'u', '_'}
    };

    for (int i = 0; i < LEEET_VARIANTS; i++) {
        strcpy(buffer, str);
        for (int j = 0; buffer[j]; j++) {
            if (tolower(buffer[j]) == leetVariants[i][0]) {
                buffer[j] = leetVariants[i][1];
            }
        }
        if (is_combination_unique(data, buffer)) {
            fprintf(data->file, "%s\n", buffer);
            data->processed_combinations++;
        }
    }
}

long estimateCombinations(int word_count, const char words[MAX_WORDS][128]) {
    long case_combinations = 1;
    for (int i = 0; i < word_count; i++) {
        case_combinations *= (1 << strlen(words[i]));
    }
    long leet_combinations = case_combinations * LEEET_VARIANTS;
    long date_combinations = leet_combinations * (YEAR_END - YEAR_START + 1) * 4 * word_count;
    long number_combinations = leet_combinations * COMMON_NUM_COUNT * 3 * word_count;
    return leet_combinations + date_combinations + number_combinations;
}

void countdown_timer(int seconds) {
    while (seconds > 0) {
        printf("\rTiempo restante: %d segundos   ", seconds);
        fflush(stdout);
        sleep(1);
        seconds--;
    }
    printf("\rGeneración completa.                 \n");
}

// Función para mezclar y generar combinaciones de palabras
void *generateWordCombinations(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    char buffer[MAX_COMBINATION_LENGTH];
    char combined[MAX_COMBINATION_LENGTH];

    for (int i = 0; i < data->word_count; i++) {
        fprintf(data->file, "%s\n", data->words[i]);  // Guardar la palabra original tal cual

        for (int j = i + 1; j < data->word_count; j++) {
            snprintf(combined, MAX_COMBINATION_LENGTH, "%s%s", data->words[i], data->words[j]);
            generateUniqueCombinations(data, combined, 0, buffer);
            leetSpeakVariants(data, combined);

            snprintf(combined, MAX_COMBINATION_LENGTH, "%s%s", data->words[j], data->words[i]);
            generateUniqueCombinations(data, combined, 0, buffer);
            leetSpeakVariants(data, combined);
        }
    }

    return NULL;
}

// Función para generar combinaciones con fechas importantes y mezclas
void *generateDateCombinations(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    char buffer[MAX_COMBINATION_LENGTH];
    char temp[MAX_COMBINATION_LENGTH];

    for (int year = YEAR_START; year <= YEAR_END; year++) {
        int firstTwoDigits = year / 100;
        int lastTwoDigits = year % 100;

        for (int i = 0; i < data->word_count; i++) {
            snprintf(temp, MAX_COMBINATION_LENGTH, "%d%s%d", firstTwoDigits, data->words[i], lastTwoDigits);
            generateUniqueCombinations(data, temp, 0, buffer);
            leetSpeakVariants(data, temp);

            int mid = strlen(data->words[i]) / 2;
            snprintf(temp, MAX_COMBINATION_LENGTH, "%.*s%d%s", mid, data->words[i], year, data->words[i] + mid);
            generateUniqueCombinations(data, temp, 0, buffer);
            leetSpeakVariants(data, temp);
        }
    }

    return NULL;
}

// Función para generar combinaciones con números comunes y mezclas
void *generateNumericCombinations(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    char buffer[MAX_COMBINATION_LENGTH];
    char temp[MAX_COMBINATION_LENGTH];

    char *commonNumbers[COMMON_NUM_COUNT] = {
        "1", "2", "3", "4", "5",
        "6", "7", "8", "9", "0",
        "00", "111", "0987654321", "111111111", "1234567890",
        "666", "777", "888", "999", "1122334455",
        "123123", "654321", "314159", "271828", "987654321",
        "2468", "13579", "101010", "202020", "333333",
        "999999", "4444", "1234", "9876", "0000",
        "5555", "1212", "6969", "0001", "7890",
        "147258", "852", "741", "852963", "951357",
        "246824", "369963", "111000", "3210", "24682468",
        "135246", "987", "123456", "654", "789",
        "2580", "142536", "369", "147", "258",
        "369", "159753", "321", "123", "987",
        "147852", "951", "852", "963", "147147",
        "123321", "321123", "987789", "789987", "258258",
        "369369", "12345", "54321", "00000", "11111",
        "22222", "33333", "44444", "55555", "66666",
        "77777", "88888", "99999", "101010", "010101",
        "202020", "212121", "222222", "234567", "765432",
        "345678", "876543", "456789", "987654", "567890",
        "098765", "678901", "098765", "67890", "09876",
        "543210", "6789", "9870", "3210", "6543",
        "7654", "8765", "9876", "2345", "1234",
        "1357", "2468", "3690", "1470", "2580",
        "3690", "8520", "7410", "9630", "741",
        "852963", "1234567", "7654321", "2345678", "8765432",
        "3456789", "9876543", "4567890", "0987654", "5678901",
        "09876543", "6789012", "098765432", "98765432", "7654321",
        "234567", "765432", "456789", "678901", "890123",
        "012345", "543210", "23456789", "987654321", "76543210"
    };

    for (int i = 0; i < COMMON_NUM_COUNT; i++) {
        for (int j = 0; j < data->word_count; j++) {
            snprintf(temp, MAX_COMBINATION_LENGTH, "%s%s", commonNumbers[i], data->words[j]);
            generateUniqueCombinations(data, temp, 0, buffer);
            leetSpeakVariants(data, temp);

            snprintf(temp, MAX_COMBINATION_LENGTH, "%s%s", data->words[j], commonNumbers[i]);
            generateUniqueCombinations(data, temp, 0, buffer);
            leetSpeakVariants(data, temp);

            int mid = strlen(data->words[j]) / 2;
            snprintf(temp, MAX_COMBINATION_LENGTH, "%.*s%s%s", mid, data->words[j], commonNumbers[i], data->words[j] + mid);
            generateUniqueCombinations(data, temp, 0, buffer);
            leetSpeakVariants(data, temp);
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 6) {
        printf("Uso: %s <palabra1> [palabra2 ... palabra5]\n", argv[0]);
        return 1;
    }

    ThreadData data;
    data.word_count = argc - 1;
    memset(data.hash_table, 0, sizeof(data.hash_table));
    
    for (int i = 0; i < data.word_count; i++) {
        strncpy(data.words[i], argv[i + 1], sizeof(data.words[i]));
    }

    // Estimar número total de combinaciones
    long total_combinations = estimateCombinations(data.word_count, data.words);
    long estimated_size = total_combinations * 30; // Estimación aproximada de 30 bytes por línea
    printf("Combinaciones estimadas: %ld\n", total_combinations);
    printf("Tamaño estimado del archivo: %ld bytes\n", estimated_size);

    if (estimated_size > 1L << 30) { // 1 GB
        printf("El archivo podría ser demasiado grande. Considere reducir el tamaño de las combinaciones.\n");
        return 1;
    }

    FILE *file = fopen("combinaciones.txt", "w");
    if (file == NULL) {
        perror("Error al abrir el archivo");
        return 1;
    }

    data.file = file;
    data.total_combinations = total_combinations;
    data.processed_combinations = 0;

    pthread_t threads[NUM_THREADS];

    // Crear hilos para cada parte del procesamiento
    pthread_create(&threads[0], NULL, generateWordCombinations, &data);
    pthread_create(&threads[1], NULL, generateDateCombinations, &data);
    pthread_create(&threads[2], NULL, generateNumericCombinations, &data);

    // Temporizador de cuenta regresiva
    int estimated_seconds = (int)(total_combinations / 50000); // Ajustar según la velocidad de procesamiento
    countdown_timer(estimated_seconds);

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    fclose(file);
    printf("Combinaciones generadas en 'combinaciones.txt'\n");

    return 0;
}

