#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Constants
#define MAX_PARKING_SPACES 50
#define FREE 0
#define OCCUPIED 1
#define GOLDEN_HOURS 200
#define PREMIUM_HOURS 100
#define GOLD 2
#define PREMIUM 1
#define NONE 0
#define BASE_FEES 100
#define EXTRA_FEES 50
#define DISCOUNT 0.10

// B+ Tree parameters
#define MAX_KEYS 4
#define MIN_KEYS ((MAX_KEYS + 1) / 2)

// Structure for arrival and departure times
struct datetime {
    int time;
    int date;
    int month;
    int year;
};

// Structure for vehicle information
typedef struct Vehicle {
    char vehicle_num[10];
    char owner_name[20];
    struct datetime arrival;
    struct datetime departure;
    int membership;
    int total_parking_hours;
    int parking_ID;
    int total_amount_paid;
    int parking_count;
} Vehicle;

// B+ Tree Node structure
typedef struct BPTreeNode {
    bool isLeaf;
    char keys[MAX_KEYS][10]; // Using vehicle number as key
    Vehicle* vehicles[MAX_KEYS]; // For leaf nodes
    struct BPTreeNode* children[MAX_KEYS + 1];
    int numKeys;
    struct BPTreeNode* next; // Pointer to the next leaf node
    struct BPTreeNode* prev; // Pointer to the previous leaf node
} BPTreeNode;

// Structure for parking space
typedef struct ParkingSpace {
    int parking_space_ID;
    int status;
    int occupancy_count;
    int space_revenue;
} ParkingSpace;

// B+ Tree Node structure for parking spaces
typedef struct ParkingSpaceNode {
    bool isLeaf;
    int keys[MAX_KEYS]; // Parking space IDs as keys
    ParkingSpace* spaces[MAX_KEYS]; // Parking space data for leaf nodes
    struct ParkingSpaceNode* children[MAX_KEYS + 1];
    int numKeys;
    struct ParkingSpaceNode* next; // Pointer to the next leaf node
    struct ParkingSpaceNode* prev; // Pointer to the previous leaf node
} ParkingSpaceNode;

// Temporary B+ Tree Node structure for sorting by revenue
typedef struct TempBPTreeNode {
    bool isLeaf;
    int keys[MAX_KEYS]; // Sorting by revenue
    ParkingSpace* spaces[MAX_KEYS]; // Pointer to parking space data
    struct TempBPTreeNode* children[MAX_KEYS + 1];
    int numKeys;
    struct TempBPTreeNode* next;
    struct TempBPTreeNode* prev;
} TempBPTreeNode;

// Global variables
BPTreeNode* vehicle_tree = NULL;
ParkingSpaceNode* parking_space_tree = NULL;
ParkingSpace* parking_spaces[MAX_PARKING_SPACES];
int vehicle_count = 0;

// Forward declarations
BPTreeNode* createNode(bool isLeaf);
ParkingSpaceNode* createParkingSpaceNode(bool isLeaf);
TempBPTreeNode* createTempNode(bool isLeaf);
BPTreeNode* findParent(BPTreeNode* current, BPTreeNode* child);
void insertInternal(char* key, BPTreeNode* parent, BPTreeNode* child);
void insertVehicle(Vehicle* vehicle);
Vehicle* searchVehicle(BPTreeNode* node, char* vehicle_num);
ParkingSpace* searchParkingSpace(ParkingSpaceNode* node, int parking_space_ID);
void initialize_parking_spaces();
int find_parking_space(int membership);
int calculate_parking_fee(int hours_parked, int membership);
int days_in_month(int month);
int totaldays(int date, int month, int year);
int date_difference(int date1, int month1, int year1, int date2, int month2, int year2);
int hours_parked(struct datetime arrival, struct datetime departure);
void park_vehicle();
void exit_vehicle();
void arrangeVehiclesByParkingCount(BPTreeNode* root);
void arrangeVehiclesByAmountPaid(BPTreeNode* root, int minAmount, int maxAmount);
void arrangeParkingSpacesByOccupancy();
void arrangeParkingSpacesByRevenue();
void printLeafNodesVisual(BPTreeNode* root);
void printAllVehicles(BPTreeNode* root);
void load_data();
void save_data();
void insertInternalParkingSpace(int key, ParkingSpaceNode* parent, ParkingSpaceNode* child);
ParkingSpaceNode* findParentParkingSpace(ParkingSpaceNode* current, ParkingSpaceNode* child);
void insertParkingSpace(ParkingSpace* space);
void displayParkingSpaces(ParkingSpaceNode* root);
void insertIntoTempTree(TempBPTreeNode** root, ParkingSpace* space, int key);
void traverseTempTree(TempBPTreeNode* root);
void insertVehicleIntoTempTree(TempBPTreeNode** root, Vehicle* vehicle);
void traverseVehicleTempTree(TempBPTreeNode* root);

// B+ Tree functions
BPTreeNode* createNode(bool isLeaf) {
    BPTreeNode* newNode = (BPTreeNode*)malloc(sizeof(BPTreeNode));
    newNode->isLeaf = isLeaf;
    newNode->numKeys = 0;
    newNode->next = NULL;
    newNode->prev = NULL; // Initialize the prev pointer
    for (int i = 0; i < MAX_KEYS + 1; i++) {
        newNode->children[i] = NULL;
        if (isLeaf) newNode->vehicles[i] = NULL;
    }
    return newNode;
}

ParkingSpaceNode* createParkingSpaceNode(bool isLeaf) {
    ParkingSpaceNode* newNode = (ParkingSpaceNode*)malloc(sizeof(ParkingSpaceNode));
    newNode->isLeaf = isLeaf;
    newNode->numKeys = 0;
    newNode->next = NULL;
    newNode->prev = NULL; // Initialize the prev pointer
    for (int i = 0; i < MAX_KEYS + 1; i++) {
        newNode->children[i] = NULL;
        if (isLeaf) newNode->spaces[i] = NULL;
    }
    return newNode;
}

TempBPTreeNode* createTempNode(bool isLeaf) {
    TempBPTreeNode* newNode = (TempBPTreeNode*)malloc(sizeof(TempBPTreeNode));
    newNode->isLeaf = isLeaf;
    newNode->numKeys = 0;
    newNode->next = NULL;
    newNode->prev = NULL;
    for (int i = 0; i < MAX_KEYS + 1; i++) {
        newNode->children[i] = NULL;
        if (isLeaf) newNode->spaces[i] = NULL;
    }
    return newNode;
}

BPTreeNode* findParent(BPTreeNode* current, BPTreeNode* child) {
    if (current == NULL || current->isLeaf) return NULL;

    for (int i = 0; i <= current->numKeys; i++) {
        if (current->children[i] == child) return current;
        BPTreeNode* p = findParent(current->children[i], child);
        if (p != NULL) return p;
    }
    return NULL;
}

void insertInternal(char* key, BPTreeNode* parent, BPTreeNode* child) {
    if (parent->numKeys < MAX_KEYS) {
        int i = parent->numKeys - 1;
        while (i >= 0 && strcmp(parent->keys[i], key) > 0) {
            strcpy(parent->keys[i + 1], parent->keys[i]);
            parent->children[i + 2] = parent->children[i + 1];
            i--;
        }
        strcpy(parent->keys[i + 1], key);
        parent->children[i + 2] = child;
        parent->numKeys++;
    } else {
        char tempKeys[MAX_KEYS + 1][10];
        BPTreeNode* tempChildren[MAX_KEYS + 2];

        for (int i = 0; i < MAX_KEYS; i++) {
            strcpy(tempKeys[i], parent->keys[i]);
        }
        for (int i = 0; i < MAX_KEYS + 1; i++) {
            tempChildren[i] = parent->children[i];
        }

        int i = MAX_KEYS - 1;
        while (i >= 0 && strcmp(tempKeys[i], key) > 0) {
            strcpy(tempKeys[i + 1], tempKeys[i]);
            tempChildren[i + 2] = tempChildren[i + 1];
            i--;
        }
        strcpy(tempKeys[i + 1], key);
        tempChildren[i + 2] = child;

        BPTreeNode* newInternal = createNode(false);
        parent->numKeys = MIN_KEYS;
        newInternal->numKeys = MAX_KEYS - MIN_KEYS;

        for (i = 0; i < parent->numKeys; i++) {
            strcpy(parent->keys[i], tempKeys[i]);
            parent->children[i] = tempChildren[i];
        }
        parent->children[i] = tempChildren[i];

        for (int j = 0; j < newInternal->numKeys; j++) {
            strcpy(newInternal->keys[j], tempKeys[i + 1 + j]);
            newInternal->children[j] = tempChildren[i + 1 + j];
        }
        newInternal->children[newInternal->numKeys] = tempChildren[MAX_KEYS + 1];

        if (parent == vehicle_tree) {
            BPTreeNode* newRoot = createNode(false);
            strcpy(newRoot->keys[0], tempKeys[MIN_KEYS]);
            newRoot->children[0] = parent;
            newRoot->children[1] = newInternal;
            newRoot->numKeys = 1;
            vehicle_tree = newRoot;
        } else {
            insertInternal(tempKeys[MIN_KEYS], findParent(vehicle_tree, parent), newInternal);
        }
    }
}

void insertVehicle(Vehicle* vehicle) {
    if (vehicle_tree == NULL) {
        vehicle_tree = createNode(true);
        strcpy(vehicle_tree->keys[0], vehicle->vehicle_num);
        vehicle_tree->vehicles[0] = vehicle;
        vehicle_tree->numKeys = 1;
        return;
    }

    BPTreeNode* current = vehicle_tree;
    BPTreeNode* parent = NULL;

    while (!current->isLeaf) {
        parent = current;
        int i = 0;
        while (i < current->numKeys && strcmp(vehicle->vehicle_num, current->keys[i]) > 0) i++;
        current = current->children[i];
    }

    if (current->numKeys < MAX_KEYS) {
        int i = current->numKeys - 1;
        while (i >= 0 && strcmp(vehicle->vehicle_num, current->keys[i]) < 0) {
            strcpy(current->keys[i + 1], current->keys[i]);
            current->vehicles[i + 1] = current->vehicles[i];
            i--;
        }
        strcpy(current->keys[i + 1], vehicle->vehicle_num);
        current->vehicles[i + 1] = vehicle;
        current->numKeys++;
    } else {
        char tempKeys[MAX_KEYS + 1][10];
        Vehicle* tempVehicles[MAX_KEYS + 1];

        for (int i = 0; i < MAX_KEYS; i++) {
            strcpy(tempKeys[i], current->keys[i]);
            tempVehicles[i] = current->vehicles[i];
        }

        int i = MAX_KEYS - 1;
        while (i >= 0 && strcmp(vehicle->vehicle_num, tempKeys[i]) < 0) {
            strcpy(tempKeys[i + 1], tempKeys[i]);
            tempVehicles[i + 1] = tempVehicles[i];
            i--;
        }
        strcpy(tempKeys[i + 1], vehicle->vehicle_num);
        tempVehicles[i + 1] = vehicle;

        BPTreeNode* newLeaf = createNode(true);
        current->numKeys = MIN_KEYS;
        newLeaf->numKeys = MAX_KEYS + 1 - current->numKeys;

        for (i = 0; i < current->numKeys; i++) {
            strcpy(current->keys[i], tempKeys[i]);
            current->vehicles[i] = tempVehicles[i];
        }
        for (i = 0; i < newLeaf->numKeys; i++) {
            strcpy(newLeaf->keys[i], tempKeys[current->numKeys + i]);
            newLeaf->vehicles[i] = tempVehicles[current->numKeys + i];
        }

        // Update the doubly linked list pointers
        newLeaf->next = current->next;
        if (current->next != NULL) {
            current->next->prev = newLeaf;
        }
        current->next = newLeaf;
        newLeaf->prev = current;

        if (current == vehicle_tree) {
            BPTreeNode* newRoot = createNode(false);
            strcpy(newRoot->keys[0], newLeaf->keys[0]);
            newRoot->children[0] = current;
            newRoot->children[1] = newLeaf;
            newRoot->numKeys = 1;
            vehicle_tree = newRoot;
        } else {
            insertInternal(newLeaf->keys[0], findParent(vehicle_tree, current), newLeaf);
        }
    }
}

Vehicle* searchVehicle(BPTreeNode* node, char* vehicle_num) {
    if (node == NULL) return NULL;

    int i = 0;
    while (i < node->numKeys && strcmp(vehicle_num, node->keys[i]) > 0) i++;

    if (node->isLeaf) {
        for (int j = 0; j < node->numKeys; j++) {
            if (strcmp(node->keys[j], vehicle_num) == 0) {
                return node->vehicles[j];
            }
        }
        return NULL;
    } else {
        return searchVehicle(node->children[i], vehicle_num);
    }
}

ParkingSpace* searchParkingSpace(ParkingSpaceNode* node, int parking_space_ID) {
    if (node == NULL) return NULL;

    int i = 0;
    while (i < node->numKeys && parking_space_ID > node->keys[i]) i++;

    if (node->isLeaf) {
        for (int j = 0; j < node->numKeys; j++) {
            if (node->keys[j] == parking_space_ID) {
                return node->spaces[j];
            }
        }
        return NULL;
    } else {
        return searchParkingSpace(node->children[i], parking_space_ID);
    }
}

// Parking system functions
void initialize_parking_spaces() {
    for (int i = 0; i < MAX_PARKING_SPACES; i++) {
        ParkingSpace* space = (ParkingSpace*)malloc(sizeof(ParkingSpace));
        space->parking_space_ID = i + 1;
        space->status = FREE;
        space->occupancy_count = 0;
        space->space_revenue = 0;
        parking_spaces[i] = space; // <-- store pointer, not struct
        insertParkingSpace(space);
    }
    printf("Parking spaces initialized and inserted into B+ tree.\n");
}

int find_parking_space(int membership) {
    int start, end;

    if (membership == GOLD) {
        start = 0;
        end = 10;
    } else if (membership == PREMIUM) {
        start = 10;
        end = 20;
    } else {
        start = 20;
        end = MAX_PARKING_SPACES;
    }

    for (int i = start; i < end; i++) {
        if (parking_spaces[i]->status == FREE) { // <-- use pointer
            return i + 1; // Return 1-based ID
        }
    }
    return -1; // No space available
}

int calculate_parking_fee(int hours_parked, int membership) {
    int fee = BASE_FEES;
    if (hours_parked > 3) {
        fee += (hours_parked - 3) * EXTRA_FEES;
    }
    if (membership > 0) {
        fee = (int)(fee * (1.0 - DISCOUNT));
    }
    return fee;
}

// Helper functions for date calculations
int days_in_month(int month) {
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return days[month - 1];
}

int totaldays(int date, int month, int year) {
    int days = 0;
    for (int i = 1990; i < year; i++) {
        days += 365;
    }
    for (int i = 1; i < month; i++) {
        days += days_in_month(i);
    }
    days += date;
    return days;
}

int date_difference(int date1, int month1, int year1, int date2, int month2, int year2) {
    int days1 = totaldays(date1, month1, year1);
    int days2 = totaldays(date2, month2, year2);
    return days2 - days1;
}

int hours_parked(struct datetime arrival, struct datetime departure) {
    int total_days_difference = date_difference(arrival.date, arrival.month, arrival.year,
                                              departure.date, departure.month, departure.year);
    int total_hours = total_days_difference * 24 + (departure.time - arrival.time);
    if (total_hours < 0) {
        total_hours += 24;
    }
    return total_hours;
}

void park_vehicle() {
    char vehicle_num[10], owner_name[20];
    struct datetime arrival;

    printf("Enter vehicle number: ");
    scanf("%9s", vehicle_num); // width specifier
    printf("Enter arrival time (24-hour format): ");
    scanf("%d", &arrival.time);
    printf("Enter arrival date: ");
    scanf("%d", &arrival.date);
    printf("Enter arrival month: ");
    scanf("%d", &arrival.month);
    printf("Enter arrival year: ");
    scanf("%d", &arrival.year);
    printf("Enter the owner name: ");
    scanf("%19s", owner_name); // width specifier

    if (arrival.date < 1 || arrival.date > days_in_month(arrival.month)) {
        printf("Invalid arrival date.\n");
        return;
    }

    Vehicle* existing_vehicle = searchVehicle(vehicle_tree, vehicle_num);
    Vehicle* vehicle;

    if (existing_vehicle == NULL) {
        vehicle = (Vehicle*)malloc(sizeof(Vehicle));
        strcpy(vehicle->vehicle_num, vehicle_num);
        strcpy(vehicle->owner_name, owner_name);
        vehicle->membership = NONE;
        vehicle->total_parking_hours = 0;
        vehicle->total_amount_paid = 0;
        vehicle->parking_count = 0;

        insertVehicle(vehicle);
        vehicle_count++;
    } else {
        vehicle = existing_vehicle;
    }

    vehicle->arrival = arrival;

    int parking_space_id = find_parking_space(vehicle->membership);
    if (parking_space_id != -1) {
        vehicle->parking_ID = parking_space_id;

        // Update the parking space status in the B+ tree
        ParkingSpace* space = searchParkingSpace(parking_space_tree, parking_space_id);
        if (space != NULL) {
            space->status = OCCUPIED;
            space->occupancy_count++;
        }

        printf("Vehicle parked at space %d\n", parking_space_id);
    } else {
        printf("No suitable parking space available.\n");
    }
}

void exit_vehicle() {
    char vehicle_num[10];
    struct datetime departure;

    printf("Enter vehicle number: ");
    scanf("%9s", vehicle_num); // width specifier
    printf("Enter departure time (24-hour format): ");
    scanf("%d", &departure.time);
    printf("Enter departure date: ");
    scanf("%d", &departure.date);
    printf("Enter departure month: ");
    scanf("%d", &departure.month);
    printf("Enter departure year: ");
    scanf("%d", &departure.year);

    Vehicle* vehicle = searchVehicle(vehicle_tree, vehicle_num);
    if (vehicle != NULL) {
        int parked_hours = hours_parked(vehicle->arrival, departure);
        vehicle->total_parking_hours += parked_hours;
        vehicle->departure = departure;

        int fee = calculate_parking_fee(parked_hours, vehicle->membership);
        vehicle->total_amount_paid += fee;
        vehicle->parking_count++;

        if (vehicle->parking_ID > 0 && vehicle->parking_ID <= MAX_PARKING_SPACES) {
            // Update the parking space in the B+ tree
            ParkingSpace* space = searchParkingSpace(parking_space_tree, vehicle->parking_ID);
            if (space != NULL) {
                space->status = FREE;
                space->occupancy_count += vehicle->parking_count;
                space->space_revenue += vehicle->total_amount_paid;
                if (vehicle->parking_ID > 0 && vehicle->membership != NONE) {
                    space->status = OCCUPIED; // Optional: Mark as occupied if you want to show current status
                }
            }
        }

        // Update membership status
        if (vehicle->total_parking_hours >= GOLDEN_HOURS) {
            vehicle->membership = GOLD;
        } else if (vehicle->total_parking_hours >= PREMIUM_HOURS) {
            vehicle->membership = PREMIUM;
        }

        printf("\nVehicle Exit Summary:\n");
        printf("Hours parked: %d\n", parked_hours);
        printf("Parking fee: %d Rs\n", fee);
        printf("Total parking hours: %d\n", vehicle->total_parking_hours);
        printf("Membership status: %s\n", 
               vehicle->membership == GOLD ? "GOLD" : 
               vehicle->membership == PREMIUM ? "PREMIUM" : "NONE");
    } else {
        printf("Vehicle not found in the system.\n");
    }
}

// Function to display all vehicles in the B+ tree
// Function to arrange vehicles based on the number of parkings done
void arrangeVehiclesByParkingCount(BPTreeNode* root) {
    if (root == NULL) return;

    Vehicle* vehicles[vehicle_count];
    int index = 0;

    // Collect all vehicles
    BPTreeNode* current = root;
    while (!current->isLeaf) {
        current = current->children[0];
    }

    while (current != NULL) {
        for (int i = 0; i < current->numKeys; i++) {
            vehicles[index++] = current->vehicles[i];
        }
        current = current->next;
    }

    // Sort vehicles by parking count
    for (int i = 0; i < vehicle_count - 1; i++) {
        for (int j = 0; j < vehicle_count - i - 1; j++) {
            if (vehicles[j]->parking_count < vehicles[j + 1]->parking_count) {
                Vehicle* temp = vehicles[j];
                vehicles[j] = vehicles[j + 1];
                vehicles[j + 1] = temp;
            }
        }
    }

    // Print sorted vehicles
    printf("\nVehicles Sorted by Parking Count:\n");
    printf("---------------------------------------------------\n");
    printf("| %-10s | %-15s | %-10s |\n", "Vehicle", "Owner", "Parkings");
    printf("---------------------------------------------------\n");
    for (int i = 0; i < vehicle_count; i++) {
        printf("| %-10s | %-15s | %-10d |\n", vehicles[i]->vehicle_num, vehicles[i]->owner_name, vehicles[i]->parking_count);
    }
    printf("---------------------------------------------------\n");
}

// Function to arrange vehicles based on parking amount paid
void arrangeVehiclesByAmountPaid(BPTreeNode* root, int minAmount, int maxAmount) {
    if (root == NULL) return;

    Vehicle* vehicles[vehicle_count];
    int index = 0;

    // Collect all vehicles
    BPTreeNode* current = root;
    while (!current->isLeaf) {
        current = current->children[0];
    }

    while (current != NULL) {
        for (int i = 0; i < current->numKeys; i++) {
            vehicles[index++] = current->vehicles[i];
        }
        current = current->next;
    }

    // Sort vehicles by total amount paid
    for (int i = 0; i < vehicle_count - 1; i++) {
        for (int j = 0; j < vehicle_count - i - 1; j++) {
            if (vehicles[j]->total_amount_paid < vehicles[j + 1]->total_amount_paid) {
                Vehicle* temp = vehicles[j];
                vehicles[j] = vehicles[j + 1];
                vehicles[j + 1] = temp;
            }
        }
    }

    // Print vehicles within the given range
    printf("\nVehicles Sorted by Amount Paid (Between %d and %d):\n", minAmount, maxAmount);
    printf("---------------------------------------------------\n");
    printf("| %-10s | %-15s | %-10s |\n", "Vehicle", "Owner", "Amount Paid");
    printf("---------------------------------------------------\n");
    for (int i = 0; i < vehicle_count; i++) {
        if (vehicles[i]->total_amount_paid >= minAmount && vehicles[i]->total_amount_paid <= maxAmount) {
            printf("| %-10s | %-15s | %-10d |\n", vehicles[i]->vehicle_num, vehicles[i]->owner_name, vehicles[i]->total_amount_paid);
        }
    }
    printf("---------------------------------------------------\n");
}

// Function to arrange parking spaces based on occupancy
void arrangeParkingSpacesByOccupancy() {
    TempBPTreeNode* tempTree = NULL;

    // Insert all parking spaces into the temporary B+ tree
    ParkingSpaceNode* current = parking_space_tree;
    while (!current->isLeaf) {
        current = current->children[0];
    }

    while (current != NULL) {
        for (int i = 0; i < current->numKeys; i++) {
            insertIntoTempTree(&tempTree, current->spaces[i], current->spaces[i]->occupancy_count);
        }
        current = current->next;
    }

    // Traverse and print the temporary B+ tree
    printf("\nParking Spaces Sorted by Occupancy:\n");
    printf("---------------------------------------------------\n");
    printf("| %-10s | %-10s |\n", "Space ID", "Occupancy");
    printf("---------------------------------------------------\n");

    TempBPTreeNode* tempCurrent = tempTree;
    while (!tempCurrent->isLeaf) {
        tempCurrent = tempCurrent->children[0];
    }

    while (tempCurrent != NULL) {
        for (int i = 0; i < tempCurrent->numKeys; i++) {
            ParkingSpace* space = tempCurrent->spaces[i];
            printf("| %-10d | %-10d |\n", space->parking_space_ID, space->occupancy_count);
        }
        tempCurrent = tempCurrent->next;
    }
    printf("---------------------------------------------------\n");

    // Free the temporary B+ tree (not implemented here for brevity)
}

// Function to arrange parking spaces based on revenue
void arrangeParkingSpacesByRevenue() {
    TempBPTreeNode* tempTree = NULL;

    // Insert all parking spaces into the temporary B+ tree
    ParkingSpaceNode* current = parking_space_tree;
    while (!current->isLeaf) {
        current = current->children[0];
    }

    while (current != NULL) {
        for (int i = 0; i < current->numKeys; i++) {
            insertIntoTempTree(&tempTree, current->spaces[i], current->spaces[i]->space_revenue);
        }
        current = current->next;
    }

    // Traverse and print the temporary B+ tree
    printf("\nParking Spaces Sorted by Revenue:\n");
    printf("---------------------------------------------------\n");
    printf("| %-10s | %-10s |\n", "Space ID", "Revenue");
    printf("---------------------------------------------------\n");

    TempBPTreeNode* tempCurrent = tempTree;
    while (!tempCurrent->isLeaf) {
        tempCurrent = tempCurrent->children[0];
    }

    while (tempCurrent != NULL) {
        for (int i = 0; i < tempCurrent->numKeys; i++) {
            ParkingSpace* space = tempCurrent->spaces[i];
            printf("| %-10d | %-10d |\n", space->parking_space_ID, space->space_revenue);
        }
        tempCurrent = tempCurrent->next;
    }
    printf("---------------------------------------------------\n");

    // Free the temporary B+ tree (not implemented here for brevity)
}

// Function to print leaf nodes visually
void printLeafNodesVisual(BPTreeNode* root) {
    if (root == NULL) return;

    // Traverse to the first leaf
    BPTreeNode* current = root;
    while (!current->isLeaf) {
        current = current->children[0];
    }

    printf("\nVisual Representation of Leaf Nodes in B+ Tree (Forward):\n\n");

    while (current != NULL) {
        printf("+");
        for (int i = 0; i < current->numKeys; i++) {
            printf("------------+");
        }
        printf("\n|");
        for (int i = 0; i < current->numKeys; i++) {
            printf(" %-10s |", current->keys[i]);
        }
        printf("\n+");
        for (int i = 0; i < current->numKeys; i++) {
            printf("------------+");
        }

        if (current->next != NULL) {
            printf(" --> ");
        } else {
            printf(" --> NULL");
        }

        current = current->next;
    }

    printf("\n\nVisual Representation of Leaf Nodes in B+ Tree (Backward):\n\n");

    // Traverse backward using the prev pointer
    while (current != NULL && current->prev != NULL) {
        current = current->prev;
    }

    while (current != NULL) {
        printf("+");
        for (int i = 0; i < current->numKeys; i++) {
            printf("------------+");
        }
        printf("\n|");
        for (int i = 0; i < current->numKeys; i++) {
            printf(" %-10s |", current->keys[i]);
        }
        printf("\n+");
        for (int i = 0; i < current->numKeys; i++) {
            printf("------------+");
        }

        if (current->prev != NULL) {
            printf(" <-- ");
        } else {
            printf(" <-- NULL");
        }

        current = current->prev;
    }
    printf("\n");
}


void printAllVehicles(BPTreeNode* root) {
    if (root == NULL) return;

    printf("\nCurrently Parked Vehicles:\n");
    printLeafNodesVisual(root);

    printf("\nDetailed Info of Parked Vehicles:\n");
    printf("------------------------------------------------------------------------------------\n");
    printf("| %-10s | %-15s | %-10s | %-10s |\n", "Vehicle", "Owner", "Parking ID", "Membership");
    printf("------------------------------------------------------------------------------------\n");

    BPTreeNode* current = root;
    while (!current->isLeaf) {
        current = current->children[0];
    }

    while (current != NULL) {
        for (int i = 0; i < current->numKeys; i++) {
            Vehicle* v = current->vehicles[i];
            // print all vehicles, regardless of parking_ID
            const char* membership =
                v->membership == GOLD ? "GOLD" :
                v->membership == PREMIUM ? "PREMIUM" : "NONE";
            printf("| %-10s | %-15s | %-10d | %-10s |\n",
                   v->vehicle_num, v->owner_name, v->parking_ID, membership);
        }
        current = current->next;
    }
    printf("------------------------------------------------------------------------------------\n");
}

void load_data() {
    FILE* file = fopen("vehicles_text.txt", "r");
    if (!file) {
        printf("No existing data found. Starting fresh.\n");
        return;
    }

    while (!feof(file)) {
        Vehicle* vehicle = (Vehicle*)malloc(sizeof(Vehicle));
        int read = fscanf(file, "%9s %19s %d %d %d %d %d %d %d %d %d",
                   vehicle->vehicle_num,
                   vehicle->owner_name,
                   &vehicle->arrival.time,
                   &vehicle->arrival.date,
                   &vehicle->arrival.month,
                   &vehicle->arrival.year,
                   &vehicle->membership,
                   &vehicle->total_parking_hours,
                   &vehicle->total_amount_paid,
                   &vehicle->parking_ID,
                   &vehicle->parking_count);
        if (read == 11) {
            printf("Loading vehicle: %s, Owner: %s\n", vehicle->vehicle_num, vehicle->owner_name);
            insertVehicle(vehicle);
            vehicle_count++;

            if (vehicle->parking_ID > 0 && vehicle->parking_ID <= MAX_PARKING_SPACES) {
                ParkingSpace* space = searchParkingSpace(parking_space_tree, vehicle->parking_ID);
                if (space != NULL) {
                    space->occupancy_count += vehicle->parking_count;
                    space->space_revenue += vehicle->total_amount_paid;
                }
            }
        } else {
            free(vehicle);
        }
    }

    fclose(file);
    printf("Vehicle data loaded successfully.\n");
}


void save_data() {
    if (vehicle_tree == NULL) return;

    FILE* file = fopen("vehicles_text.txt", "w");
    if (!file) {
        printf("Error: Unable to save data.\n");
        return;
    }

    BPTreeNode* current = vehicle_tree;
    while (!current->isLeaf) {
        current = current->children[0];
    }

    while (current != NULL) {
        for (int i = 0; i < current->numKeys; i++) {
            Vehicle* vehicle = current->vehicles[i];
            fprintf(file, "%s %s %d %d %d %d %d %d %d %d %d\n",
                    vehicle->vehicle_num,
                    vehicle->owner_name,
                    vehicle->arrival.time,
                    vehicle->arrival.date,
                    vehicle->arrival.month,
                    vehicle->arrival.year,
                    vehicle->membership,
                    vehicle->total_parking_hours,
                    vehicle->total_amount_paid,
                    vehicle->parking_ID,
                    vehicle->parking_count);
        }
        current = current->next;
    }

    fclose(file);
    printf("Vehicle data saved successfully.\n");
}

// Forward declarations
void insertInternalParkingSpace(int key, ParkingSpaceNode* parent, ParkingSpaceNode* child);

ParkingSpaceNode* findParentParkingSpace(ParkingSpaceNode* current, ParkingSpaceNode* child) {
    if (current == NULL || current->isLeaf) return NULL;

    for (int i = 0; i <= current->numKeys; i++) {
        if (current->children[i] == child) return current;
        ParkingSpaceNode* p = findParentParkingSpace(current->children[i], child);
        if (p != NULL) return p;
    }
    return NULL;
}

void insertParkingSpace(ParkingSpace* space) {
    if (parking_space_tree == NULL) {
        parking_space_tree = createParkingSpaceNode(true);
        parking_space_tree->keys[0] = space->parking_space_ID;
        parking_space_tree->spaces[0] = space;
        parking_space_tree->numKeys = 1;
        return;
    }

    ParkingSpaceNode* current = parking_space_tree;
    ParkingSpaceNode* parent = NULL;

    while (!current->isLeaf) {
        parent = current;
        int i = 0;
        while (i < current->numKeys && space->parking_space_ID > current->keys[i]) i++;
        current = current->children[i];
    }

    if (current->numKeys < MAX_KEYS) {
        int i = current->numKeys - 1;
        while (i >= 0 && space->parking_space_ID < current->keys[i]) {
            current->keys[i + 1] = current->keys[i];
            current->spaces[i + 1] = current->spaces[i];
            i--;
        }
        current->keys[i + 1] = space->parking_space_ID;
        current->spaces[i + 1] = space;
        current->numKeys++;
    } else {
        int tempKeys[MAX_KEYS + 1];
        ParkingSpace* tempSpaces[MAX_KEYS + 1];

        for (int i = 0; i < MAX_KEYS; i++) {
            tempKeys[i] = current->keys[i];
            tempSpaces[i] = current->spaces[i];
        }

        int i = MAX_KEYS - 1;
        while (i >= 0 && space->parking_space_ID < tempKeys[i]) {
            tempKeys[i + 1] = tempKeys[i];
            tempSpaces[i + 1] = tempSpaces[i];
            i--;
        }
        tempKeys[i + 1] = space->parking_space_ID;
        tempSpaces[i + 1] = space;

        ParkingSpaceNode* newLeaf = createParkingSpaceNode(true);
        current->numKeys = MIN_KEYS;
        newLeaf->numKeys = MAX_KEYS + 1 - current->numKeys;

        for (i = 0; i < current->numKeys; i++) {
            current->keys[i] = tempKeys[i];
            current->spaces[i] = tempSpaces[i];
        }
        for (i = 0; i < newLeaf->numKeys; i++) {
            newLeaf->keys[i] = tempKeys[current->numKeys + i];
            newLeaf->spaces[i] = tempSpaces[current->numKeys + i];
        }

        newLeaf->next = current->next;
        if (current->next != NULL) {
            current->next->prev = newLeaf;
        }
        current->next = newLeaf;
        newLeaf->prev = current;

        if (current == parking_space_tree) {
            ParkingSpaceNode* newRoot = createParkingSpaceNode(false);
            newRoot->keys[0] = newLeaf->keys[0];
            newRoot->children[0] = current;
            newRoot->children[1] = newLeaf;
            newRoot->numKeys = 1;
            parking_space_tree = newRoot;
        } else {
            insertInternalParkingSpace(newLeaf->keys[0], findParentParkingSpace(parking_space_tree, current), newLeaf);
        }
    }
}

void insertInternalParkingSpace(int key, ParkingSpaceNode* parent, ParkingSpaceNode* child) {
    if (parent->numKeys < MAX_KEYS) {
        int i = parent->numKeys - 1;
        while (i >= 0 && key < parent->keys[i]) {
            parent->keys[i + 1] = parent->keys[i];
            parent->children[i + 2] = parent->children[i + 1];
            i--;
        }
        parent->keys[i + 1] = key;
        parent->children[i + 2] = child;
        parent->numKeys++;
    } else {
        int tempKeys[MAX_KEYS + 1];
        ParkingSpaceNode* tempChildren[MAX_KEYS + 2];

        for (int i = 0; i < MAX_KEYS; i++) {
            tempKeys[i] = parent->keys[i];
        }
        for (int i = 0; i < MAX_KEYS + 1; i++) {
            tempChildren[i] = parent->children[i];
        }

        int i = MAX_KEYS - 1;
        while (i >= 0 && key < tempKeys[i]) {
            tempKeys[i + 1] = tempKeys[i];
            tempChildren[i + 2] = tempChildren[i + 1];
            i--;
        }
        tempKeys[i + 1] = key;
        tempChildren[i + 2] = child;

        ParkingSpaceNode* newInternal = createParkingSpaceNode(false);
        parent->numKeys = MIN_KEYS;
        newInternal->numKeys = MAX_KEYS - MIN_KEYS;

        for (i = 0; i < parent->numKeys; i++) {
            parent->keys[i] = tempKeys[i];
            parent->children[i] = tempChildren[i];
        }
        parent->children[i] = tempChildren[i];

        for (int j = 0; j < newInternal->numKeys; j++) {
            newInternal->keys[j] = tempKeys[i + 1 + j];
            newInternal->children[j] = tempChildren[i + 1 + j];
        }
        newInternal->children[newInternal->numKeys] = tempChildren[MAX_KEYS + 1];

        if (parent == parking_space_tree) {
            ParkingSpaceNode* newRoot = createParkingSpaceNode(false);
            newRoot->keys[0] = tempKeys[MIN_KEYS];
            newRoot->children[0] = parent;
            newRoot->children[1] = newInternal;
            newRoot->numKeys = 1;
            parking_space_tree = newRoot;
        } else {
            insertInternalParkingSpace(tempKeys[MIN_KEYS], findParentParkingSpace(parking_space_tree, parent), newInternal);
        }
    }
}

void displayParkingSpaces(ParkingSpaceNode* root) {
    if (root == NULL) return;

    ParkingSpaceNode* current = root;
    while (!current->isLeaf) {
        current = current->children[0];
    }

    printf("\nParking Spaces:\n");
    printf("---------------------------------------------------\n");
    printf("| %-10s | %-10s | %-10s | %-10s |\n", "Space ID", "Status", "Occupancy", "Revenue");
    printf("---------------------------------------------------\n");

    while (current != NULL) {
        for (int i = 0; i < current->numKeys; i++) {
            ParkingSpace* space = current->spaces[i];
            printf("| %-10d | %-10s | %-10d | %-10d |\n",
                   space->parking_space_ID,
                   space->status == FREE ? "FREE" : "OCCUPIED",
                   space->occupancy_count,
                   space->space_revenue);
        }
        current = current->next;
    }
    printf("---------------------------------------------------\n");
}

void insertIntoTempTree(TempBPTreeNode** root, ParkingSpace* space, int key) {
    if (*root == NULL) {
        *root = createTempNode(true);
        (*root)->keys[0] = key;
        (*root)->spaces[0] = space;
        (*root)->numKeys = 1;
        return;
    }

    TempBPTreeNode* current = *root;
    TempBPTreeNode* parent = NULL;

    while (!current->isLeaf) {
        parent = current;
        int i = 0;
        while (i < current->numKeys && key > current->keys[i]) i++;
        current = current->children[i];
    }

    if (current->numKeys < MAX_KEYS) {
        int i = current->numKeys - 1;
        while (i >= 0 && key < current->keys[i]) {
            current->keys[i + 1] = current->keys[i];
            current->spaces[i + 1] = current->spaces[i];
            i--;
        }
        current->keys[i + 1] = key;
        current->spaces[i + 1] = space;
        current->numKeys++;
    } else {
        // Handle node splitting (same as before, just use 'key')
        int tempKeys[MAX_KEYS + 1];
        ParkingSpace* tempSpaces[MAX_KEYS + 1];

        for (int i = 0; i < MAX_KEYS; i++) {
            tempKeys[i] = current->keys[i];
            tempSpaces[i] = current->spaces[i];
        }

        int i = MAX_KEYS - 1;
        while (i >= 0 && key < tempKeys[i]) {
            tempKeys[i + 1] = tempKeys[i];
            tempSpaces[i + 1] = tempSpaces[i];
            i--;
        }
        tempKeys[i + 1] = key;
        tempSpaces[i + 1] = space;

        TempBPTreeNode* newLeaf = createTempNode(true);
        current->numKeys = MIN_KEYS;
        newLeaf->numKeys = MAX_KEYS + 1 - MIN_KEYS;

        for (i = 0; i < current->numKeys; i++) {
            current->keys[i] = tempKeys[i];
            current->spaces[i] = tempSpaces[i];
        }
        for (i = 0; i < newLeaf->numKeys; i++) {
            newLeaf->keys[i] = tempKeys[current->numKeys + i];
            newLeaf->spaces[i] = tempSpaces[current->numKeys + i];
        }

        newLeaf->next = current->next;
        if (current->next != NULL) {
            current->next->prev = newLeaf;
        }
        current->next = newLeaf;
        newLeaf->prev = current;

        if (current == *root) {
            TempBPTreeNode* newRoot = createTempNode(false);
            newRoot->keys[0] = newLeaf->keys[0];
            newRoot->children[0] = current;
            newRoot->children[1] = newLeaf;
            newRoot->numKeys = 1;
            *root = newRoot;
        } else {
            // Insert into parent node (not implemented for brevity)
        }
    }
}

void traverseTempTree(TempBPTreeNode* root) {
    if (root == NULL) return;

    TempBPTreeNode* current = root;
    while (!current->isLeaf) {
        current = current->children[0];
    }

    printf("\nParking Spaces Sorted by Revenue:\n");
    printf("---------------------------------------------------\n");
    printf("| %-10s | %-10s |\n", "Space ID", "Revenue");
    printf("---------------------------------------------------\n");

    while (current != NULL) {
        for (int i = 0; i < current->numKeys; i++) {
            ParkingSpace* space = current->spaces[i];
            printf("| %-10d | %-10d |\n", space->parking_space_ID, space->space_revenue);
        }
        current = current->next;
    }
    printf("---------------------------------------------------\n");
}

// Remove or comment out insertVehicleIntoTempTree() and traverseVehicleTempTree() if not used,
// or refactor to avoid unsafe casting. (No change if not used in menu.)

// Main function
int main() {
    initialize_parking_spaces();
    load_data(); // Load data from file at the start

     // Print the number of registered vehicles
     printf("\nTotal Registered Vehicles: %d\n", vehicle_count);


    int choice;

    printf("Welcome to Smart Parking System (B+ Tree Implementation)\n");

    do {
        printf("\n=== Smart Parking System Menu ===\n");
        printf("1. Park Vehicle\n");
        printf("2. Exit Vehicle\n");
        printf("3. Display All Parked Vehicles\n");
        printf("4. Arrange Vehicles by Parking Count\n");
        printf("5. Arrange Vehicles by Amount Paid (Within Range)\n");
        printf("6. Arrange Parking Spaces by Occupancy\n");
        printf("7. Arrange Parking Spaces by Revenue\n");
        printf("8. Display Parking Spaces\n");
        printf("0. Exit System\n");
        printf("===============================\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                park_vehicle();
                break;
            case 2:
                exit_vehicle();
                break;
            case 3:
                printAllVehicles(vehicle_tree);
                break;
            case 4:
                arrangeVehiclesByParkingCount(vehicle_tree);
                break;
            case 5: {
                int minAmount, maxAmount;
                printf("Enter minimum parking amount: ");
                scanf("%d", &minAmount);
                printf("Enter maximum parking amount: ");
                scanf("%d", &maxAmount);
                arrangeVehiclesByAmountPaid(vehicle_tree, minAmount, maxAmount);
                break;
            }
            case 6:
                arrangeParkingSpacesByOccupancy();
                break;
            case 7:
                arrangeParkingSpacesByRevenue();
                break;
            case 8:
                displayParkingSpaces(parking_space_tree);
                break;
            case 0:
                save_data(); // Save data to file before exiting
                printf("\nThank you for using Smart Parking System!\n");
                break;
            default:
                printf("\nInvalid choice. Please try again.\n");
        }
    } while (choice != 0);

    return 0;
}