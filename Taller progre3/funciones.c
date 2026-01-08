/*
  Implementación modular del SGIC (inventario + ventas + búsqueda "cercana" para cliente)
  Versión SIN persistencia en archivos (todo en memoria)
*/

#include "funciones.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

static int nextId = 1;
static int nextSaleId = 1;

/* ================== VALIDACIONES ================== */

int isOnlyLetters(const char *s) {
    if (!s || !*s) return 0;
    for (size_t i = 0; s[i]; i++) {
        if (!isalpha((unsigned char)s[i]) && s[i] != ' ')
            return 0;
    }
    return 1;
}

int isPositiveInt(int n) {
    return n > 0;
}

int isPositiveDouble(double n) {
    return n > 0.0;
}

/* Año válido: 1980 hasta año actual + 1 */
int isValidYear(int anio) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    int anioMax = tm->tm_year + 1900 + 1;
    return anio >= 1980 && anio <= anioMax;
}

/* ================== IDs ================== */

int getNextId(void) { return nextId++; }
void setNextId(int start) { if (start > nextId) nextId = start; }

int getNextSaleId(void) { return nextSaleId++; }
void setNextSaleId(int start) { if (start > nextSaleId) nextSaleId = start; }

/* ================== Helpers ================== */

static void ensureCapacity(Inventory *inv) {
    if (!inv->vehiculos) {
        inv->capacity = 10;
        inv->vehiculos = malloc(inv->capacity * sizeof(Vehicle));
    } else if (inv->size >= inv->capacity) {
        inv->capacity *= 2;
        inv->vehiculos = realloc(inv->vehiculos, inv->capacity * sizeof(Vehicle));
    }
    if (!inv->vehiculos) {
        fprintf(stderr, "Error de memoria\n");
        exit(EXIT_FAILURE);
    }
}

static void ensureSalesCapacity(Inventory *inv) {
    if (!inv->sales) {
        inv->sales_capacity = 8;
        inv->sales = malloc(inv->sales_capacity * sizeof(Sale));
    } else if (inv->sales_size >= inv->sales_capacity) {
        inv->sales_capacity *= 2;
        inv->sales = realloc(inv->sales, inv->sales_capacity * sizeof(Sale));
    }
    if (!inv->sales) {
        fprintf(stderr, "Error de memoria (ventas)\n");
        exit(EXIT_FAILURE);
    }
}

static int matchesStr(const char *a, const char *b) {
    if (!a || !*a) return 1;
    if (!b) return 0;

    char la[STR_LEN], lb[STR_LEN];
    size_t i;

    for (i = 0; a[i] && i < STR_LEN - 1; i++)
        la[i] = tolower((unsigned char)a[i]);
    la[i] = '\0';

    for (i = 0; b[i] && i < STR_LEN - 1; i++)
        lb[i] = tolower((unsigned char)b[i]);
    lb[i] = '\0';

    return strstr(lb, la) != NULL;
}

/* ================== Init / Free ================== */

void initInventory(Inventory *inv) {
    inv->vehiculos = NULL;
    inv->size = inv->capacity = 0;
    inv->sales = NULL;
    inv->sales_size = inv->sales_capacity = 0;
}

void freeInventory(Inventory *inv) {
    free(inv->vehiculos);
    free(inv->sales);
    initInventory(inv);
}

/* ================== CRUD Vehículos ================== */

int addVehicle(Inventory *inv, const Vehicle *v) {

    if (!v) return 0;
    if (!isOnlyLetters(v->marca)) return 0;
    if (!isOnlyLetters(v->modelo)) return 0;
    if (!isValidYear(v->anio)) return 0;
    if (!isPositiveDouble(v->precio)) return 0;
    if (v->kilometraje < 0) return 0;

    ensureCapacity(inv);

    Vehicle copy = *v;
    if (copy.id <= 0) copy.id = getNextId();

    inv->vehiculos[inv->size++] = copy;

    if (copy.id >= nextId) setNextId(copy.id + 1);

    return copy.id;
}

Vehicle *findVehicleById(Inventory *inv, int id) {
    if (!isPositiveInt(id)) return NULL;

    for (size_t i = 0; i < inv->size; i++)
        if (inv->vehiculos[i].id == id)
            return &inv->vehiculos[i];
    return NULL;
}

int removeVehicleById(Inventory *inv, int id) {
    if (!isPositiveInt(id)) return 0;

    for (size_t i = 0; i < inv->size; i++) {
        if (inv->vehiculos[i].id == id) {
            memmove(&inv->vehiculos[i], &inv->vehiculos[i + 1],
                    (inv->size - i - 1) * sizeof(Vehicle));
            inv->size--;
            return 1;
        }
    }
    return 0;
}

/* ================== Búsqueda simple ================== */

Vehicle *findVehiclesByRequirement(Inventory *inv, const Requirement *req, size_t *out_count) {

    if (!req || req->maxPrecio < 0) return NULL;
    if (req->minAnio && !isValidYear(req->minAnio)) return NULL;
    if (req->maxAnio && !isValidYear(req->maxAnio)) return NULL;
    if (req->minAnio && req->maxAnio && req->minAnio > req->maxAnio) return NULL;

    Vehicle *res = NULL;
    size_t count = 0, cap = 0;

    for (size_t i = 0; i < inv->size; i++) {
        Vehicle *v = &inv->vehiculos[i];
        if (!v->disponible) continue;
        if (!matchesStr(req->marca, v->marca)) continue;
        if (!matchesStr(req->modelo, v->modelo)) continue;
        if (req->minAnio && v->anio < req->minAnio) continue;
        if (req->maxAnio && v->anio > req->maxAnio) continue;
        if (req->maxPrecio && v->precio > req->maxPrecio) continue;

        if (count >= cap) {
            cap = cap ? cap * 2 : 8;
            res = realloc(res, cap * sizeof(Vehicle));
        }
        res[count++] = *v;
    }

    *out_count = count;
    return res;
}

/* ================== Búsqueda para cliente ================== */

typedef struct {
    Vehicle v;
    double score;
} VS;

static int cmpVS(const void *a, const void *b) {
    const VS *A = a;
    const VS *B = b;
    return (A->score > B->score) - (A->score < B->score);
}

Vehicle *findVehiclesForClient(Inventory *inv, const Client *c, size_t *out_count) {

    if (!c || !isPositiveDouble(c->presupuesto)) return NULL;
    if (c->marcaPreferida[0] && !isOnlyLetters(c->marcaPreferida)) return NULL;

    VS *arr = NULL;
    size_t cnt = 0, cap = 0;

    time_t t = time(NULL);
    int curYear = localtime(&t)->tm_year + 1900;

    for (size_t i = 0; i < inv->size; i++) {
        Vehicle *v = &inv->vehiculos[i];
        if (!v->disponible || v->precio > c->presupuesto) continue;
        if (!isValidYear(v->anio)) continue;

        double score = fabs(v->precio - c->presupuesto) / c->presupuesto;
        score += 0.2 * ((double)(curYear - v->anio) / 50.0);
        score += 0.2 * ((double)v->kilometraje / 300000.0);

        if (c->marcaPreferida[0] && matchesStr(c->marcaPreferida, v->marca))
            score *= 0.7;

        if (cnt >= cap) {
            cap = cap ? cap * 2 : 8;
            arr = realloc(arr, cap * sizeof(VS));
        }

        arr[cnt++] = (VS){*v, score};
    }

    qsort(arr, cnt, sizeof(VS), cmpVS);

    Vehicle *out = malloc(cnt * sizeof(Vehicle));
    for (size_t i = 0; i < cnt; i++)
        out[i] = arr[i].v;

    free(arr);
    *out_count = cnt;
    return out;
}

/* ================== Ventas ================== */

int registerSale(Inventory *inv, int vehicleId,
                 const char *buyer, const char *seller, double price) {

    if (!isPositiveInt(vehicleId)) return 0;
    if (!buyer || !isOnlyLetters(buyer)) return 0;
    if (!seller || !isOnlyLetters(seller)) return 0;
    if (!isPositiveDouble(price)) return 0;

    Vehicle *v = findVehicleById(inv, vehicleId);
    if (!v || !v->disponible) return 0;

    ensureSalesCapacity(inv);

    Sale s = {0};
    s.sale_id = getNextSaleId();
    s.vehicle_id = vehicleId;
    strncpy(s.buyer, buyer, STR_LEN - 1);
    strncpy(s.seller, seller, STR_LEN - 1);
    s.price = price;

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    snprintf(s.date, DATE_LEN, "%04d-%02d-%02d",
             tm->tm_year + 1900,
             tm->tm_mon + 1,
             tm->tm_mday);

    inv->sales[inv->sales_size++] = s;
    v->disponible = 0;

    return s.sale_id;
}

/* ================== Impresión ================== */

void printVehicle(const Vehicle *v) {
    if (!v) return;

    printf("ID: %d\n", v->id);
    printf("Marca: %s\n", v->marca);
    printf("Modelo: %s\n", v->modelo);
    printf("Año: %d\n", v->anio);
    printf("Precio: %.2f\n", v->precio);
    printf("Kilometraje: %d\n", v->kilometraje);
    printf("Condición: %s\n", v->esNuevo ? "Nuevo" : "Usado");
    printf("Disponible: %s\n", v->disponible ? "Sí" : "No");
}

void listVehicles(const Inventory *inv) {
    if (!inv || inv->size == 0) {
        printf("El inventario está vacío.\n");
        return;
    }

    for (size_t i = 0; i < inv->size; i++) {
        printVehicle(&inv->vehiculos[i]);
        printf("----------------------------------\n");
    }
}

void listSales(const Inventory *inv) {
    if (!inv || inv->sales_size == 0) {
        printf("No hay ventas registradas.\n");
        return;
    }

    for (size_t i = 0; i < inv->sales_size; i++) {
        Sale *s = &inv->sales[i];
        printf("Venta %d | Vehículo %d | Comprador: %s | Vendedor: %s | %.2f | %s\n",
               s->sale_id,
               s->vehicle_id,
               s->buyer,
               s->seller,
               s->price,
               s->date);
    }
}

/* ================== Estado ================== */

int markSold(Inventory *inv, int id) {
    if (!isPositiveInt(id)) return 0;

    Vehicle *v = findVehicleById(inv, id);
    if (!v) return 0;

    v->disponible = 0;
    return 1;
}

int markAvailable(Inventory *inv, int id) {
    if (!isPositiveInt(id)) return 0;

    Vehicle *v = findVehicleById(inv, id);
    if (!v) return 0;

    v->disponible = 1;
    return 1;
}
