#include "circlearray.h"
#include <stdlib.h>


int main() {

    int index = 1;
    for (int i = 0; i < 2000; i+=2) {
        int op = rand() % 5;
        if (op == 0 || op == 2){
            pod* rcv = ca_remove();
            if (rcv != NULL) {
                printf("Remove %d \n",rcv->packet_index );
                free(rcv);
            }
        }
        else if (op == 1)
            printf("add %d (%d)\n",i+1,ca_insert(1,i+1,100));
        else{
            printf("add %d (%d)\n",index, ca_insert(1,index,100));
            index++;
        } 
    }
    int missing;
    int* ms = ca_get_missing(&missing);
    printf("Number missing %d \n",missing);
}
