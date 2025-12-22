/*
  Implementación modular del SGIC (inventario + ventas + búsqueda "cercana" para cliente)
  - Inventario en memoria con realloc
  - Persistencia CSV simple (sin escapes)
  - Búsqueda para cliente que filtra por presupuesto y preferencias y ordena por "cercanía"
*/

#include "funciones.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include <math.h>

static int nextId = 1;
static int nextSaleId = 1;

int getNextId(void) { return nextId++; }
void setNextId(int start) { if (start > nextId) nextId = start; }
int getNextSaleId(void) { return nextSaleId++; }
void setNextSaleId(int start) { if (start > nextSaleId) nextSaleId = start; }

/* Helpers internos */
static void ensureCapacity(Inventory *inv) {
    if (inv->vehiculos == NULL) {
        inv->capacity = 10;
        inv->vehiculos = malloc(inv->capacity * sizeof(Vehicle));
        if (!inv->vehiculos) { fprintf(stderr, "Memoria insuficiente\n"); exit(EXIT_FAILURE); }
    } else if (inv->size >= inv->capacity) {
        inv->capacity *= 2;
        Vehicle *tmp = realloc(inv->vehiculos, inv->capacity * sizeof(Vehicle));
        if (!tmp) { fprintf(stderr, "realloc falló\n"); exit(EXIT_FAILURE); }
        inv->vehiculos = tmp;
    }
}

static void ensureSalesCapacity(Inventory *inv) {
    if (inv->sales == NULL) {
        inv->sales_capacity = 8;
        inv->sales = malloc(inv->sales_capacity * sizeof(Sale));
        if (!inv->sales) { fprintf(stderr, "Memoria insuficiente (sales)\n"); exit(EXIT_FAILURE); }
    } else if (inv->sales_size >= inv->sales_capacity) {
        inv->sales_capacity *= 2;
        Sale *tmp = realloc(inv->sales, inv->sales_capacity * sizeof(Sale));
        if (!tmp) { fprintf(stderr, "realloc ventas falló\n"); exit(EXIT_FAILURE); }
        inv->sales = tmp;
    }
}

/* Comparación case-insensitive substring: si 'a' vacío => true */
static int matchesStr(const char *a, const char *b) {
    if (!a || a[0] == '\0') return 1;
    if (!b) return 0;
    char la[STR_LEN], lb[STR_LEN];
    size_t i;
    for (i = 0; i < STR_LEN-1 && a[i]; ++i) la[i] = tolower((unsigned char)a[i]);
    la[i] = '\0';
    for (i = 0; i < STR_LEN-1 && b[i]; ++i) lb[i] = tolower((unsigned char)b[i]);
    lb[i] = '\0';
    return strstr(lb, la) != NULL;
}

void initInventory(Inventory *inv) {
    inv->vehiculos = NULL; inv->size = inv->capacity = 0;
    inv->sales = NULL; inv->sales_size = inv->sales_capacity = 0;
}

void freeInventory(Inventory *inv) {
    free(inv->vehiculos); inv->vehiculos = NULL; inv->size = inv->capacity = 0;
    free(inv->sales); inv->sales = NULL; inv->sales_size = inv->sales_capacity = 0;
}

/* Operaciones básicas */
int addVehicle(Inventory *inv, const Vehicle *v) {
    ensureCapacity(inv);
    Vehicle copy = *v;
    if (copy.id <= 0) copy.id = getNextId();
    inv->vehiculos[inv->size++] = copy;
    if (copy.id >= nextId) setNextId(copy.id + 1);
    return copy.id;
}

int removeVehicleById(Inventory *inv, int id) {
    for (size_t i = 0; i < inv->size; ++i) {
        if (inv->vehiculos[i].id == id) {
            for (size_t j = i+1; j < inv->size; ++j) inv->vehiculos[j-1] = inv->vehiculos[j];
            inv->size--;
            return 1;
        }
    }
    return 0;
}

Vehicle *findVehicleById(Inventory *inv, int id) {
    for (size_t i = 0; i < inv->size; ++i) if (inv->vehiculos[i].id == id) return &inv->vehiculos[i];
    return NULL;
}

int updateVehicle(Inventory *inv, int id, const Vehicle *v) {
    Vehicle *pv = findVehicleById(inv, id);
    if (!pv) return 0;
    Vehicle copy = *v; copy.id = id;
    *pv = copy;
    return 1;
}

/* Búsqueda simple por Requirement (sin orden) */
Vehicle *findVehiclesByRequirement(Inventory *inv, const Requirement *req, size_t *out_count) {
    Vehicle *results = NULL;
    size_t count = 0, cap = 0;
    for (size_t i = 0; i < inv->size; ++i) {
        Vehicle *v = &inv->vehiculos[i];
        if (!v->disponible) continue;
        if (!matchesStr(req->marca, v->marca)) continue;
        if (!matchesStr(req->modelo, v->modelo)) continue;
        if (req->minAnio != 0 && v->anio < req->minAnio) continue;
        if (req->maxAnio != 0 && v->anio > req->maxAnio) continue;
        if (req->maxPrecio != 0 && v->precio > req->maxPrecio) continue;
        if (req->maxKilometraje != 0 && v->kilometraje > req->maxKilometraje) continue;
        if (req->requiereNuevo != -1 && v->esNuevo != req->requiereNuevo) continue;
        if (count >= cap) { cap = cap==0?8:cap*2; results = realloc(results, cap * sizeof(Vehicle)); if (!results) { fprintf(stderr,"Memoria\n"); exit(EXIT_FAILURE);} }
        results[count++] = *v;
    }
    *out_count = count;
    return results;
}

/* Búsqueda para cliente: filtra por presupuesto y preferencias y ordena por "cercanía".
   Se consideran únicamente vehículos disponibles con precio <= presupuesto.
   Se retorna arreglo malloc con copias ordenadas por score ascendente. */
Vehicle *findVehiclesForClient(Inventory *inv, const Client *c, size_t *out_count) {
    if (!c || !inv) { *out_count = 0; return NULL; }

    /* temporal: array de (Vehicle, score) */
    typedef struct { Vehicle v; double score; } VS;
    VS *arr = NULL;
    size_t cnt = 0, cap = 0;

    /* determine some normalization baselines */
    double budget = c->presupuesto > 0.0 ? c->presupuesto : 1.0;
    int maxYearDiff = 50; /* para normalizar diferencia de año */
    int maxKm = 300000; /* para normalizar kilometraje */

    for (size_t i = 0; i < inv->size; ++i) {
        Vehicle *v = &inv->vehiculos[i];
        if (!v->disponible) continue;
        if (v->precio > c->presupuesto) continue; /* fuera de presupuesto */
        if (c->minAnio != 0 && v->anio < c->minAnio) continue;
        if (c->requiresNew != 0) { /* placeholder to avoid warnings */ ; }
        /* Preferencias de nuevo/usado */
        if (c->requiereNuevo != -1 && v->esNuevo != c->requiereNuevo) continue;
        if (c->maxKilometraje != 0 && v->kilometraje > c->maxKilometraje) continue;
        /* Modelo/marca preferida are soft preferences; they don't filter unless explicitly wanted? We'll keep them as preferences */
        /* Compute score (lower = better) */
        double score = 0.0;
        /* price difference relative to budget (the closer to budget, the better) */
        double priceDiff = fabs(v->precio - budget) / budget; /* 0..inf, usually 0..1 */
        if (priceDiff > 1.0) priceDiff = 1.0;
        score += 0.5 * priceDiff; /* weight 0.5 */

        /* year: if client specified minAnio, prefer newer than or near it; else prefer newer vehicles */
        double yearPref = 0.0;
        if (c->minAnio != 0) {
            int diff = c->minAnio - v->anio;
            if (diff > 0) yearPref = (double)diff / maxYearDiff; /* penalize if older than minAnio */
        } else {
            /* prefer newer vehicles: relative to 2025 (or current year) */
            int curYear = 1900 + localtime(&(time_t){time(NULL)})->tm_year;
            int age = curYear - v->anio;
            if (age < 0) age = 0;
            yearPref = (double)age / maxYearDiff;
        }
        if (yearPref > 1.0) yearPref = 1.0;
        score += 0.2 * yearPref; /* weight 0.2 */

        /* kilometraje preference: lower is better */
        double kmPref = 0.0;
        if (c->maxKilometraje != 0) kmPref = (double)v->kilometraje / (double)c->maxKilometraje;
        else kmPref = (double)v->kilometraje / (double)maxKm;
        if (kmPref > 1.0) kmPref = 1.0;
        score += 0.2 * kmPref; /* weight 0.2 */

        /* brand/model soft preference: give bonus if match, penalty if not */
        if (c->marcaPreferida[0] != '\0') {
            if (matchesStr(c->marcaPreferida, v->marca)) score *= 0.6; /* better */
            else score += 0.2; /* small penalty */
        }
        if (c->modeloPreferido[0] != '\0') {
            if (matchesStr(c->modeloPreferido, v->modelo)) score *= 0.8;
            else score += 0.1;
        }

        /* accumulate into arr */
        if (cnt >= cap) { cap = cap==0?8:cap*2; arr = realloc(arr, cap * sizeof(VS)); if (!arr) { fprintf(stderr,"Memoria\n"); exit(EXIT_FAILURE); } }
        arr[cnt].v = *v;
        arr[cnt].score = score;
        cnt++;
    }

    if (cnt == 0) { *out_count = 0; free(arr); return NULL; }

    /* sort arr by score asc */
    int cmpVS(const void *a, const void *b) {
        const VS *A = a; const VS *B = b;
        if (A->score < B->score) return -1;
        if (A->score > B->score) return 1;
        return 0;
    }
    qsort(arr, cnt, sizeof(VS), cmpVS);

    /* prepare output array of Vehicles (possibly limited by maxResults) */
    size_t outN = cnt;
    if (c->maxResultados > 0 && (size_t)c->maxResultados < outN) outN = (size_t)c->maxResultados;
    Vehicle *out = malloc(outN * sizeof(Vehicle));
    if (!out) { fprintf(stderr,"Memoria\n"); exit(EXIT_FAILURE); }
    for (size_t i = 0; i < outN; ++i) out[i] = arr[i].v;

    free(arr);
    *out_count = outN;
    return out;
}

/* Listado */
void listVehicles(const Inventory *inv) {
    if (!inv || inv->size == 0) { printf("El inventario está vacío.\n"); return; }
    for (size_t i = 0; i < inv->size; ++i) {
        printVehicle(&inv->vehiculos[i]);
        printf("-------------------------------------------------\n");
    }
}

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

/* Persistencia inventario CSV: id,marca,modelo,anio,precio,kilometraje,esNuevo,disponible */
int saveInventoryToFile(const Inventory *inv, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) return 0;
    fprintf(f, "id,marca,modelo,anio,precio,kilometraje,esNuevo,disponible\n");
    for (size_t i = 0; i < inv->size; ++i) {
        const Vehicle *v = &inv->vehiculos[i];
        fprintf(f, "%d,%s,%s,%d,%.2f,%d,%d,%d\n",
                v->id, v->marca, v->modelo, v->anio, v->precio, v->kilometraje, v->esNuevo, v->disponible);
    }
    fclose(f); return 1;
}

int loadInventoryFromFile(Inventory *inv, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return 0;
    freeInventory(inv);
    initInventory(inv);
    char line[512];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return 1; }
    if (strstr(line, "id,") == NULL) fseek(f, 0, SEEK_SET);
    int max_id = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strlen(line) < 3) continue;
        Vehicle v; memset(&v,0,sizeof(v));
        int scanned = sscanf(line, "%d,%63[^,],%63[^,],%d,%lf,%d,%d,%d",
                             &v.id, v.marca, v.modelo, &v.anio, &v.precio, &v.kilometraje, &v.esNuevo, &v.disponible);
        if (scanned >= 8) { addVehicle(inv,&v); if (v.id>max_id) max_id=v.id; }
    }
    fclose(f);
    setNextId(max_id+1);
    return 1;
}

/* Ventas: registrar y listar. Registro sencillo en memoria y marcado como vendido. */
int registerSale(Inventory *inv, int vehicleId, const char *buyer, const char *seller, double price) {
    Vehicle *v = findVehicleById(inv, vehicleId);
    if (!v) return 0;
    if (!v->disponible) return 0;
    ensureSalesCapacity(inv);
    Sale s; memset(&s,0,sizeof(s));
    s.sale_id = getNextSaleId();
    s.vehicle_id = vehicleId;
    strncpy(s.buyer, buyer?buyer:"", STR_LEN-1);
    strncpy(s.seller, seller?seller:"", STR_LEN-1);
    s.price = price;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(s.date, DATE_LEN, "%04d-%02d-%02d", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
    inv->sales[inv->sales_size++] = s;
    v->disponible = 0;
    if (s.sale_id >= nextSaleId) setNextSaleId(s.sale_id + 1);
    return s.sale_id;
}

void listSales(const Inventory *inv) {
    if (!inv || inv->sales_size == 0) { printf("No hay ventas registradas.\n"); return; }
    for (size_t i = 0; i < inv->sales_size; ++i) {
        const Sale *s = &inv->sales[i];
        printf("Venta ID: %d | Vehículo ID: %d | Comprador: %s | Vendedor: %s | Precio: %.2f | Fecha: %s\n",
               s->sale_id, s->vehicle_id, s->buyer, s->seller, s->price, s->date);
    }
}

/* Guardar/cargar ventas CSV: sale_id,vehicle_id,buyer,seller,price,date */
int saveSalesToFile(const Inventory *inv, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) return 0;
    fprintf(f, "sale_id,vehicle_id,buyer,seller,price,date\n");
    for (size_t i = 0; i < inv->sales_size; ++i) {
        const Sale *s = &inv->sales[i];
        fprintf(f, "%d,%d,%s,%s,%.2f,%s\n", s->sale_id, s->vehicle_id, s->buyer, s->seller, s->price, s->date);
    }
    fclose(f); return 1;
}

int loadSalesFromFile(Inventory *inv, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return 0;
    char line[512];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return 1; }
    if (strstr(line, "sale_id,") == NULL) fseek(f,0,SEEK_SET);
    int max_sale_id = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strlen(line) < 3) continue;
        Sale s; memset(&s,0,sizeof(s));
        int scanned = sscanf(line, "%d,%d,%63[^,],%63[^,],%lf,%19s",
                             &s.sale_id, &s.vehicle_id, s.buyer, s.seller, &s.price, s.date);
        if (scanned >= 6) {
            ensureSalesCapacity(inv);
            inv->sales[inv->sales_size++] = s;
            if (s.sale_id > max_sale_id) max_sale_id = s.sale_id;
            Vehicle *v = findVehicleById(inv, s.vehicle_id);
            if (v) v->disponible = 0;
        }
    }
    fclose(f);
    setNextSaleId(max_sale_id+1);
    return 1;
}

/* Marcar vendido/disponible */
int markSold(Inventory *inv, int id) {
    Vehicle *v = findVehicleById(inv,id);
    if (!v) return 0;
    v->disponible = 0; return 1;
}
int markAvailable(Inventory *inv, int id) {
    Vehicle *v = findVehicleById(inv,id);
    if (!v) return 0;
    v->disponible = 1; return 1;
}