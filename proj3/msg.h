struct msg{
    int iFrom; // the # of thread that sent this msg
    int value;
    int cnt; //count of operations, not neccessary for all messages
    int tot; //the total time used, not neccessary for all messages
};

