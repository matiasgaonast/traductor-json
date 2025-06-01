//Traductor de JSON a XML
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// Definición de los tipos de token
typedef enum {
    L_CORCHETE, R_CORCHETE, L_LLAVE, R_LLAVE, COMA, DOS_PUNTOS,
    LITERAL_CADENA, LITERAL_NUM, PR_TRUE, PR_FALSE, PR_NULL,
    EOF_TOKEN, TOKEN_INVALID
} TokenType;

// Estructura para los tokens
typedef struct {
    TokenType type;
    char lexeme[128];
} Token;

// Declaraciones de funciones auxiliares
const char* tokenTypeToString(TokenType type);

Token currentToken;
bool errorReported = false;
bool anyError = false;
FILE *fuente;
int lastChar = ' ';
int currentLine = 1;

// Lexer
void initLexer(const char *filename) {
    fuente = fopen(filename, "r");
    if (!fuente) {
        perror("No se pudo abrir el archivo fuente");
        exit(1);
    }
}

int nextChar() {
    int c = fgetc(fuente);
    if (c == '\n') currentLine++;
    return c;
}

void skipWhitespace() {
    while (isspace(lastChar)) lastChar = nextChar();
}

Token getNextToken() {
    Token t;
    t.lexeme[0] = '\0';
    skipWhitespace();

    if (lastChar == EOF) {
        t.type = EOF_TOKEN;
        strcpy(t.lexeme, "EOF");
        return t;
    }

    if (lastChar == '{') {
        t.type = L_LLAVE; strcpy(t.lexeme, "{"); lastChar = nextChar();
    } else if (lastChar == '}') {
        t.type = R_LLAVE; strcpy(t.lexeme, "}"); lastChar = nextChar();
    } else if (lastChar == '[') {
        t.type = L_CORCHETE; strcpy(t.lexeme, "["); lastChar = nextChar();
    } else if (lastChar == ']') {
        t.type = R_CORCHETE; strcpy(t.lexeme, "]"); lastChar = nextChar();
    } else if (lastChar == ':') {
        t.type = DOS_PUNTOS; strcpy(t.lexeme, ":"); lastChar = nextChar();
    } else if (lastChar == ',') {
        t.type = COMA; strcpy(t.lexeme, ","); lastChar = nextChar();
    } else if (lastChar == '"') {
        int i = 0; lastChar = nextChar();
        while (lastChar != '"' && lastChar != EOF && i < 120) {
            t.lexeme[i++] = lastChar;
            lastChar = nextChar();
        }
        t.lexeme[i] = '\0';
        if (lastChar == '"') {
            t.type = LITERAL_CADENA; lastChar = nextChar();
        } else {
            t.type = TOKEN_INVALID;
        }
    } else if (isdigit(lastChar) || lastChar == '-') {
        int i = 0;
        while (isdigit(lastChar) || lastChar == '.' || lastChar == '-' || lastChar == 'e' || lastChar == 'E') {
            t.lexeme[i++] = lastChar; lastChar = nextChar();
        }
        t.lexeme[i] = '\0';
        t.type = LITERAL_NUM;
    } else if (lastChar == 't') {
        char buffer[5] = {0}; buffer[0] = 't';
        for (int i = 1; i < 4; i++) buffer[i] = nextChar();
        buffer[4] = '\0';
        if (strcmp(buffer, "true") == 0) {
            t.type = PR_TRUE; strcpy(t.lexeme, "true"); lastChar = nextChar();
        } else {
            t.type = TOKEN_INVALID; strcpy(t.lexeme, buffer); lastChar = nextChar();
        }
    } else if (lastChar == 'f') {
        char buffer[6] = {0}; buffer[0] = 'f';
        for (int i = 1; i < 5; i++) buffer[i] = nextChar();
        buffer[5] = '\0';
        if (strcmp(buffer, "false") == 0) {
            t.type = PR_FALSE; strcpy(t.lexeme, "false"); lastChar = nextChar();
        } else {
            t.type = TOKEN_INVALID; strcpy(t.lexeme, buffer); lastChar = nextChar();
        }
    } else if (lastChar == 'n') {
        char buffer[5] = {0}; buffer[0] = 'n';
        for (int i = 1; i < 4; i++) buffer[i] = nextChar();
        buffer[4] = '\0';
        if (strcmp(buffer, "null") == 0) {
            t.type = PR_NULL; strcpy(t.lexeme, "null"); lastChar = nextChar();
        } else {
            t.type = TOKEN_INVALID; strcpy(t.lexeme, buffer); lastChar = nextChar();
        }
    } else {
        t.type = TOKEN_INVALID;
        sprintf(t.lexeme, "%c", lastChar);
        lastChar = nextChar();
    }
    return t;
}

void syntaxError(const char* msg) {
    if (!errorReported) {
        printf("Error de sintaxis en línea %d: %s (token actual: '%s')\n", currentLine, msg, currentToken.lexeme);
        errorReported = true;
        anyError = true;
    }
}

void match(TokenType expected) {
    if (currentToken.type == expected) {
        currentToken = getNextToken();
    } else {
        char buffer[128];
        sprintf(buffer, "Se esperaba el token %s", tokenTypeToString(expected));
        syntaxError(buffer);
    }
}

bool inFollowSet(TokenType tok, TokenType follow[], int size) {
    for (int i = 0; i < size; i++) if (tok == follow[i]) return true;
    return false;
}

void scanUntilFollow(TokenType follow[], int size) {
    while (!inFollowSet(currentToken.type, follow, size) && currentToken.type != EOF_TOKEN)
        currentToken = getNextToken();
    errorReported = false;
}
//Fin del Lexer

// Archivo de salida para XML
FILE *salida;

// Pila de nombres de etiquetas para traducir objetos
char etiquetaActual[128] = "";

void writeIndent(int level) {
    for (int i = 0; i < level; i++) fprintf(salida, "\t");
}

void attribute_value(int indent);
void object(int indent);
void array(int indent);
void element(int indent);
void syntaxError(const char* msg);

// Traduce un objeto JSON a XML
void object(int indent) {
    if (currentToken.type != L_LLAVE) {
        syntaxError("Se esperaba '{' para comenzar un objeto");
        TokenType follow[] = { COMA, R_CORCHETE, R_LLAVE, EOF_TOKEN }; // sincronización
        scanUntilFollow(follow, 4);
        return;
    }
    match(L_LLAVE);
    if (currentToken.type == LITERAL_CADENA) {
        do {
            if (currentToken.type != LITERAL_CADENA) break;
            char clave[128];
            strcpy(clave, currentToken.lexeme);
            match(LITERAL_CADENA);
            match(DOS_PUNTOS);
            writeIndent(indent);
            fprintf(salida, "<%s>", clave);

            if (currentToken.type == L_LLAVE || currentToken.type == L_CORCHETE) {
                fprintf(salida, "\n");
                attribute_value(indent + 1);
                writeIndent(indent);
            } else {
                attribute_value(0);
            }
            fprintf(salida, "</%s>\n", clave);

            if (currentToken.type == COMA)
                match(COMA);
            else
                break;
        } while (currentToken.type == LITERAL_CADENA);
    }
    if (currentToken.type != R_LLAVE) {
        syntaxError("Se esperaba '}' para cerrar el objeto");
        TokenType follow[] = { COMA, R_CORCHETE, R_LLAVE, EOF_TOKEN }; // sincronización
        scanUntilFollow(follow, 4);
    } else {
        match(R_LLAVE);
    }
}

// Traduce un arreglo JSON a XML, envolviendo elementos en <item>
void array(int indent) {
    if (currentToken.type != L_CORCHETE) {
        syntaxError("Se esperaba '[' para comenzar un arreglo");
        TokenType follow[] = { COMA, R_LLAVE, R_CORCHETE, EOF_TOKEN }; // sincronización
        scanUntilFollow(follow, 4);
        return;
    }
    match(L_CORCHETE);
    if (currentToken.type != R_CORCHETE) {
        do {
            writeIndent(indent);
            fprintf(salida, "<item>\n");
            element(indent + 1);
            writeIndent(indent);
            fprintf(salida, "</item>\n");
            if (currentToken.type == COMA)
                match(COMA);
            else
                break;
        } while (1);
    }
    if (currentToken.type != R_CORCHETE) {
        syntaxError("Se esperaba ']' para cerrar el arreglo");
        TokenType follow[] = { COMA, R_LLAVE, EOF_TOKEN }; // sincronización
        scanUntilFollow(follow, 3);
    } else {
        match(R_CORCHETE);
    }
}

// Traduce un valor JSON según su tipo
void attribute_value(int indent) {
    switch (currentToken.type) {
        case LITERAL_CADENA:
            fprintf(salida, "\"%s\"", currentToken.lexeme);
            match(LITERAL_CADENA);
            break;
        case LITERAL_NUM:
        case PR_TRUE:
        case PR_FALSE:
        case PR_NULL:
            fprintf(salida, "%s", currentToken.lexeme);
            match(currentToken.type);
            break;
        case L_LLAVE:
            fprintf(salida, "\n");
            object(indent);
            break;
        case L_CORCHETE:
            fprintf(salida, "\n");
            array(indent);
            break;
        default:
            syntaxError("Valor de atributo inválido");
            TokenType follow[] = { COMA, R_LLAVE, R_CORCHETE, EOF_TOKEN };
            scanUntilFollow(follow, 4);
    }
}

// Traduce un elemento JSON (objeto o arreglo)
void element(int indent) {
    if (currentToken.type == L_LLAVE)
        object(indent);
    else if (currentToken.type == L_CORCHETE)
        array(indent);
    else {
        syntaxError("Se esperaba '{' o '['");
        TokenType follow[] = { COMA, R_CORCHETE, R_LLAVE, EOF_TOKEN };
        scanUntilFollow(follow, 4);
    }
}

// Entrada principal JSON
void json() {
    if (currentToken.type == L_LLAVE) {
        match(L_LLAVE);
        // Procesa la clave principal (ej. "personas")
        if (currentToken.type == LITERAL_CADENA) {
            strcpy(etiquetaActual, currentToken.lexeme);
            match(LITERAL_CADENA);
            match(DOS_PUNTOS);
            fprintf(salida, "<%s>\n", etiquetaActual);
            element(1);
            fprintf(salida, "</%s>\n", etiquetaActual);
        }
        match(R_LLAVE);
    }
    if (currentToken.type != EOF_TOKEN)
        syntaxError("Se esperaba fin de archivo");
    else if (!anyError)
        printf("Traducción completada correctamente.\n");
}

const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case L_CORCHETE: return "'['";
        case R_CORCHETE: return "']'";
        case L_LLAVE: return "'{'";
        case R_LLAVE: return "'}'";
        case COMA: return "','";
        case DOS_PUNTOS: return "':'";
        case LITERAL_CADENA: return "cadena";
        case LITERAL_NUM: return "número";
        case PR_TRUE: return "true";
        case PR_FALSE: return "false";
        case PR_NULL: return "null";
        case EOF_TOKEN: return "fin de archivo";
        case TOKEN_INVALID: return "token inválido";
        default: return "desconocido";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Uso: %s archivo_entrada.json archivo_salida.xml\n", argv[0]);
        return 1;
    }
    initLexer(argv[1]);
    salida = fopen(argv[2], "w");
    if (!salida) {
        perror("No se pudo abrir archivo de salida");
        return 1;
    }
    lastChar = nextChar();
    currentToken = getNextToken();
    json();
    fclose(fuente);
    fclose(salida);
    return 0;
}
