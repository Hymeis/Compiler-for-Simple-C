struct node {
    int data;
    struct node *link;
};

struct list {
    int count;
    struct node *head;
};

struct list stack;

char *malloc(unsigned long n);

int push(struct list *list, int x)
{
    struct node *node;

    node = (struct node *) malloc(sizeof(struct node));
    node->link = (struct node *) 0;
    (*list).head = node;
    list->head->data = -- x;

    return 0;
}

int main()
{
    push(&stack, 0);
}
