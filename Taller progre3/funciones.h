#ifndef FUNCIONES_H
#define FUNCIONES_H

#include <stdio.h>

#define STR_LEN 64
#define DATE_LEN 20

/* ================== Estructuras ================== */

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

typedef struct Sale {
    int sale_id;
    int vehicle_id;
    char buyer[STR_LEN];
    char seller[STR_LEN];
    double price;
    char date[DATE_LEN]; /* YYYY-MM-DD */
} Sale;

/* Cliente -> incluye presupuesto y preferencias; se usará para búsqueda y ordenación por cercanía */
typedef struct {
    int id;
    char nombre[STR_LEN];
    char apellido[STR_LEN];
    char telefono[STR_LEN];         /* opcional, solo dígitos */
    double presupuesto;
    char marcaPreferida[STR_LEN];   /* "" para indiferente */
} Client;

typedef struct {
    Vehicle *vehiculos;
    size_t size;
    size_t capacity;

    Sale *sales;
    size_t sales_size;
    size_t sales_capacity;

    Client *clientes;
    size_t clientes_size;
    size_t clientes_capacity;
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

/* ================== Inicialización y liberación ================== */

void initInventory(Inventory *inv);
void freeInventory(Inventory *inv);

/* ================== Vehículos ================== */

int addVehicle(Inventory *inv, const Vehicle *v); 
int removeVehicleById(Inventory *inv, int id);     
Vehicle *findVehicleById(Inventory *inv, int id);  
int updateVehicle(Inventory *inv, int id, const Vehicle *v); 

Vehicle *findVehiclesByRequirement(Inventory *inv, const Requirement *req, size_t *out_count);
Vehicle *findVehiclesForClient(Inventory *inv, const Client *c, size_t *out_count);

void listVehicles(const Inventory *inv);
void printVehicle(const Vehicle *v);

int markSold(Inventory *inv, int id);   
int markAvailable(Inventory *inv, int id); 

/* ================== Clientes ================== */

int addClient(Inventory *inv, const Client *c);       /* retorna id del cliente agregado */
Client *findClientById(Inventory *inv, int id);       /* retorna puntero dentro del inventario o NULL */
int removeClientById(Inventory *inv, int id);         /* 1 si eliminado, 0 si no existe */
int updateClient(Inventory *inv, int id, const Client *c); /* 1 si actualizado */
void listClients(const Inventory *inv);

/* ================== Ventas ================== */

int registerSale(Inventory *inv, int vehicleId, const char *buyer, const char *seller, double price); 
void listSales(const Inventory *inv);

int saveSalesToFile(const Inventory *inv, const char *filename);
int loadSalesFromFile(Inventory *inv, const char *filename);

/* ================== Persistencia Inventario ================== */

int saveInventoryToFile(const Inventory *inv, const char *filename);
int loadInventoryFromFile(Inventory *inv, const char *filename);

/* ================== IDs ================== */

int getNextId(void);
void setNextId(int start);
int getNextSaleId(void);
void setNextSaleId(int start);

#endif /* FUNCIONES_H */
