/*
  Interfaz de línea de comandos para el SGIC (según esquema anterior).
  Ahora permite ingresar datos del cliente, buscar vehículos según presupuesto y preferencias,
  mostrar los vehículos más cercanos a lo que busca (ordenados por "cercanía") y registrar la venta.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "funciones.h"

#define INPUT_BUF 256

static void readLine(const char *prompt, char *out, size_t len) {
    printf("%s", prompt);
    if (!fgets(out, (int)len, stdin)) { out[0] = '\0'; return; }
    size_t l = strlen(out);
    if (l > 0 && out[l-1] == '\n') out[l-1] = '\0';
}

static int promptInt(const char *prompt, int *out) {
    char buf[INPUT_BUF]; readLine(prompt, buf, sizeof(buf));
    if (buf[0] == '\0') return 0;
    *out = atoi(buf); return 1;
}

static int promptDouble(const char *prompt, double *out) {
    char buf[INPUT_BUF]; readLine(prompt, buf, sizeof(buf));
    if (buf[0] == '\0') return 0;
    *out = atof(buf); return 1;
}

/* Pedir datos para nuevo vehículo (esquema anterior sin 'tipo') */
static Vehicle promptNewVehicle(void) {
    Vehicle v; memset(&v,0,sizeof(v));
    readLine("Marca: ", v.marca, sizeof(v.marca));
    readLine("Modelo: ", v.modelo, sizeof(v.modelo));
    if (!promptInt("Año (ej: 2020): ", &v.anio)) v.anio = 0;
    if (!promptDouble("Precio: ", &v.precio)) v.precio = 0.0;
    if (!promptInt("Kilometraje: ", &v.kilometraje)) v.kilometraje = 0;
    char buf[INPUT_BUF];
    readLine("Condición (n = nuevo, u = usado) [n/u]: ", buf, sizeof(buf));
    if (buf[0]=='n' || buf[0]=='N') v.esNuevo = 1; else v.esNuevo = 0;
    v.disponible = 1; v.id = 0;
    return v;
}

/* Pedir datos para cliente (nombre, edad, presupuesto y preferencias) */
static Client promptClient(void) {
    Client c; memset(&c,0,sizeof(c));
    c.requiereNuevo = -1;
    c.maxKilometraje = 0;
    c.minAnio = 0;
    c.maxResultados = 5; /* por defecto mostrar top 5 */
    readLine("Nombre del cliente: ", c.nombre, sizeof(c.nombre));
    char buf[INPUT_BUF];
    readLine("Edad del cliente (opcional): ", buf, sizeof(buf));
    c.edad = buf[0] ? atoi(buf) : 0;
    readLine("Presupuesto (ej: 14000.00): ", buf, sizeof(buf));
    c.presupuesto = buf[0] ? atof(buf) : 0.0;
    readLine("Marca preferida (dejar vacío = indiferente): ", c.marcaPreferida, sizeof(c.marcaPreferida));
    readLine("Modelo preferido (dejar vacío = indiferente): ", c.modeloPreferido, sizeof(c.modeloPreferido));
    readLine("Requiere vehículo nuevo? (s/n/enter = indiferente): ", buf, sizeof(buf));
    if (buf[0]=='s' || buf[0]=='S') c.requiereNuevo = 1;
    else if (buf[0]=='n' || buf[0]=='N') c.requiereNuevo = 0;
    readLine("Kilometraje máximo (0 = indiferente): ", buf, sizeof(buf));
    c.maxKilometraje = buf[0] ? atoi(buf) : 0;
    readLine("Año mínimo (0 = indiferente): ", buf, sizeof(buf));
    c.minAnio = buf[0] ? atoi(buf) : 0;
    readLine("Máximo resultados a mostrar (0 = todos) [predeterminado 5]: ", buf, sizeof(buf));
    c.maxResultados = buf[0] ? atoi(buf) : 5;
    return c;
}

static void menu() {
    printf("\n--- Concesionaria Ruedas de Oro - SGIC ---\n");
    printf("1) Añadir vehículo\n");
    printf("2) Eliminar vehículo por ID\n");
    printf("3) Listar vehículos\n");
    printf("4) Buscar vehículo por ID\n");
    printf("5) Buscar por requerimiento (viejo esquema)\n");
    printf("6) Ingresar cliente y buscar vehículos (ordenados por cercanía)\n");
    printf("7) Registrar venta manual\n");
    printf("8) Listar ventas\n");
    printf("9) Guardar inventario/ventas a archivos\n");
    printf("A) Cargar inventario/ventas desde archivos\n");
    printf("0) Salir\n");
    printf("-------------------------------------------\n");
}

int main(void) {
    Inventory inv; initInventory(&inv);
    char opt[INPUT_BUF]; int running = 1;

    while (running) {
        menu();
        readLine("Seleccione una opción: ", opt, sizeof(opt));
        if (opt[0] == '\0') continue;
        switch (opt[0]) {
            case '1': {
                printf("== Añadir vehículo ==\n");
                Vehicle v = promptNewVehicle();
                int id = addVehicle(&inv, &v);
                printf("Vehículo añadido con ID %d\n", id);
                break;
            }
            case '2': {
                int id;
                if (promptInt("ID a eliminar: ", &id)) {
                    if (removeVehicleById(&inv, id)) printf("Vehículo ID %d eliminado.\n", id);
                    else printf("No se encontró vehículo con ID %d.\n", id);
                } else printf("ID no válido.\n");
                break;
            }
            case '3': {
                printf("== Listado de vehículos ==\n");
                listVehicles(&inv);
                break;
            }
            case '4': {
                int id;
                if (promptInt("ID a buscar: ", &id)) {
                    Vehicle *v = findVehicleById(&inv, id);
                    if (v) printVehicle(v);
                    else printf("No encontrado.\n");
                } else printf("ID no válido.\n");
                break;
            }
            case '5': {
                printf("== Búsqueda por requerimiento ==\n");
                Requirement r; memset(&r,0,sizeof(r)); r.requiereNuevo = -1;
                readLine("Marca (dejar vacío = indiferente): ", r.marca, sizeof(r.marca));
                readLine("Modelo (dejar vacío = indiferente): ", r.modelo, sizeof(r.modelo));
                char buf[INPUT_BUF];
                readLine("Año mínimo (0 = indiferente): ", buf, sizeof(buf)); r.minAnio = buf[0] ? atoi(buf) : 0;
                readLine("Año máximo (0 = indiferente): ", buf, sizeof(buf)); r.maxAnio = buf[0] ? atoi(buf) : 0;
                readLine("Precio máximo (0 = indiferente): ", buf, sizeof(buf)); r.maxPrecio = buf[0] ? atof(buf) : 0.0;
                readLine("Kilometraje máximo (0 = indiferente): ", buf, sizeof(buf)); r.maxKilometraje = buf[0] ? atoi(buf) : 0;
                readLine("Requiere vehículo nuevo? (s/n/enter = indiferente): ", buf, sizeof(buf));
                if (buf[0]=='s'||buf[0]=='S') r.requiereNuevo = 1; else if(buf[0]=='n'||buf[0]=='N') r.requiereNuevo = 0;
                size_t found = 0; Vehicle *res = findVehiclesByRequirement(&inv, &r, &found);
                if (found == 0) printf("No se encontraron vehículos.\n");
                else {
                    printf("Se encontraron %zu vehículo(s):\n", found);
                    for (size_t i=0;i<found;++i) { printVehicle(&res[i]); printf("---------------------------------\n"); }
                }
                free(res);
                break;
            }
            case '6': {
                printf("== Ingresar cliente y buscar vehículos más cercanos ==\n");
                Client c = promptClient();
                if (c.presupuesto <= 0.0) { printf("Presupuesto debe ser mayor a 0.\n"); break; }
                size_t found = 0;
                Vehicle *res = findVehiclesForClient(&inv, &c, &found);
                if (!res || found == 0) {
                    printf("No se encontraron vehículos que cumplan con el presupuesto y preferencias.\n");
                    free(res);
                    break;
                }
                printf("Mostrando %zu vehículo(s) ordenados por cercanía a las preferencias de %s:\n", found, c.nombre);
                for (size_t i = 0; i < found; ++i) {
                    printVehicle(&res[i]);
                    printf("---------------------------------\n");
                }
                /* Ofrecer registrar compra para el cliente */
                char buf[INPUT_BUF];
                readLine("¿Desea registrar la compra de alguno de estos vehículos para el cliente? (s/n): ", buf, sizeof(buf));
                if (buf[0]=='s' || buf[0]=='S') {
                    int vid;
                    if (!promptInt("Ingrese ID del vehículo a vender: ", &vid)) { printf("ID inválido.\n"); free(res); break; }
                    Vehicle *vsel = findVehicleById(&inv, vid);
                    if (!vsel) { printf("Vehículo no encontrado.\n"); free(res); break; }
                    if (!vsel->disponible) { printf("Vehículo no está disponible.\n"); free(res); break; }
                    char seller[STR_LEN] = "Jose";
                    readLine("Nombre del vendedor/asesor (Enter para 'Jose'): ", buf, sizeof(buf));
                    if (buf[0]) strncpy(seller, buf, STR_LEN-1);
                    double price = vsel->precio;
                    char pricebuf[INPUT_BUF];
                    snprintf(pricebuf, sizeof(pricebuf), "%.2f", price);
                    readLine("Precio de venta (Enter para precio listado): ", pricebuf, sizeof(pricebuf));
                    if (pricebuf[0]) price = atof(pricebuf);
                    int sale_id = registerSale(&inv, vid, c.nombre, seller, price);
                    if (sale_id > 0) {
                        printf("Venta registrada ID %d. Vehículo %d marcado como vendido.\n", sale_id, vid);
                    } else {
                        printf("No se pudo registrar la venta.\n");
                    }
                } else {
                    printf("No se registró ninguna venta.\n");
                }
                free(res);
                break;
            }
            case '7': {
                printf("== Registrar venta manual ==\n");
                int vid;
                if (!promptInt("ID vehículo a vender: ", &vid)) { printf("ID inválido.\n"); break; }
                Vehicle *vsel = findVehicleById(&inv, vid);
                if (!vsel) { printf("Vehículo no encontrado.\n"); break; }
                if (!vsel->disponible) { printf("Vehículo no disponible.\n"); break; }
                char buyer[STR_LEN], seller[STR_LEN], buf[INPUT_BUF];
                readLine("Nombre del comprador: ", buyer, sizeof(buyer));
                readLine("Nombre del vendedor/asesor: ", seller, sizeof(seller));
                double price;
                promptDouble("Precio de venta: ", &price);
                int sale_id = registerSale(&inv, vid, buyer, seller, price);
                if (sale_id > 0) printf("Venta registrada con ID %d\n", sale_id);
                else printf("Error registrando la venta.\n");
                break;
            }
            case '8': {
                printf("== Ventas registradas ==\n");
                listSales(&inv);
                break;
            }
            case '9': {
                char fname[INPUT_BUF];
                readLine("Archivo para guardar inventario (ej: inventario.csv): ", fname, sizeof(fname));
                if (fname[0]) { if (saveInventoryToFile(&inv, fname)) printf("Inventario guardado en %s\n", fname); else printf("Error guardando inventario.\n"); }
                readLine("Archivo para guardar ventas (ej: ventas.csv): ", fname, sizeof(fname));
                if (fname[0]) { if (saveSalesToFile(&inv, fname)) printf("Ventas guardadas en %s\n", fname); else printf("Error guardando ventas.\n"); }
                break;
            }
            case 'A': case 'a': {
                char fname[INPUT_BUF];
                readLine("Archivo para cargar inventario (ej: inventario.csv): ", fname, sizeof(fname));
                if (fname[0]) { if (loadInventoryFromFile(&inv, fname)) printf("Inventario cargado desde %s\n", fname); else printf("Error cargando inventario.\n"); }
                readLine("Archivo para cargar ventas (ej: ventas.csv): ", fname, sizeof(fname));
                if (fname[0]) { if (loadSalesFromFile(&inv, fname)) printf("Ventas cargadas desde %s\n", fname); else printf("Error cargando ventas.\n"); }
                break;
            }
            case '0': running = 0; break;
            default: printf("Opción no válida.\n");
        }
    }

    freeInventory(&inv);
    printf("Saliendo. ¡Hasta luego!\n");
    return 0;
}