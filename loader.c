#include "loader.h"

char **script = NULL;
size_t script_size = 0;

static char *clean_line(char *line){
    
    char *out = malloc(1);
    size_t len = 0;

    // salta spazi e tab iniziali
    while (*line == ' ' || *line == '\t')
        line++;

    while (*line){

        if (*line != '\n' && *line != '\r' && *line != '\t'){

            out = realloc(out, len + 2);
            out[len++] = *line;
        }

        line++;
    }

    while (len && out[len - 1] == ' ')
        len--;

    out[len] = '\0';

    return out;
}

void load_script(const char *filename){

    FILE *fp = fopen(filename, "r");

    if (!fp)
        return;

    char *line = NULL;
    size_t len = 0;

    int c;

    while ((c = fgetc(fp)) != EOF){

        if (c != '\n'){

            line = realloc(line, len + 2);
            line[len++] = (char)c;
            line[len] = '\0';
        }
        else{

            char *clean = clean_line(line ? line : "");

            if (*clean){

                script = realloc(script, sizeof(char *) * (script_size + 1));
                script[script_size++] = clean;
            }
            else
                free(clean);

            free(line);
            line = NULL;
            len = 0;
        }
    }

    if (line){

        char *clean = clean_line(line);

        if (*clean){

            script = realloc(script, sizeof(char *) * (script_size + 1));
            script[script_size++] = clean;
        }
        else
            free(clean);

        free(line);
    }

    fclose(fp);
}

void free_script(void){

    for (size_t i = 0; i < script_size; i++)
        free(script[i]);

    free(script);

    script = NULL;
    script_size = 0;
}