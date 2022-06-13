#include <stdio.h>
#include <string.h>
#include <aio.h>
#include <stdlib.h>
#include <glib.h>
#include <argp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <uuid/uuid.h>
#include <stdbool.h>
#include <zip.h>
#include <libgen.h>

#define __USE_XOPEN_EXTENDED 1
#include <ftw.h>

struct stat st = {0};

const char * argp_program_version = "txt-to-epub 0.2.0";

static char doc[] = "txt-to-epub - Transform .txt document(s) to an epub document.";
static char args_doc[] = "[INPUT FILE...]";

static struct argp_option options[] = {
    {"output", 'o', "FILE", 0, "Output to FILE. Defaults to ./<Document Title>.epub"},
    {"title", 't', "TITLE", 0, "Title of the document. Defaults to \"EPUB Title\""},
    {"keep", 'k', 0, 0, "Keep the construction directory."},
    {"verbose", 'v', 0, 0, "Include verbose output.."},
    {"delim", 'd', "delimiter", 0, "Delimiter of the input file. Splits input files into subsequent sections. By default, the entire input file is consumed as a section. Note, the delimiter should be the only line contents."},
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

struct SectionTitlePair {
    char * sectionPath;
    char * sectionTitle;
};

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

void prepend_list (List * list, void * data) {
    Node * next = (Node*) malloc(sizeof(Node));
    next->data = data;
    if ( list->head == NULL ) {
        list->head = next;
        next->next = NULL;
        next->prev = NULL;
    } else if ( list->tail == NULL ) {
        list->tail = list->head;
        list->head = next;
        next->next = list->tail;
        list->tail->prev = next;
        next->prev = NULL;
    } else {
        next->next = list->head;
        next->prev = NULL;
        list->head->prev = next;
        list->head = next;
    }
    list->len += 1;
}

void free_list_node (List * list, Node * node, bool freeData) {
    if ( node == NULL || list == NULL ) return;
    if ( freeData ) free(node->data);
    // Reorganize list.
    if ( list->head == node ) { // Outer node (head)
        if ( node->next == list->tail && list->tail != NULL ) { // List: | node - head | node->next - tail |
            list->head = list->tail;
            list->tail = NULL;
        } else { // List: | node - head | OR | node - head | node->next | ... | tail |
            list->head = node->next;
            if ( list->head != NULL ) list->head->prev = NULL;
        }
    } else if ( list->tail == node ) { // Outer node (tail)
        if ( node->prev == list->head ) { // List: | head | node - tail |
            list->head->next = NULL;
            list->tail = NULL;
        } else { // List: | head | ... | node - tail |
            list->tail = list->tail->prev;
            list->tail->next = NULL;
        }
    } else { // Inner node: | head | ... | node | ... | tail |
        // I'm unlinking the node by unreferencing the node from the neighbors and having the neighbors reference each other.
        ( (Node*) node->prev )->next = node->next;
        ( (Node*) node->next )->prev = node->prev;
    }
    free(node);
}

int handle_remove (const char* path, const struct stat * sb, int flag, struct FTW * ftwbuf) {
    int ret = remove(path);
    if ( ret ) fprintf(stderr, "%s", path);
    return ret;
}

int rmrf (char * path) { // Recursive remove.
    return nftw(path, handle_remove, 10, FTW_DEPTH);
}

struct arguments {
    List * inputFiles;
    char * outputFile;
    char * title;
    char * delimiter;
    bool keep;
    bool verbose;
};

static error_t parse_opt (int key, char *arg, struct argp_state * state) {
    struct arguments * arguments = state->input;

    switch (key) {
        case 'o':
            arguments->outputFile = arg;
            break;
        case 't':
            arguments->title = arg;
            break;
        case 'd':
            arguments->delimiter = arg;
            break;
        case 'k':
            arguments->keep = TRUE;
            break;
        case 'v':
            arguments->verbose = TRUE;
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

// Assume that the first line in a section is the section title.
void parse_input (FILE * input, List * sectionList, char * delim) {
    Section * section = malloc(sizeof(Section));
    section->title = "Default Title";
    section->bodyElements = malloc(sizeof(List));
    section->bodyElements->head = NULL;
    section->bodyElements->tail = NULL;
    section->bodyElements->len = 0;
    // First line is styled as a header.
    GString * line = g_string_new("");
    int emptyLine = 1;
    int c;
    while ( (c = fgetc(input)) != EOF ) {
        if ( c != '\n') {
            if ( c == '<' )
                g_string_append(line, "&lt;");
            else if (c == '>' )
                g_string_append(line, "&gt;");
            else if ( c == '&' ) 
                g_string_append(line, "&amp;");
            else g_string_append_c(line, c);
            emptyLine = 0;
        } else if ( !emptyLine ) { // Ignore multiple new-lines.
            // Handle the delimiter line.
            if ( delim != NULL && strcmp(delim, line->str) == 0) {
                append_list(sectionList, section);
                g_string_free(line, 1); // We don't need to save the delimiter line.
                section = malloc(sizeof(Section));
                section->title = "Default Title";
                section->bodyElements = malloc(sizeof(List));
                section->bodyElements->head = NULL;
                section->bodyElements->tail = NULL;
                section->bodyElements->len = 0;

                line = g_string_new("");
                emptyLine = 1;
                continue;
            }
            if ( section->bodyElements->len == 0 ) { // This indicates the first ("title") line.
                section->title = strcpy(malloc(sizeof(char)*line->len), line->str);
                g_string_prepend(line, "\t<h2>");
                g_string_append(line, "</h2>\n");
                append_list(section->bodyElements, line);
                line = g_string_new("");
                emptyLine = 1;
            } else {
                g_string_prepend(line, "\t<p>");
                g_string_append(line, "</p>\n");
                append_list(section->bodyElements, line);
                line = g_string_new("");
                emptyLine = 1;
            }

        }
    }
    if ( section->bodyElements->len > 0 ) append_list(sectionList, section);
    else if ( line->len > 0 ) {
        // IDK why someone would want just a title/header line in a section... but whatever.
        section->title = line->str;
        g_string_prepend(line, "\t<h2>");
        g_string_append(line, "</h2>\n");
        append_list(section->bodyElements, line);
        append_list(sectionList, section);
    } else g_string_free(line, 1);
}

static zip_t * zipFile;
static int rootPath;

int handle_zip_file (const char* path, const struct stat * sb, int flag, struct FTW * ftwbuf) {
    zip_source_t * sourceFile;
    if ( flag == FTW_D ) {
        zip_dir_add(zipFile, path + rootPath, ZIP_FL_ENC_UTF_8);
    } else if ( flag == FTW_F ) {
        sourceFile = zip_source_file(zipFile, path, 0, 0);
        zip_add(zipFile, path+rootPath, sourceFile);
    }
    return 0;
}

/*
Zip the given path (folder or file) into the given output file.
*/
void create_zip (char * pathName, char * outputFile) {
    zip_source_t * sourceFile;
    char path[256];
    rootPath = strlen(realpath(pathName, path));
    struct stat statbuf;
    int err;
    stat(pathName, &statbuf);
    if ( S_ISDIR(statbuf.st_mode) ) {
        zipFile = zip_open(outputFile, ZIP_CREATE | ZIP_TRUNCATE, &err);
        nftw(pathName, handle_zip_file, 10, FTW_DEPTH);
    } else {
        zipFile = zip_open(outputFile, ZIP_CREATE | ZIP_TRUNCATE, &err);
        sourceFile = zip_source_file(zipFile, pathName, 0, 0);
        zip_add(zipFile, basename(pathName), sourceFile );
    }
    zip_close(zipFile);
}

int main (int argc, char** argv) {
    // Arguments
    struct arguments arguments;

    bool outputFileProvided = FALSE;
    // List of input files.
    List inputList;
    inputList.head = NULL;
    inputList.tail = NULL;
    inputList.len = 0;
    arguments.inputFiles = &inputList;
    // Default to STDOUT
    arguments.title = "EPUB Title";
    arguments.delimiter = NULL; // By default, consume the entire input as a section.
    arguments.keep = FALSE;
    arguments.verbose = FALSE;
    arguments.outputFile = NULL;

    // Argument Parsing
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if ( arguments.outputFile == NULL ) {
        char * outputFile = malloc(sizeof(char)*(strlen(arguments.title)+6));
        sprintf(outputFile, "%s.epub", arguments.title);
        arguments.outputFile = outputFile;
    } else outputFileProvided = TRUE;

    if ( inputList.len == 0 ) {
        append_list(&inputList, stdin);
    }
    // End Argument Parsing

    // Program Construction 
    List sectionList; // List<Section*>;
    sectionList.head = NULL;
    sectionList.tail = NULL;
    sectionList.len = 0;

    // Parse all the input files.
    for ( Node * current = inputList.head; current != NULL; current = current->next ) {
        parse_input(current->data, &sectionList, arguments.delimiter);
    }

    uuid_t raw_uuid;
    uuid_generate_random(raw_uuid);

    // 36 characters for uuid, 5 for "/tmp/", and 1 for '\0'.
    char * uuid = malloc(sizeof(char)*(37 + 5));
    char * rawUUID = uuid + 5;
    char * path = malloc(sizeof(char)*(37+5+30));
    strcpy(uuid, "/tmp/");
    uuid_unparse(raw_uuid, rawUUID); // Note rawUUID = uuid + 5
    sprintf(path, "%s/OEBPS", uuid);


    // Create the tmp directory.
    if ( stat(uuid, &st) == -1 ) {
        // Make /tmp/uuid
        mkdir(uuid, 0700);
        // Make /tmp/uuid/OEBPS
        mkdir(path, 0700);
        sprintf(path, "%s/OEBPS/Text", uuid);
        // Make /tmp/uuid/OEBPS/Text
        mkdir(path, 0700);
        sprintf(path, "%s/OEBPS/Styles", uuid);
        // Make /tmp/uuid/OEBPS/Styles
        mkdir(path, 0700);
    } else {
        fprintf(stderr, "Error creating the EPub Document Directory");
        return 1;
    }
    
    List filenameList; // List<struct SectionTitlePair*>
    filenameList.head = NULL;
    filenameList.tail = NULL;
    filenameList.len = 0;

    int idx = 1;
    // Write the section/chapter files.
    for ( Node * current = sectionList.head ; current != NULL; current = current->next) {
        GString * str = g_string_new("");
        Section * section = current->data;
        sprintf(path, "%s/OEBPS/Text/Section%d.xhtml", uuid, idx);

        // Make sure to keep the section path relative to OEBPS
        struct SectionTitlePair * sectionPair = malloc(sizeof(struct SectionTitlePair));
        char * sectionPath = malloc(sizeof(char)*(strlen(path+strlen(uuid))+8)); // Allocate enough past uuid/OEPBS
        sectionPair->sectionPath = sectionPath;
        sectionPair->sectionTitle = section->title; // !!!! Be careful of FREE ORDER !!!!
        strcpy(sectionPath, path+strlen(uuid)+7);
        append_list(&filenameList, sectionPair);

        FILE * newFile = fopen(path, "w+");
        for (Node * bNode = section->bodyElements->head; bNode != NULL; bNode = bNode->next) {
            g_string_append(str, ((GString*)bNode->data)->str);
        }
        fprintf(newFile, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n"
        "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
        "<head>\n"
        "  <title>%s</title>\n"
        "</head>\n"
        "<body>\n"
        "%s"
        "</body>\n"
        "</html>\n", section->title, str->str);
        // Verbose output: Chapter Title
        if ( arguments.verbose ) printf("Added Chapter: %s\n", section->title);
        g_string_free(str, 1);
        fclose(newFile);
        idx++;
    }

    // Write TOC
    sprintf(path, "%s/OEBPS/toc.ncx", uuid);
    FILE * toc = fopen(path, "w+");
    fprintf(toc, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<!DOCTYPE ncx PUBLIC \"-//NISO//DTD ncx 2005-1//EN\" \"http://www.daisy.org/z3986/2005/ncx-2005-1.dtd\">\n"
        "<ncx xmlns=\"http://www.daisy.org/z3986/2005/ncx/\" version=\"2005-1\">\n"
        "  <head>\n"
        "    <meta name=\"dtb:uid\" content=\"urn:uuid:%s\" />\n"
        "    <meta name=\"dtb:depth\" content=\"0\" />\n"
        "    <meta name=\"dtb:totalPageCount\" content=\"0\" />\n"
        "    <meta name=\"dtb:maxPageNumber\" content=\"0\" />\n"
        "  </head>\n"
        "<docTitle>\n"
        "  <text>%s</text>\n"
        "</docTitle>\n"
        "<navMap>\n", rawUUID, arguments.title);
    int i = 1;
    for ( Node * current = filenameList.head; current != NULL; current = current->next ) {
        struct SectionTitlePair * spair = current->data;
        fprintf(toc, "<navPoint id=\"navPoint-%d\" playOrder=\"%d\">\n"
        "  <navLabel>\n"
        "    <text>%s</text>\n"
        "  </navLabel>\n"
        "  <content src=\"%s\" />\n"
        "</navPoint>\n", i, i, spair->sectionTitle, spair->sectionPath);
        i++;
    }
    fprintf(toc, "</navMap>\n</ncx>\n");

    // Program cleanup
    // Close file streams.
    for ( Node * current = inputList.head; current != NULL; current = current->next) {
        FILE * file = current->data;
        fclose(file);
    }


    // Free the sections.
    while (sectionList.head != NULL) {
        while ( ((Section*)sectionList.head->data)->bodyElements->head != NULL ) {
            g_string_free(((Section*)sectionList.head->data)->bodyElements->head->data, 1);
            free_list_node(
                ((Section*)sectionList.head->data)->bodyElements,
                ((Section*)sectionList.head->data)->bodyElements->head,
                0
            );
        }
        free(((Section*)sectionList.head->data)->bodyElements);
        // This check is just to be safe. It really, by design, shouldn't be necessary.
        if ( strcmp(((Section*)sectionList.head->data)->title, "Default Title") != 0 ) free(((Section*)sectionList.head->data)->title);
        free_list_node(&sectionList, sectionList.head, 1);
    }

    // Free the section path list.
    
    while ( filenameList.head != NULL ) {
        free(((struct SectionTitlePair *) filenameList.head->data)->sectionPath);
        free_list_node(&filenameList, filenameList.head, TRUE);
    }
    
    // Print the location of the construction directory if the keep option is specified.
    if ( arguments.keep ) printf("Construction Directory: %s\n", uuid);

    fclose(toc);

    create_zip(uuid, arguments.outputFile);

    // Remove the epub directory.
    if ( !arguments.keep ) rmrf(uuid);
    free(uuid);
    free(path);

    if ( !outputFileProvided ) free(arguments.outputFile);

    printf("Title: %s\n", arguments.title);
    printf("Delimiter: %s\n", arguments.delimiter == NULL ? "No Delimiter" : arguments.delimiter);

    return 0;
}
