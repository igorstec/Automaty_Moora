// Autor: Igor Stec
// Data: 13.04.2025
//
// Biblioteka symulująca automaty Moore'a — deterministyczne automaty skończone stosowane
// w synchronicznych układach cyfrowych. Implementacja zakłada dynamiczne tworzenie, łączenie,
// aktualizację i usuwanie automatów Moore’a oraz obsługę ich stanów i połączeń.

#include "ma.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <malloc.h>

#define ILE_UINT(x) (((x) + 63) / 64) // Oblicza liczbę 64-bitowych słów potrzebnych na x bitów


// Reprezentuje połączenie bitowe między dwoma automatami
typedef struct polaczenie {
    size_t bit_biore; // Który bit z automatu nadrzędnego jest pobierany
    moore_t *a_z_kad; // Automat, z którego pobierany jest bit
    moore_t *ja; // Automat, który pobiera bit
} polaczenie_t;


// Lista jednokierunkowa dla automatow
typedef struct Lista {
    moore_t *automat_moore; // Wskaźnik na potomka
    struct Lista *nxt; // Kolejny element listy
} list_ma;

// Reprezentacja automatu Moore'a
struct moore {
    size_t n, m, s; // n: liczba wejść, m: liczba wyjść, s: liczba bitów stanu
    transition_function_t t; // Funkcja przejścia
    output_function_t y; // Funkcja wyjściowa

    uint64_t *input; // Bufor wejść
    uint64_t *state; // Bufor stanu
    uint64_t *output; // Bufor wyjść
    uint64_t *next_state; // Bufor na przyszły stan

    list_ma *rodzice; // Lista rodziców
    list_ma *dzieci; // Lista dzieci
    polaczenie_t *podlaczenia_do_a; // Połączenia wejść
};

// Tworzy nowy, kompletny automat Moore’a
moore_t *ma_create_full(size_t n, size_t m, size_t s, transition_function_t t,
                        output_function_t y, uint64_t const *q) {
    if (!t || !y || !q || m == 0 || s == 0) {
        errno = EINVAL;
        return NULL;
    }
    moore_t *a = calloc(1, sizeof(moore_t));

    if (!a) {
        errno = ENOMEM;
        return NULL;
    }

    a->n = n;
    a->m = m;
    a->s = s;
    a->t = t;
    a->y = y;

    // Alokacja buforów
    a->input = calloc(ILE_UINT(n), sizeof(uint64_t));
    a->state = calloc(ILE_UINT(s), sizeof(uint64_t));
    //next_state nie jest alokowany gdyz alokujemy go i zarówno zwalniamy zawsze w ma_step
    //dlatego tez ostatecznie nie trzeba w delete go free'ować

    a->output = calloc(ILE_UINT(m), sizeof(uint64_t));
    a->podlaczenia_do_a = calloc(n, sizeof(polaczenie_t));
    if (!a->podlaczenia_do_a || !a->input || !a->state || !a->output) {
        free(a->podlaczenia_do_a);
        a->podlaczenia_do_a = NULL;
        free(a->input);
        a->input = NULL;
        free(a->state);
        a->state = NULL;
        free(a->output);
        a->output = NULL;
        free(a);
        a = NULL;
        errno = ENOMEM;
        return NULL;
    }
    // Tworzenie sztucznych (pustych) węzłów list
    list_ma *malist = calloc(1, sizeof(list_ma));
    list_ma *malist2 = calloc(1, sizeof(list_ma));

    if (!malist || !malist2) {

        free(malist);
        malist = NULL;
        free(malist2);
        malist2 = NULL;
        free(a->podlaczenia_do_a);
        a->podlaczenia_do_a = NULL;
        free(a->input);
        a->input = NULL;
        free(a->state);
        a->state = NULL;
        free(a->output);
        a->output = NULL;
        free(a);
        a = NULL;
        errno = ENOMEM;
        return NULL;
    }

    a->rodzice = malist; //do naprawy
    a->dzieci = malist2;

    memcpy(a->state, q, ILE_UINT(a->s) * sizeof(uint64_t));
    a->y(a->output, a->state, a->m, a->s);

    // Inicjalizacja połączeń wejść
    for (size_t i = 0; i < n; i++) {
        a->podlaczenia_do_a[i].a_z_kad = NULL;
        a->podlaczenia_do_a[i].ja = a;
        a->podlaczenia_do_a[i].bit_biore = 0;
    }

    return a;
}

// Ustawia stan automatu i aktualizuje wyjście
int ma_set_state(moore_t *a, uint64_t const *state) {
    if (!a || !state) {
        errno = EINVAL;
        return -1;
    }
    memcpy(a->state, state, ILE_UINT(a->s) * sizeof(uint64_t));
    a->y(a->output, a->state, a->m, a->s);
    return 0;
}



/** Zwalnia całą listę jednokierunkową `list_ma`. */
void free_list_ma(list_ma *head) {
    list_ma *current = head;
    while (current != NULL) {
        list_ma *next_node = current->nxt;
        free(current);
        current = next_node;
    }
}

/** Funkcja identyczności — kopiuje `state` na `output`. */
void identycznosc(uint64_t *output, uint64_t const *state,
                  size_t m, size_t s) {
    if (s == 0) { return; }
    size_t wielkosc = ILE_UINT(m);
    memcpy(output, state, wielkosc * sizeof(uint64_t));
}

// Tworzy prosty automat z y = state
moore_t *ma_create_simple(size_t n, size_t s, transition_function_t t) {
    if (s == 0 || !t) {
        errno = EINVAL;
        return NULL;
    }
    uint64_t *q = calloc(ILE_UINT(s), sizeof(uint64_t));
    if (!q) {
        errno = ENOMEM;
        return NULL;
    }
    moore_t *a = ma_create_full(n, s, s, t, identycznosc, q);
    free(q);
    q = NULL;//nule do usuniecia na koniec
    return a;
}


/** Ustawia wejścia dla automatu - podpiete nie maja znaczenia bo i tak w ma_step sie zaktualizują */
int ma_set_input(moore_t *a, uint64_t const *input) {
    if (a == NULL || input == NULL || a->n == 0) {//? a->n ==0?
        errno = EINVAL;
        return -1;
    }
    size_t slowa = ILE_UINT(a->n);
    memcpy(a->input, input, sizeof(uint64_t) * slowa);

    return 0;
}


/** Zwraca wskaźnik na dane wyjściowe automatu. */
uint64_t const *ma_get_output(moore_t const *a) {
    if (a == NULL) {
        errno = EINVAL;
        return NULL;
    }
    return a->output;
}

/** Zwalnia cały automat Moore'a i wszystkie zasoby. */
void ma_delete(moore_t *a) {
    if (!a) return;

    free(a->state);
    a->state = NULL;
    free(a->input);
    a->input = NULL;
    free(a->output);
    a->output = NULL;
    //next_state jest juz free

    free(a->podlaczenia_do_a);
    a->podlaczenia_do_a = NULL;


    //disconnectujemy automaty (dzieci ) ktore byly podlaczone do wyjscia a
    list_ma *dzieci = a->dzieci->nxt;

    while (dzieci) {
        moore_t *dziecko = dzieci->automat_moore;
        for (size_t i=0; i<dziecko->n; i++) {
            if (dziecko->podlaczenia_do_a[i].a_z_kad==a) {
                dziecko->podlaczenia_do_a[i].a_z_kad = NULL;
            }
        }
        //mosze dzeiciom powiedzeic ze nie jestem juz ich rodzicem
        list_ma *rodzice = dziecko->rodzice->nxt;
        list_ma *poprzedni = dziecko->rodzice;
        while (rodzice) {
            if (rodzice->automat_moore==a) {
                poprzedni->nxt = rodzice->nxt; //usuwamy z listy rodzicow
                free(rodzice);
                rodzice = NULL;
                break;
            }
            poprzedni = rodzice;
            rodzice = rodzice->nxt;
        }
        dzieci=dzieci->nxt;
    }

    //mose rodzicom powiedziec ze zostaje wydziedziczony
    list_ma *rodzice = a->rodzice->nxt;

    while (rodzice) {
        moore_t *rodzic = rodzice->automat_moore;
        dzieci = rodzic->dzieci->nxt;
        list_ma *poprzedni = rodzic->dzieci;
        while (dzieci) {
            if (dzieci->automat_moore==a) {
                poprzedni->nxt = rodzice->nxt; //usuwamy z listy rodzicow
                free(rodzice);
                rodzice = NULL;
                break;
            }
            poprzedni = dzieci;
            dzieci = dzieci->nxt;
        }
        rodzice = rodzice->nxt;
    }

    free_list_ma(a->rodzice);
    free_list_ma(a->dzieci);
    free(a);
    a = NULL;
}
void dodaj_do_listy(list_ma *head, moore_t *a) {
    if (a == NULL || head == NULL) {
        errno = EINVAL;
        return;
    }
    list_ma *new_node = calloc(1, sizeof(list_ma));
    if (!new_node) {
        errno = ENOMEM;
        return;
    }
    new_node->automat_moore = a;
    new_node->nxt = head->nxt;
    head->nxt = new_node;
}

/** Tworzy połączenia między automatami i aktualizuje struktury. */
int ma_connect(moore_t *a_in, size_t in, moore_t *a_out, size_t out, size_t num) {
    if (a_in == NULL || a_out == NULL || num == 0 || in + num > a_in->n || out + num > a_out->m) {
        errno = EINVAL;
        return -1;
    }
    list_ma *rodzice = a_in->rodzice;
    while(rodzice->nxt) {
        if(rodzice->nxt->automat_moore == a_out) {
            break;
        }
        rodzice = rodzice->nxt;
    }
    if(rodzice->nxt == NULL) {
        dodaj_do_listy(rodzice, a_out);
        if(errno == ENOMEM) {
            return -1; // Błąd alokacji pamięci
        }
    }

    list_ma *dzieci = a_out->dzieci;
    while(dzieci->nxt) {
        if(dzieci->nxt->automat_moore == a_in) {
            break;
        }
        dzieci = dzieci->nxt;
    }
    if(dzieci->nxt == NULL) {
        dodaj_do_listy(dzieci, a_in);
        if(errno == ENOMEM) {
            return -1; // Błąd alokacji pamięci
        }
    }
    size_t i = 0;
    while (i < num) {
        a_in->podlaczenia_do_a[in + i].bit_biore = out + i;
        a_in->podlaczenia_do_a[in + i].a_z_kad = a_out;
        a_in->podlaczenia_do_a[in + i].ja = a_in;
        i++;
    }

    return 0;
}


/** Usuwa połączenia wejść `a_in`. */
int ma_disconnect(moore_t *a_in, size_t in, size_t num) {
    if (!a_in || num == 0 || in + num > a_in->n) {
        errno = EINVAL;
        return -1;
    }
    size_t i = 0;
    while (i < num) {
        a_in->podlaczenia_do_a[in + i].a_z_kad = NULL;
        i++;
    }

    return 0;
}

/** Wykonuje jeden krok dla każdego automatu: input → state → output. */
int ma_step(moore_t *at[], size_t num) {
    //na poczatku mallock
    if (at == NULL || num == 0) {
        errno = EINVAL;
        return -1;
    }
    for (size_t i = 0; i < num; i++) {
        moore_t *a = at[i];
        if (a == NULL) {
            for(size_t j=0; j<i;j++) {
                free(at[j]->next_state);
            }
            errno = EINVAL;
            return -1;
        }
        size_t uint_state = ILE_UINT(a->s);
        uint64_t *next_state = calloc(uint_state, sizeof(uint64_t));
        if (!next_state) {
            for(size_t j=0; j<i;j++) {
                free(at[j]->next_state);
            }
            errno = ENOMEM;
            return -1;
        }
        a->next_state = next_state;
    }


    //aktaulizuje input
    for (size_t i = 0; i < num; i++) {
        moore_t *a = at[i];

        //aktualizacja input
        for (size_t j = 0; j < a->n; j++) {
            moore_t *b = a->podlaczenia_do_a[j].a_z_kad;
            if (b != NULL) {
                uint64_t zkad = b->output[ILE_UINT(a->podlaczenia_do_a[j].bit_biore+1) - 1];
                uint64_t x = 1ULL << (a->podlaczenia_do_a[j].bit_biore % 64);
                uint64_t y = x & zkad;
                size_t slowo = ILE_UINT(j+1) - 1;

                if (y > 0) {
                    a->input[slowo] |= y;
                } else {
                    a->input[slowo] &= (~x);
                }
            }
        }
        //input dla a aktualny


        size_t uint_state = ILE_UINT(a->s);
        memcpy(a->next_state, a->state, uint_state * sizeof(uint64_t));
        a->t(a->next_state, a->input,
             a->state, a->n, a->s);
    }
    //aktualizujemy output i ustawiamy stany
    for (size_t i = 0; i < num; i++) {
        moore_t *a = at[i];
        size_t uint_state = ILE_UINT(a->s);
        memcpy(a->state, a->next_state, uint_state * sizeof(uint64_t));
        a->y(a->output, a->state, a->m, a->s);

        free(a->next_state);
        a->next_state = NULL;
    }
    return 0;
}
