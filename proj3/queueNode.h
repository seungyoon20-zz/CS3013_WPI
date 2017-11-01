struct msg;

struct queueNode{
    struct queueNode* prev; //points to the previous node
    struct queueNode* next; //points to the next node
    int recipient; //record the receipient of this msg
    struct msg* msg; //points to the undelivered msg

};
