#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "funciones.h"

int main() {
    Inventory inv;
    initInventory(&inv);

    printf("=== Sistema de Gestión de Inventario de Vehículos ===\n");

    int opcion = 0;
    do {
        printf("\nMenu:\n");
        printf("1. Agregar vehículo\n");
        printf("2. Listar vehículos\n");
        printf("3. Agregar cliente\n");
        printf("4. Listar clientes\n");
        printf("5. Registrar venta\n");
        printf("6. Listar ventas\n");
        printf("7. Salir\n");
        printf("Seleccione una opción: ");
        scanf("%d", &opcion);
        getchar(); // limpiar buffer

        if (opcion == 1) {
            Vehicle v;
            printf("Ingrese marca: ");
            fgets(v.marca, STR_LEN, stdin);
            v.marca[strcspn(v.marca, "\n")] = 0;

            printf("Ingrese modelo: ");
            fgets(v.modelo, STR_LEN, stdin);
            v.modelo[strcspn(v.modelo, "\n")] = 0;

            printf("Ingrese año: ");
            scanf("%d", &v.anio);

            printf("Ingrese precio: ");
            scanf("%lf", &v.precio);

            printf("Ingrese kilometraje: ");
            scanf("%d", &v.kilometraje);

            printf("Es nuevo? (1=Si, 0=No): ");
            scanf("%d", &v.esNuevo);

            v.disponible = 1;
            v.id = 0; // se asignará automáticamente

            int id = addVehicle(&inv, &v);
            if (id) {
                printf("Vehículo agregado con ID %d\n", id);
            } else {
                printf("Error al agregar vehículo. Revise los datos.\n");
            }
            getchar(); // limpiar buffer

        } else if (opcion == 2) {
            listVehicles(&inv);

        } else if (opcion == 3) {
            Client c;
            printf("Ingrese nombre: ");
            fgets(c.nombre, STR_LEN, stdin);
            c.nombre[strcspn(c.nombre, "\n")] = 0;

            printf("Ingrese apellido: ");
            fgets(c.apellido, STR_LEN, stdin);
            c.apellido[strcspn(c.apellido, "\n")] = 0;

            printf("Ingrese teléfono (solo dígitos, opcional): ");
            fgets(c.telefono, STR_LEN, stdin);
            c.telefono[strcspn(c.telefono, "\n")] = 0;

            printf("Ingrese presupuesto: ");
            scanf("%lf", &c.presupuesto);
            getchar(); // limpiar buffer

            printf("Ingrese marca preferida (opcional): ");
            fgets(c.marcaPreferida, STR_LEN, stdin);
            c.marcaPreferida[strcspn(c.marcaPreferida, "\n")] = 0;

            int id = addClient(&inv, &c);
            if (id) {
                printf("Cliente agregado con ID %d\n", id);
            } else {
                printf("Error al agregar cliente. Revise los datos.\n");
            }

        } else if (opcion == 4) {
            listClients(&inv);

        } else if (opcion == 5) {
            int vid;
            char buyer[STR_LEN], seller[STR_LEN];
            double price;

            printf("Ingrese ID del vehículo: ");
            scanf("%d", &vid);
            getchar();

            printf("Ingrese nombre del comprador: ");
            fgets(buyer, STR_LEN, stdin);
            buyer[strcspn(buyer, "\n")] = 0;

            printf("Ingrese nombre del vendedor: ");
            fgets(seller, STR_LEN, stdin);
            seller[strcspn(seller, "\n")] = 0;

            printf("Ingrese precio de venta: ");
            scanf("%lf", &price);
            getchar();

            int saleId = registerSale(&inv, vid, buyer, seller, price);
            if (saleId) {
                printf("Venta registrada con ID %d\n", saleId);
            } else {
                printf("Error al registrar venta. Revise los datos.\n");
            }

        } else if (opcion == 6) {
            listSales(&inv);

        } else if (opcion == 7) {
            printf("Saliendo...\n");

        } else {
            printf("Opción inválida\n");
        }

    } while (opcion != 7);

    freeInventory(&inv);
    return 0;
}
