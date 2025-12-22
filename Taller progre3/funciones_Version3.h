#ifndef FUNCIONES_H
#define FUNCIONES_H

#include <stddef.h>

#define STR_LEN 64
#define DATE_LEN 20

typedef struct {
    int id;
    char marca[STR_LEN];
    char modelo[STR_LEN];
    int anio;
    double precio;
    int kilometraje;
    int esNuevo;    /* 1 = nuevo, 0 = usado */
    int disponible; /* 1 = disponible, 0 = vendido/no disponible */
} Vehicle;

typedef struct {
    Vehicle *vehiculos;
    size_t size;
    size_t capacity;
    /* ventas en memoria */
    struct Sale *sales;
    size_t sales_size;
    size_t sales_capacity;
} Inventory;

typedef struct {
    char marca[STR_LEN];     /* "" para indiferente */
    char modelo[STR_LEN];    /* "" para indiferente */
    int minAnio;             /* 0 para indiferente */
    int maxAnio;             /* 0 para indiferente */
    double maxPrecio;        /* 0 para indiferente */
    int maxKilometraje;      /* 0 para indiferente */
    int requiereNuevo;       /* -1 indiferente, 1 nuevo, 0 usado */
} Requirement;

/* Cliente -> incluye presupuesto y preferencias; se usará para búsqueda y ordenación por cercanía */
typedef struct {
    char nombre[STR_LEN];
    int edad;
    double presupuesto;
    char marcaPreferida[STR_LEN];   /* "" para indiferente */
    char modeloPreferido[STR_LEN];  /* "" para indiferente */
    int requiereNuevo; /* -1 indiferente, 1 nuevo, 0 usado */
    int maxKilometraje; /* 0 indiferente */
    int minAnio; /* 0 indiferente */
    int maxResultados; /* máximo de resultados a mostrar (0 = mostrar todos) */
} Client;

typedef struct Sale {
    int sale_id;
    int vehicle_id;
    char buyer[STR_LEN];
    char seller[STR_LEN];
    double price;
    char date[DATE_LEN]; /* YYYY-MM-DD */
} Sale;

/* Inicialización y liberación */
void initInventory(Inventory *inv);
void freeInventory(Inventory *inv);

/* Operaciones básicas */
int addVehicle(Inventory *inv, const Vehicle *v); /* retorna id del vehículo añadido */
int removeVehicleById(Inventory *inv, int id);     /* retorna 1 si removido, 0 si no encontrado */
Vehicle *findVehicleById(Inventory *inv, int id);  /* retorna puntero dentro del inventario o NULL */
int updateVehicle(Inventory *inv, int id, const Vehicle *v); /* 1 si actualizado */

/* Búsquedas */
Vehicle *findVehiclesByRequirement(Inventory *inv, const Requirement *req, size_t *out_count);
/* Devuelve arreglo malloc con copias de Vehicles; el llamador debe free() */

/* Búsqueda adaptada a un cliente: filtra por presupuesto/preferencias y devuelve vehículos ordenados por "cercanía".
   Devuelve arreglo malloc que debe liberarse con free(). */
Vehicle *findVehiclesForClient(Inventory *inv, const Client *c, size_t *out_count);

/* Listado y utilidades */
void listVehicles(const Inventory *inv);
void printVehicle(const Vehicle *v);

/* Persistencia (CSV simple) */
int saveInventoryToFile(const Inventory *inv, const char *filename);
int loadInventoryFromFile(Inventory *inv, const char *filename);

/* Ventas: registrar y listar */
int registerSale(Inventory *inv, int vehicleId, const char *buyer, const char *seller, double price); /* retorna sale_id (>0) o 0 en error */
void listSales(const Inventory *inv);
int saveSalesToFile(const Inventory *inv, const char *filename);
int loadSalesFromFile(Inventory *inv, const char *filename);

/* Marcar vehículo vendido/disponible */
int markSold(Inventory *inv, int id);   /* 1 ok, 0 no encontrado */
int markAvailable(Inventory *inv, int id); /* volver disponible */

/* Generadores de IDs (internos expuestos) */
int getNextId(void);
void setNextId(int start);
int getNextSaleId(void);
void setNextSaleId(int start);

#endif /* FUNCIONES_H */