#include <stdio.h>
#include <string.h>
#include <aio.h>
#include <stdlib.h>
#include <glib.h>
#include <argp.h>

const char * argp_program_version = "txt-to-epub 0.1.0";

static char doc[] = "txt-to-epub - Transform .txt document(s) to an epub document.";
static char args_doc[] = "[INPUT FILE...]";

static struct argp_option options[] = {
    {"output", 'o', "FILE", 0, "Output to FILE instead of Standard Output."},
    {"title", 't', "TITLE", 0, "Title of the document."},
    {"delim", 'd', "delimiter", 0, "Delimiter of the input file. Splits input files into subsequent sections. By default, the entire input file is consumed as a section."},
    { 0 }
};

typedef struct Node {
    void * prev;
    void * next;
    void * data;
} Node;

typedef struct List {
    Node * head;
    Node * tail;
    int len;
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
    list->len += 1;
}



struct arguments {
    List * inputFiles;
    FILE * outputFile;
    char * title;
    char * delimiter;
};

static error_t parse_opt (int key, char *arg, struct argp_state * state) {
    struct arguments * arguments = state->input;

    switch (key) {
        case 'o':
            arguments->outputFile = fopen(arg, "w+");
            break;
        case 't':
            arguments->title = arg;
            break;
        case 'd':
            arguments->delimiter = arg;
            break;
        case ARGP_KEY_ARG:
            append_list(arguments->inputFiles, fopen(arg, "r"));
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc};


typedef struct Section {
    char * title;
    List * bodyElements; // List<Gstring *>;
} Section;



int main (int argc, char** argv) {
    // Arguments
    struct arguments arguments;
    // List of input files.
    List inputList;
    inputList.head = NULL;
    inputList.tail = NULL;
    inputList.len = 0;
    arguments.inputFiles = &inputList;
    // Default to STDOUT
    arguments.outputFile = stdout;
    arguments.title = "EPub Title";
    arguments.delimiter = NULL; // By default, consume the entire input as a section.


    // Argument Parsing
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if ( inputList.len == 0 ) {
        append_list(&inputList, stdin);
    }
    // End Argument Parsing

    // Program Construction 
    List sectionList; // List<Section*>;
    sectionList.head = NULL;
    sectionList.tail = NULL;
    sectionList.len = 0;



    
    // Program cleanup
    // Close file streams.
    for ( Node * current = inputList.head; current != NULL; current = current->next) {
        FILE * file = current->data;
        fclose(file);
    }

    // Close output stream.
    if ( arguments.outputFile != stdout ) fclose(arguments.outputFile);

    printf("Title: %s\n", arguments.title);
    printf("Delimiter: %s\n", arguments.delimiter == NULL ? "No Delimiter" : arguments.delimiter);

    return 0;
}
