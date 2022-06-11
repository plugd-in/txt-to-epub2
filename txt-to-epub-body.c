#include <stdio.h>
#include <string.h>
#include <aio.h>
#include <stdlib.h>
#include <glib.h>

typedef struct _Node {
    void * prev;
    void * next;
    void * data;
} Node;

typedef struct _List {
    Node * head;
    Node * tail;
} List;

void append_list (List * list, void * data) {
    Node * next = (Node*) malloc(sizeof(Node));
    next->data = data;
    if ( list->head == NULL ) {
        list->head = next;
        next->next = NULL;
        next->prev = NULL;
    } else if ( list->tail == NULL ) {
        list->tail = next;
        next->prev = list->head;
        list->head->next = next;
        next->next = NULL;
    } else {
        next->prev = list->tail;
        next->next = NULL;
        list->tail->next = next;
        list->tail = next;
    }
}

int main (int argc, char** args) {
    char * inputFilePath;
    FILE * inputFile = NULL;
    FILE * outputFile = NULL;
    if ( argc < 2 ) {
        printf("No file argument.\n");
        return 1;
    }
    inputFilePath = args[1];
    if ( strcmp(inputFilePath, "-") == 0 ) inputFile = stdin;
    else inputFile = fopen(inputFilePath, "r");
    int c;
    List list;
    list.head = NULL;
    list.tail = NULL;
    GString * line = g_string_new("<p>");
    
    while ( (c = fgetc(inputFile)) != EOF ) {
        if ( c != '\n') g_string_append_c(line, c);
        else if ( line->len > 3 ) { // Ignore subsequent lines.
            g_string_append(line, "</p>");
            append_list(&list, line);
            line = g_string_new("<p>");
            
        }
        
    }
    if ( line->len > 3) {
        g_string_append(line, "</p>");
        append_list(&list, line);
    }
    for ( Node * current = list.head; current != NULL; current = current->next ) {
        printf("%s\n", ((GString*) current->data)->str );
    }
    return 0;
}
