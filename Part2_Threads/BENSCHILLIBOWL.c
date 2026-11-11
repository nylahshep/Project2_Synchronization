#include "BENSCHILLIBOWL.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>

bool IsEmpty(BENSCHILLIBOWL* bcb);
bool IsFull(BENSCHILLIBOWL* bcb);
void AddOrderToBack(Order **orders, Order *order);

MenuItem BENSCHILLIBOWLMenu[] = { 
    "BensChilli", 
    "BensHalfSmoke", 
    "BensHotDog", 
    "BensChilliCheeseFries", 
    "BensShake",
    "BensHotCakes",
    "BensCake",
    "BensHamburger",
    "BensVeggieBurger",
    "BensOnionRings",
};
int BENSCHILLIBOWLMenuLength = 10;

/* Select a random item from the Menu and return it */
MenuItem PickRandomMenuItem() {
    int index = rand() % BENSCHILLIBOWLMenuLength;
    return BENSCHILLIBOWLMenu[index];
}

/* Open the restaurant */
BENSCHILLIBOWL* OpenRestaurant(int max_size, int expected_num_orders) {
    BENSCHILLIBOWL *bcb = malloc(sizeof(BENSCHILLIBOWL));

    if (bcb == NULL) {
        return NULL;
    }

    bcb->orders = NULL;
    bcb->current_size = 0;
    bcb->max_size = max_size;
    bcb->next_order_number = 1;
    bcb->orders_handled = 0;
    bcb->expected_num_orders = expected_num_orders;

    pthread_mutex_init(&bcb->mutex, NULL);
    pthread_cond_init(&bcb->can_add_orders, NULL);
    pthread_cond_init(&bcb->can_get_orders, NULL);

    srand(time(NULL));

    printf("Restaurant is open!\n");
    return bcb;
}

/* Close the restaurant */
void CloseRestaurant(BENSCHILLIBOWL* bcb) {
    assert(bcb->current_size == 0);
    assert(bcb->orders == NULL);
    assert(bcb->orders_handled == bcb->expected_num_orders);

    pthread_mutex_destroy(&bcb->mutex);
    pthread_cond_destroy(&bcb->can_add_orders);
    pthread_cond_destroy(&bcb->can_get_orders);

    free(bcb);

    printf("Restaurant is closed!\n");
}

/* Add an order to the restaurant */
int AddOrder(BENSCHILLIBOWL* bcb, Order* order) {
    pthread_mutex_lock(&bcb->mutex);

    while (IsFull(bcb)) {
        pthread_cond_wait(&bcb->can_add_orders, &bcb->mutex);
    }

    order->order_number = bcb->next_order_number;
    bcb->next_order_number++;

    AddOrderToBack(&bcb->orders, order);
    bcb->current_size++;

    int order_number = order->order_number;

    pthread_cond_signal(&bcb->can_get_orders);
    pthread_mutex_unlock(&bcb->mutex);

    return order_number;
}

/* Get an order from the restaurant */
Order *GetOrder(BENSCHILLIBOWL* bcb) {
    pthread_mutex_lock(&bcb->mutex);

    while (IsEmpty(bcb) &&
           bcb->orders_handled < bcb->expected_num_orders) {
        pthread_cond_wait(&bcb->can_get_orders, &bcb->mutex);
    }

    if (IsEmpty(bcb) &&
        bcb->orders_handled == bcb->expected_num_orders) {
        pthread_cond_broadcast(&bcb->can_get_orders);
        pthread_mutex_unlock(&bcb->mutex);
        return NULL;
    }

    Order *order = bcb->orders;
    bcb->orders = order->next;
    order->next = NULL;

    bcb->current_size--;
    bcb->orders_handled++;

    pthread_cond_signal(&bcb->can_add_orders);

    if (bcb->orders_handled == bcb->expected_num_orders) {
        pthread_cond_broadcast(&bcb->can_get_orders);
    }

    pthread_mutex_unlock(&bcb->mutex);

    return order;
}

/* Check if restaurant is empty */
bool IsEmpty(BENSCHILLIBOWL* bcb) {
    return bcb->current_size == 0;
}

/* Check if restaurant is full */
bool IsFull(BENSCHILLIBOWL* bcb) {
    return bcb->current_size >= bcb->max_size;
}

/* Add order to the back of the queue */
void AddOrderToBack(Order **orders, Order *order) {
    order->next = NULL;

    if (*orders == NULL) {
        *orders = order;
        return;
    }

    Order *current = *orders;

    while (current->next != NULL) {
        current = current->next;
    }

    current->next = order;
}