#include "funciones.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

/* ================== IDs ================== */
static int nextId = 1;
static int nextSaleId = 1;

int getNextId(void) { return nextId++; }
void setNextId(int start) { if (start > nextId) nextId = start; }

int getNextSaleId(void) { return nextSaleId++; }
void setNextSaleId(int start) { if (start > nextSaleId) nextSaleId = start; }

/* ================== Validaciones ================== */
static int isOnlyLetters(const char *s) {
    if (!s || !*s) return 0;
    for (size_t i = 0; s[i]; i++) {
        if (!isalpha((unsigned char)s[i]) && s[i] != ' ')
            return 0;
    }
    return 1;
}

static int isPositiveInt(int n) { return n > 0; }
static int isPositiveDouble(double n) { return n > 0.0; }

static int isValidYear(int anio) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    int max = tm->tm_year + 1900 + 1;
    return anio >= 1980 && anio <= max;
}

static int isOnlyDigits(const char *s) {
    if (!s) return 0;
    for (size_t i = 0; s[i]; i++) {
        if (!isdigit((unsigned char)s[i])) return 0;
    }
    return 1;
}

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

static void ensureClientsCapacity(Inventory *inv) {
    if (!inv->clientes) {
        inv->clientes_capacity = 10;
        inv->clientes = malloc(inv->clientes_capacity * sizeof(Client));
    } else if (inv->clientes_size >= inv->clientes_capacity) {
        inv->clientes_capacity *= 2;
        inv->clientes = realloc(inv->clientes, inv->clientes_capacity * sizeof(Client));
    }

    if (!inv->clientes) {
        fprintf(stderr, "Error de memoria (clientes)\n");
        exit(EXIT_FAILURE);
    }
}

static int matchesStr(const char *a, const char *b) {
    if (!a || !*a) return 1;
    if (!b) return 0;

    char la[STR_LEN], lb[STR_LEN];
    size_t i;
    for (i = 0; a[i] && i < STR_LEN - 1; i++) la[i] = tolower((unsigned char)a[i]);
    la[i] = '\0';
    for (i = 0; b[i] && i < STR_LEN - 1; i++) lb[i] = tolower((unsigned char)b[i]);
    lb[i] = '\0';

    return strstr(lb, la) != NULL;
}

/* ================== Init / Free ================== */
void initInventory(Inventory *inv) {
    inv->vehiculos = NULL;
    inv->size = inv->capacity = 0;

    inv->sales = NULL;
    inv->sales_size = inv->sales_capacity = 0;

    inv->clientes = NULL;
    inv->clientes_size = inv->clientes_capacity = 0;
}

void freeInventory(Inventory *inv) {
    free(inv->vehiculos);
    free(inv->sales);
    free(inv->clientes);
    initInventory(inv);
}

/* ================== CRUD Vehículos ================== */
int addVehicle(Inventory *inv, const Vehicle *v) {
    if (!inv || !v) return 0;
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
    if (!inv || !isPositiveInt(id)) return NULL;
    for (size_t i = 0; i < inv->size; i++)
        if (inv->vehiculos[i].id == id)
            return &inv->vehiculos[i];
    return NULL;
}

int removeVehicleById(Inventory *inv, int id) {
    if (!inv || !isPositiveInt(id)) return 0;
    for (size_t i = 0; i < inv->size; i++) {
        if (inv->vehiculos[i].id == id) {
            memmove(&inv->vehiculos[i], &inv->vehiculos[i+1], (inv->size-i-1)*sizeof(Vehicle));
            inv->size--;
            return 1;
        }
    }
    return 0;
}

int updateVehicle(Inventory *inv, int id, const Vehicle *v) {
    Vehicle *orig = findVehicleById(inv, id);
    if (!orig || !v) return 0;
    *orig = *v;
    orig->id = id;
    return 1;
}

/* ================== Clientes ================== */
int addClient(Inventory *inv, const Client *c) {
    if (!inv || !c) return 0;
    if (!isOnlyLetters(c->nombre)) return 0;
    if (!isOnlyLetters(c->apellido)) return 0;
    if (c->telefono[0] && !isOnlyDigits(c->telefono)) return 0;
    if (!isPositiveDouble(c->presupuesto)) return 0;

    ensureClientsCapacity(inv);

    Client copy = *c;
    copy.id = getNextId();
    inv->clientes[inv->clientes_size++] = copy;
    if (copy.id >= nextId) setNextId(copy.id + 1);

    return copy.id;
}

Client *findClientById(Inventory *inv, int id) {
    if (!inv || !isPositiveInt(id)) return NULL;
    for (size_t i = 0; i < inv->clientes_size; i++)
        if (inv->clientes[i].id == id)
            return &inv->clientes[i];
    return NULL;
}

int removeClientById(Inventory *inv, int id) {
    if (!inv || !isPositiveInt(id)) return 0;
    for (size_t i = 0; i < inv->clientes_size; i++) {
        if (inv->clientes[i].id == id) {
            memmove(&inv->clientes[i], &inv->clientes[i+1], (inv->clientes_size-i-1)*sizeof(Client));
            inv->clientes_size--;
            return 1;
        }
    }
    return 0;
}

int updateClient(Inventory *inv, int id, const Client *c) {
    Client *orig = findClientById(inv, id);
    if (!orig || !c) return 0;
    *orig = *c;
    orig->id = id;
    return 1;
}

/* ================== Ventas ================== */
int registerSale(Inventory *inv, int vehicleId, const char *buyer, const char *seller, double price) {
    if (!inv || !isPositiveInt(vehicleId)) return 0;
    if (!buyer || !isOnlyLetters(buyer)) return 0;
    if (!seller || !isOnlyLetters(seller)) return 0;
    if (!isPositiveDouble(price)) return 0;

    Vehicle *v = findVehicleById(inv, vehicleId);
    if (!v || !v->disponible) return 0;

    ensureSalesCapacity(inv);

    Sale s = {0};
    s.sale_id = getNextSaleId();
    s.vehicle_id = vehicleId;
    strncpy(s.buyer, buyer, STR_LEN-1);
    strncpy(s.seller, seller, STR_LEN-1);
    s.price = price;

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    snprintf(s.date, DATE_LEN, "%04d-%02d-%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);

    inv->sales[inv->sales_size++] = s;
    v->disponible = 0;
    return s.sale_id;
}

void listSales(const Inventory *inv) {
    if (!inv || inv->sales_size == 0) {
        printf("No hay ventas registradas.\n");
        return;
    }
    for (size_t i = 0; i < inv->sales_size; i++) {
        const Sale *s = &inv->sales[i];
        printf("Venta %d | Vehículo %d | %s -> %s | %.2f | %s\n",
               s->sale_id, s->vehicle_id, s->buyer, s->seller, s->price, s->date);
    }
}

/* ================== Listado ================== */
void printVehicle(const Vehicle *v) {
    if (!v) return;
    printf("ID: %d\nMarca: %s\nModelo: %s\nAño: %d\nPrecio: %.2f\nKilometraje: %d\nCondición: %s\nDisponible: %s\n",
           v->id, v->marca, v->modelo, v->anio, v->precio, v->kilometraje,
           v->esNuevo ? "Nuevo" : "Usado",
           v->disponible ? "Sí" : "No");
}

void listVehicles(const Inventory *inv) {
    if (!inv || inv->size == 0) {
        printf("El inventario está vacío.\n");
        return;
    }
    for (size_t i = 0; i < inv->size; i++) {
        printVehicle(&inv->vehiculos[i]);
        printf("-----------------------------\n");
    }
}

void listClients(const Inventory *inv) {
    if (!inv || inv->clientes_size == 0) {
        printf("No hay clientes registrados.\n");
        return;
    }
    for (size_t i = 0; i < inv->clientes_size; i++) {
        const Client *c = &inv->clientes[i];
        printf("ID: %d\nNombre: %s %s\nTeléfono: %s\nPresupuesto: %.2f\nMarca Preferida: %s\n-------------------------\n",
               c->id, c->nombre, c->apellido, c->telefono[0] ? c->telefono : "N/A",
               c->presupuesto, c->marcaPreferida[0] ? c->marcaPreferida : "N/A");
    }
}

/* ================== Marcar vehículo ================== */
int markSold(Inventory *inv, int id) {
    Vehicle *v = findVehicleById(inv, id);
    if (!v) return 0;
    v->disponible = 0;
    return 1;
}

int markAvailable(Inventory *inv, int id) {
    Vehicle *v = findVehicleById(inv, id);
    if (!v) return 0;
    v->disponible = 1;
    return 1;
}

/* ================== Persistencia mínima ================== */
int saveInventoryToFile(const Inventory *inv, const char *filename) { return 1; }
int loadInventoryFromFile(Inventory *inv, const char *filename) { return 1; }
int saveSalesToFile(const Inventory *inv, const char *filename) { return 1; }
int loadSalesFromFile(Inventory *inv, const char *filename) { return 1; }
