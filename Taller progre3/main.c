/*
  Interfaz de linea de comandos para el SGIC
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "funciones.h"

#define INPUT_BUF 256

/* ================== Helpers seguros ================== */

static void readLine(const char *prompt, char *out, size_t len) {
    printf("%s", prompt);
    if (!fgets(out, (int)len, stdin)) {
        out[0] = '\0';
        return;
    }
    size_t l = strlen(out);
    if (l > 0 && out[l - 1] == '\n') out[l - 1] = '\0';
}

/* Solo letras */
static int isOnlyLettersLocal(const char *s) {
    if (!s || !*s) return 0;
    for (size_t i = 0; s[i]; i++) {
        if (!isalpha((unsigned char)s[i]) && s[i] != ' ')
            return 0;
    }
    return 1;
}

/* Entero positivo */
static int promptPositiveInt(const char *prompt) {
    char buf[INPUT_BUF];
    int val;

    while (1) {
        readLine(prompt, buf, sizeof(buf));
        if (sscanf(buf, "%d", &val) == 1 && val > 0)
            return val;
        printf("Valor invalido. Intente otra vez.\n");
    }
}

/* Año válido */
static int promptValidYear(const char *prompt) {
    char buf[INPUT_BUF];
    int anio;

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    int maxYear = tm->tm_year + 1900 + 1;

    while (1) {
        readLine(prompt, buf, sizeof(buf));
        if (sscanf(buf, "%d", &anio) == 1 && anio >= 1980 && anio <= maxYear)
            return anio;

        printf("Año invalido. Rango permitido: 1980 - %d\n", maxYear);
    }
}

/* Double positivo */
static double promptPositiveDouble(const char *prompt) {
    char buf[INPUT_BUF];
    double val;

    while (1) {
        readLine(prompt, buf, sizeof(buf));
        if (sscanf(buf, "%lf", &val) == 1 && val > 0)
            return val;
        printf("Valor invalido. Intente otra vez.\n");
    }
}

/* Letras obligatorias */
static void promptLetters(const char *prompt, char *out, size_t len) {
    while (1) {
        readLine(prompt, out, len);
        if (isOnlyLettersLocal(out))
            return;
        printf("Entrada invalida. Solo letras y espacios.\n");
    }
}

/* ================== Entrada Vehiculo ================== */

static Vehicle promptNewVehicle(void) {
    Vehicle v;
    memset(&v, 0, sizeof(v));

    promptLetters("Marca: ", v.marca, sizeof(v.marca));
    promptLetters("Modelo: ", v.modelo, sizeof(v.modelo));

    v.anio = promptValidYear("Año (1980 - actual+1): ");
    v.precio = promptPositiveDouble("Precio: ");
    v.kilometraje = promptPositiveInt("Kilometraje: ");

    char buf[INPUT_BUF];
    while (1) {
        readLine("Condicion (n = nuevo, u = usado): ", buf, sizeof(buf));
        if (buf[0] == 'n' || buf[0] == 'N') {
            v.esNuevo = 1;
            break;
        }
        if (buf[0] == 'u' || buf[0] == 'U') {
            v.esNuevo = 0;
            break;
        }
        printf("Opcion invalida. Use n o u.\n");
    }

    v.disponible = 1;
    v.id = 0;
    return v;
}

/* ================== Entrada Cliente ================== */

static Client promptClient(void) {
    Client c;
    memset(&c, 0, sizeof(c));

    promptLetters("Nombre del cliente: ", c.nombre, sizeof(c.nombre));
    c.presupuesto = promptPositiveDouble("Presupuesto: ");

    readLine("Marca preferida (enter = indiferente): ",
             c.marcaPreferida, sizeof(c.marcaPreferida));

    if (c.marcaPreferida[0] && !isOnlyLettersLocal(c.marcaPreferida)) {
        printf("Marca invalida, se ignorara.\n");
        c.marcaPreferida[0] = '\0';
    }

    c.maxResultados = 5;
    c.requiereNuevo = -1;

    return c;
}

/* ================== Menu ================== */

static void menu(void) {
    printf("\n--- SGIC ---\n");
    printf("1) Agregar vehiculo\n");
    printf("2) Eliminar vehiculo por ID\n");
    printf("3) Listar vehiculos\n");
    printf("4) Buscar vehiculo por ID\n");
    printf("5) Buscar por cliente (cercania)\n");
    printf("6) Registrar venta\n");
    printf("7) Listar ventas\n");
    printf("0) Salir\n");
    printf("-----------------------------\n");
}

int main(void) {
    Inventory inv;
    initInventory(&inv);

    char opt[INPUT_BUF];
    int running = 1;

    while (running) {
        menu();
        readLine("Seleccione opcion: ", opt, sizeof(opt));

        switch (opt[0]) {

            case '1': {
                Vehicle v = promptNewVehicle();
                int id = addVehicle(&inv, &v);
                printf("Vehiculo agregado con ID %d\n", id);
                break;
            }

            case '2': {
                int id = promptPositiveInt("ID a eliminar: ");
                if (removeVehicleById(&inv, id))
                    printf("Vehiculo eliminado.\n");
                else
                    printf("No existe ese vehiculo.\n");
                break;
            }

            case '3':
                listVehicles(&inv);
                break;

            case '4': {
                int id = promptPositiveInt("ID a buscar: ");
                Vehicle *v = findVehicleById(&inv, id);
                if (v) printVehicle(v);
                else printf("No encontrado.\n");
                break;
            }

            case '5': {
                Client c = promptClient();
                size_t found = 0;

                Vehicle *res = findVehiclesForClient(&inv, &c, &found);
                if (!res || found == 0) {
                    printf("No se encontraron vehiculos.\n");
                    free(res);
                    break;
                }

                for (size_t i = 0; i < found; i++) {
                    printVehicle(&res[i]);
                    printf("-----------------------\n");
                }
                free(res);
                break;
            }

            case '6': {
                int id = promptPositiveInt("ID vehiculo: ");
                char buyer[STR_LEN], seller[STR_LEN];

                promptLetters("Comprador: ", buyer, sizeof(buyer));
                promptLetters("Vendedor: ", seller, sizeof(seller));

                double price = promptPositiveDouble("Precio venta: ");

                if (registerSale(&inv, id, buyer, seller, price))
                    printf("Venta registrada.\n");
                else
                    printf("Error al registrar venta.\n");
                break;
            }

            case '7':
                listSales(&inv);
                break;

            case '0':
                running = 0;
                break;

            default:
                printf("Opcion no valida.\n");
        }
    }

    freeInventory(&inv);
    return 0;
}
