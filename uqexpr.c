#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tinyexpr.h>

const char* const def = "--def";
const char* const loop = "--forloop";
const char* const sig = "--sigfigures";
const char* const usageError =
    "Usage: ./uqexpr [--sigfigures 2..9] [--forloop "
    "string] [--def string] [inputfilename]\n";
const char* const invalidVariablesError =
    "uqexpr: invalid variable(s) specified on the command line\n";
const char* const duplicateNameError =
    "uqexpr: one or more variables are duplicated\n";
const char* const fileReadError =
    "uqexpr: unable to read from input file \"%s\"\n";
const char* const noFileFound =
    "Submit your expressions and assignment "
    "operations to be evaluated.\n";
const char* const welcomeMessage =
    "Welcome to uqexpr!\nThis program was written by s4828041.\n";
const char* const endMessage = "Thanks for using uqexpr!\n";
const char* const runningError =
    "Error in command, expression or assignment operation detected\n";
const char* const print = "@print";
const char* const range = "@range";

#define USAGE_CODE 12
#define INVALID_VARIABLES_CODE 4
#define DUPLICATE_NAME_CODE 18
#define FILE_READ_CODE 19
#define DEFAULT_SIG_FIGURES 3
#define SIG_FIGURES_MIN 2
#define SIG_FIGURES_MAX 9
#define DEF_TOKEN_SIZE 3
#define LOOP_TOKEN_SIZE 5
#define LOOP_VARIABLE_SIZE 4
#define ASSIGNMENT_TOKEN_SIZE 2
#define BUFFER_SIZE 80
#define VARIABLE_NAME_MIN 1
#define VARIABLE_NAME_MAX 20
#define DEF_FORMAT_SIZE 50
#define LOOP_FORMAT_SIZE 100
#define EXPRESSION_FORMAT_SIZE 20
#define VARIABLE_NAME_SIZE 21
#define START 1
#define INCREMENT 2
#define END 3
#define LOOP_VALUES_SIZE 3

// https://edstem.org/au/courses/19964/lessons/67046/slides/451584 A Def
// structure, which is made of a variable name and a value. Named after the
// --def arg which creates them
typedef struct Def {
  char* name;
  double value;
} Def;

// A Loop structure, which is made of a variable name, start, increment, and end
// value. Named after the --forloop arg which creates them
typedef struct Loop {
  char* name;
  double start;
  double increment;
  double end;
} Loop;

/**
 * variable_print()
 * ----------------
 * Prints all defined variables and loop variables along with their values,
 *formatted according to the specified significant figures.
 *
 * defs: Array of defined variables.
 * defSize: Pointer to the number of defined variables.
 * loops: Array of loop variables.
 * loopSize: Pointer to the number of loop variables.
 * sigFigures: Number of significant figures to use when printing values.
 *
 * Returns: void
 *
 **/
void variable_print(Def** defs, const int* defSize, Loop** loops,
                    const int* loopSize, int sigFigures);
int valid_variable_name(const char* variableName);
void add_def(Def** defs, int* defSize, const char* name, double value);

/**
 * print_expression()
 * ----------------
 * Prints the evaluated result of an expression with the specified number of
 *significant figures. In the format Result = <expression-result>
 *
 * result: The computed numerical result of an expression.
 * sigFigures: The number of significant figures to use when printing.
 *
 * Returns: void
 *
 **/
void print_expression(double result, int sigFigures) {
  char expressionFormat[EXPRESSION_FORMAT_SIZE];

  // Create formats and print with sigFigures values
  snprintf(expressionFormat, sizeof(expressionFormat), "Result = %%.%dg\n",
           sigFigures);

  printf(expressionFormat, result);
}

/**
 * handle_new_variable()
 * ----------------
 * Handles variable assignment, ensuring uniqueness, overwriting existing values
 *if needed, or adding new definitions.
 *
 * defs: Pointer to the array of defined variables.
 * defSize: Pointer to the number of defined variables.
 * loops: Pointer to the array of loop variables.
 * loopSize: Pointer to the number of loop variables.
 * name: Name of the variable being assigned.
 * value: Value to assign to the variable.
 * sigFigures: Number of significant figures to use when printing the
 *assignment.
 *
 * Returns: void
 *
 **/
void handle_new_variable(Def** defs, int* defSize, Loop** loops,
                         const int* loopSize, const char* name,
                         const double value, const int sigFigures) {
  // Successful assignment is always printed
  char newVariableFormat[VARIABLE_NAME_SIZE];
  snprintf(newVariableFormat, sizeof(newVariableFormat), "%%s = %%.%dg\n",
           sigFigures);

  printf(newVariableFormat, name, value);

  // If there is a loop variable with this name, do nothing
  if (*loopSize != 0) {
    for (int i = 0; i < *loopSize; ++i) {
      if (strcmp((*loops)[i].name, name) == 0) {
        return;
      }
    }
  }

  // If there are no defs, add this one
  if (*defSize == 0) {
    add_def(defs, defSize, name, value);
    return;
  }

  // If a def with the same name already exists, overwrite it
  for (int i = 0; i < *defSize; ++i) {
    if (strcmp((*defs)[i].name, name) == 0) {
      (*defs)[i].value = value;
      return;
    }
  }
  // If every check is passed, add the new def
  add_def(defs, defSize, name, value);
}

/**
 * tiny_expr()
 * ----------------
 * Evaluates a mathematical expression using tinyexpr, allowing for variable
 *substitutions.
 *
 * expression: Null-terminated string representing the mathematical expression
 *to evaluate. defs: Pointer to the array of defined variables. defSize: Pointer
 *to the number of defined variables.
 *
 * Returns: The computed result of the expression. If evaluation fails, returns
 *NAN.
 *
 * Errors: If the expression is invalid, returns NAN.
 *
 *21/3/25 11:30
 **/
double tiny_expr(const char* expression, Def** defs, const int* defSize) {
  // Create an array of te_variable using the provided defs
  te_variable teVars[*defSize];

  // Initialise result, default to NAN if invalid input
  double result = NAN;

  // Populate teVars if variables exist
  // tiny_expr() 21/3/25 14:07
  if (*defSize != 0) {
    for (int i = 0; i < *defSize; i++) {
      teVars[i].name = (*defs)[i].name;
      teVars[i].address = &(*defs)[i].value;  // Use existing values from defs
      teVars[i].type = TE_VARIABLE;
      teVars[i].context = NULL;
    }
  }

  int errPos;
  // Parse and compile the expression using defs
  te_expr* expr = te_compile(expression, teVars, *defSize, &errPos);

  if (expr) {
    result = te_eval(expr);
    te_free(expr);
  }
  return result;
}

/**
 * expression_handler()
 * ----------------
 * Processes an expression by evaluating it and printing the result if valid.
 *
 * strippedLine: Null-terminated string containing the expression to evaluate.
 * defs: Pointer to the array of defined variables.
 * defSize: Pointer to the number of defined variables.
 * sigFigures: Number of significant figures to use when printing the result.
 *
 * Returns: void
 *
 * Errors: If the expression is invalid, prints an error message to stderr.
 *
 **/
void expression_handler(char strippedLine[], Def** defs, const int* defSize,
                        const int sigFigures) {
  const double result = tiny_expr(strippedLine, defs, defSize);
  if (isnan(result)) {
    fprintf(stderr, runningError);
    return;
  }
  print_expression(result, sigFigures);
}

/**
 * assignment_handler()
 * ----------------
 * Processes an assignment operation, validating variable names and evaluating
 *the assigned expression.
 *
 * strippedLine: Null-terminated string containing the assignment operation.
 * defs: Pointer to the array of defined variables.
 * defSize: Pointer to the number of defined variables.
 * loops: Pointer to the array of loop variables.
 * loopSize: Pointer to the number of loop variables.
 * sigFigures: Number of significant figures to use when printing the
 *assignment.
 *
 * Returns: void
 *
 * Errors: If the assignment is invalid, prints an error message to stderr.
 *
 **/
void assignment_handler(char strippedLine[], Def** defs, int* defSize,
                        Loop** loops, const int* loopSize,
                        const int sigFigures) {
  // Split up string based on '='
  char* tokens[ASSIGNMENT_TOKEN_SIZE] = {NULL, NULL};
  int count = 0;

  char* myptr = strtok(strippedLine, "=");
  while (myptr != NULL && count < ASSIGNMENT_TOKEN_SIZE) {
    tokens[count] = myptr;
    count += 1;
    myptr = strtok(NULL, "=");
  }

  // check if expression is valid and store it
  const double result = tiny_expr(tokens[1], defs, defSize);
  if (isnan(result)) {
    fprintf(stderr, runningError);
    return;
  }

  // check if variable name is allowed
  if (valid_variable_name(tokens[0]) == 0) {
    fprintf(stderr, runningError);
    return;
  }

  // If expression is valid and variable name allowed, handle appropriately
  handle_new_variable(defs, defSize, loops, loopSize, tokens[0], result,
                      sigFigures);
}

/**
 * line_handler()
 * ----------------
 * Processes a line of input from stdin or a file, determining whether it is an
 *expression, an assignment, or a command.
 *
 * line: Null-terminated string containing the line to process.
 * defs: Pointer to an array of defined variables.
 * defSize: Pointer to an integer representing the number of defined variables.
 * loops: Pointer to an array of loop variables.
 * loopSize: Pointer to an integer representing the number of loop variables.
 * sigFigures: The number of significant figures to use when evaluating
 *expressions.
 *
 * Returns: void
 *
 * Errors: If the line contains an invalid assignment or expression, an error
 *message is printed to stderr.
 *
 **/
void line_handler(char line[], Def** defs, int* defSize, Loop** loops,
                  int* loopSize, int sigFigures) {
  // Strip white space before processing line
  // https://www.geeksforgeeks.org/c-program-to-trim-leading-white-spaces-from-string/
  char strippedLine[strlen(line) + 1];
  int writePtr = 0;

  for (int readPtr = 0; line[readPtr] != '\0'; ++readPtr) {
    if (!isspace(line[readPtr])) {
      strippedLine[writePtr++] = line[readPtr];
    }
  }
  strippedLine[writePtr] = '\0';

  // Ignore lines that are commented out or blank
  if (strippedLine[0] == '#' || strippedLine[0] == '\0') {
    return;
  }

  // If line == @print, run initial_print
  if (strcmp(line, print) == 0) {
    variable_print(defs, defSize, loops, loopSize, sigFigures);
    return;
  }

  // Determine if the line is an assignment or an expression by counting the
  // number of '=' present
  int equalsCounter = 0;
  for (size_t i = 1; i < strlen(strippedLine); ++i) {
    if (strippedLine[i] == '=') {
      equalsCounter++;
    }
  }
  if (equalsCounter > 1) {
    fprintf(stderr, runningError);
    return;
  }

  if (equalsCounter == 0) {
    expression_handler(strippedLine, defs, defSize, sigFigures);
  }

  if (equalsCounter == 1) {
    assignment_handler(strippedLine, defs, defSize, loops, loopSize,
                       sigFigures);
  }
}

/**
 * read_line()
 * ----------------
 * Reads a line from a file or stdin and returns a dynamically allocated string.
 *
 * file: Pointer to a FILE stream to read from.
 *
 * Returns: A dynamically allocated string containing the line read from the
 *file. Caller must free the returned memory. Returns NULL on EOF.
 *
 * Errors: If memory allocation fails, returns NULL.
 *
 **/
char* read_line(FILE* file) {
  int bufferSize = BUFFER_SIZE;

  char* buffer = malloc(sizeof(char) * bufferSize);
  int numRead = 0;
  int next;

  if (feof(file)) {
    free(buffer);
    return NULL;
  }

  while (1) {
    next = fgetc(file);
    if (next == EOF && numRead == 0) {
      free(buffer);
      return NULL;
    }
    if (numRead == bufferSize - 1) {
      bufferSize *= 2;
      buffer = realloc(buffer, sizeof(char) * bufferSize);
    }
    if (next == '\n' || next == EOF) {
      buffer[numRead] = '\0';
      break;
    }
    buffer[numRead++] = next;
  }

  char* result = strdup(buffer);
  free(buffer);
  return result;
}

/**
 * file_handler()
 * ----------------
 * Reads a file line by line and processes each line accordingly.
 *
 * fileName: Null-terminated string containing the name of the file to read.
 * defs: Pointer to the array of defined variables.
 * defSize: Pointer to the number of defined variables.
 * loops: Pointer to the array of loop variables.
 * loopSize: Pointer to the number of loop variables.
 * sigFigures: Number of significant figures to use when processing.
 *
 * Returns: void
 *
 * Errors: If the file cannot be opened, prints an error and exits.
 *
 **/
void file_handler(char fileName[], Def** defs, int* defSize, Loop** loops,
                  int* loopSize, int sigFigures) {
  FILE* file = fopen(fileName, "r");

  // Send lines to read_line() to be read and then send to line_handler() to
  // be processed
  char* line;
  while ((line = read_line(file)) != NULL) {
    line_handler(line, defs, defSize, loops, loopSize, sigFigures);
    free(line);
  }

  fclose(file);
}

/**
 * file_validator()
 * ----------------
 * Checks whether a given file can be opened for reading. If the file cannot be
 *opened, the function exits the program with an error.
 *
 * fileName: Null-terminated string representing the name of the file to
 *validate. filePresent: Pointer to an integer flag that is set to 1 if the file
 *is valid.
 *
 * Returns: void
 *
 * Errors:
 * - If the filename starts with "--", prints an error message and exits with
 *code 12.
 * - If the file cannot be opened, prints an error message and exits with
 *code 19.
 *
 **/
void file_validator(char fileName[], int* filePresent) {
  // Files cannot start with '--'
  if (fileName[0] == '-' && fileName[1] == '-') {
    fprintf(stderr, usageError);
    exit(USAGE_CODE);
  }

  FILE* file = fopen(fileName, "r");

  // https://stackoverflow.com/questions/11772291/how-can-i-print-a-quotation-mark-in-c
  if (!file) {
    fprintf(stderr, fileReadError, fileName);
    exit(FILE_READ_CODE);
  }

  // Change variable to indicate a valid file has been entered for future
  // print statements
  *filePresent = 1;

  fclose(file);
}

/**
 * valid_double()
 * ----------------
 * Checks whether a given string represents a valid floating-point number.
 *
 * value: Null-terminated string to validate as a double.
 *
 * Returns: 1 if the string is a valid double, 0 otherwise.
 *
 **/
int valid_double(const char* value) {
  char* endptr;
  // https://www.tutorialspoint.com/c_standard_library/c_function_strtod.htm
  strtod(value, &endptr);

  // Return 1 if entire string is consumed, 0 if not
  return (*endptr == '\0');
}

/**
 * valid_variable_name()
 * ----------------
 * Checks if the given variable name consists only of alphabetic characters and
 *is between 1-20 characters in length.
 *
 * variableName: Pointer to a null-terminated string representing the variable
 *name.
 *
 * Returns: 1 if the variable name is valid, 0 otherwise.
 *
 **/
int valid_variable_name(const char* variableName) {
  int length = strlen(variableName);
  if (length < VARIABLE_NAME_MIN || length > VARIABLE_NAME_MAX) {
    return 0;
  }
  for (int i = 0; i < length; ++i) {
    if (!isalpha(variableName[i])) {
      return 0;
    }
  }
  return 1;
}

/**
 * add_loop()
 * ----------------
 * Adds a new loop variable definition to the list of loop variables,
 *reallocating memory as needed.
 *
 * loops: Pointer to the array of loop variables.
 * loopSize: Pointer to the number of loop variables.
 * name: Name of the loop variable.
 * start: Start value of the loop.
 * increment: Increment value of the loop.
 * end: End value of the loop.
 *
 * Returns: void
 *
 **/
void add_loop(Loop** loops, int* loopSize, const char* name, double start,
              double increment, double end) {
  // Reallocate memory for the array of Defs (size increases by 1)
  Loop* temp = realloc(*loops, (*loopSize + 1) * sizeof(Loop));

  *loops = temp;

  // Add new Loop to the end of the array
  (*loops)[*loopSize].name = strdup(name);
  (*loops)[*loopSize].start = start;
  (*loops)[*loopSize].increment = increment;
  (*loops)[*loopSize].end = end;

  (*loopSize)++;  // Increase the size of the array
}

/**
 * validate_loop_tokens()
 * ----------------
 * Validates the tokens representing a loop variable definition, ensuring the
 *variable name is valid and that the start, increment, and end values are valid
 *doubles and follow logical constraints.
 *
 * tokens: Array of strings representing the loop variable name, start,
 *increment, and end values. count: The number of tokens provided.
 *
 * Returns: 1 if the tokens are valid, or 0 if:
 * - The number of tokens is incorrect, returns 0.
 * - The variable name is invalid, returns 0.
 * - Any of the start, increment, or end values are not valid doubles, returns
 *0.
 * - The increment value is 0, returns 0.
 * - The start value is less than the end value, the increment must be positive;
 *otherwise, returns 0.
 * - The start value is greater than the end value, the increment must be
 *negative; otherwise, returns 0.
 *
 **/
int validate_loop_tokens(char* tokens[], int count) {
  // Must have 4 tokens
  if (count != LOOP_VARIABLE_SIZE) {
    return 0;
  }

  // Name must be valid
  if (valid_variable_name(tokens[0]) == 0) {
    return 0;
  }

  // Start, increment, and end variables must be valid doubles
  for (int i = 1; i <= LOOP_VALUES_SIZE; ++i) {
    if (valid_double(tokens[i]) == 0) {
      return 0;
    }
  }

  // https://www.tutorialspoint.com/c_standard_library/c_function_atof.htm
  double start = atof(tokens[START]);
  double increment = atof(tokens[INCREMENT]);
  double end = atof(tokens[END]);

  // Increment must != 0 and if start == end, increment must == any value
  // other than zero
  if (increment == 0) {
    return 0;
  }

  // Increment must be positive if start value < end value
  if (start < end && increment <= 0) {
    return 0;
  }

  // Increment must be negative if start value > end value
  if (start > end && increment >= 0) {
    return 0;
  }

  return 1;
}

/**
 * delim_checker()
 * ----------------
 * Ensures that there are the correct number of seperators for potential --def
 *or --forloop variables.
 *
 * variable: Null-terminated string containing the potential --def or --lop
 *variable. delim: The seperator which will be tested against.
 *
 * Returns: If variable has the correct amount of seperators.
 * - If seperate == '=' and there is not exactly than 1, returns 0.
 * - If delim == ',' and there is not exactly 3, returns 0.
 *
 **/
int delim_check(char string[], char delim) {
  int delimCheck = 0;
  int stringLength = strlen(string);

  // If we are checking for loop variables, make sure there are ',' seperators
  if (delim == ',') {
    for (int i = 0; i < stringLength; ++i) {
      if (string[i] == ',') {
        ++delimCheck;
      }
    }
    if (delimCheck != LOOP_VALUES_SIZE) {
      return 0;
    }
    // Else if we are checking for def variables, ensure there is only 1 '='
    // seperator
  } else if (delim == '=') {
    for (int i = 0; i < stringLength; ++i) {
      if (string[i] == '=') {
        ++delimCheck;
      }
    }
    if (delimCheck != 1) {
      return 0;
    }
  }
  return 1;
}

/**
 * loop_handler()
 * ----------------
 * Parses and validates a loop variable definition, adding it to the list of
 *loop variables if valid.
 *
 * variable: Null-terminated string containing the loop variable definition in
 *the format "name,start,increment,end". loops: Pointer to an array of loop
 *variables. loopSize: Pointer to an integer representing the number of loop
 *variables.
 *
 * Returns: 1 if the loop variable is successfully added, 0 if the definition is
 *invalid.
 * - If memory allocation fails, returns 0.
 * - If the variable definition does not contain the expected number of tokens,
 *returns 0.
 * - If the loop variable name or values are invalid, returns 0.
 *
 **/
int loop_handler(char variable[], Loop** loops, int* loopSize) {
  // Copy variable name into mutable string
  char* variableCopy = strdup(variable);
  if (!variableCopy) {
    return 0;
  }

  // Ensure there is an appropriate number of seperators
  if (delim_check(strdup(variable), ',') == 0) {
    return 0;
  }

  // Setup tokens to handle string when it is split
  // TODO: repeated code, make function for it?
  char* tokens[LOOP_TOKEN_SIZE] = {NULL, NULL, NULL, NULL, NULL};
  int count = 0;

  // Split the string into 5 tokens (5th token is to detect invalid inputs)
  char* myptr = strtok(variableCopy, ",");
  while (myptr != NULL && count < LOOP_TOKEN_SIZE) {
    tokens[count] = myptr;
    count += 1;
    myptr = strtok(NULL, ",");
  }

  // Validate tokens
  if (validate_loop_tokens(tokens, count) == 0) {
    return 0;
  }

  // If valid add them to tokens
  add_loop(loops, loopSize, tokens[0], atof(tokens[START]),
           atof(tokens[INCREMENT]), atof(tokens[END]));

  free(variableCopy);

  return 1;
}

/**
 * add_def()
 * ----------------
 * Adds a new variable definition to the list of defined variables, reallocating
 *memory as needed.
 *
 * defs: Pointer to the array of defined variables.
 * defSize: Pointer to the number of defined variables.
 * name: Name of the variable to be added.
 * value: Value associated with the variable.
 *
 * Returns: void
 *
 **/
void add_def(Def** defs, int* defSize, const char* name, double value) {
  // Reallocate memory for the array of Defs (size increases by 1)
  *defs = realloc(*defs, (*defSize + 1) * sizeof(Def));

  // Add new Def structure at the end of the array
  (*defs)[*defSize].name = strdup(name);
  (*defs)[*defSize].value = value;

  (*defSize)++;
}

/**
 * def_handler()
 * ----------------
 * Parses and validates a variable definition, adding it to the list of defined
 *variables if valid.
 *
 * variable: Null-terminated string containing the variable definition in the
 *format "name=value". defs: Pointer to an array of defined variables. defSize:
 *Pointer to an integer representing the number of defined variables.
 *
 * Returns: 1 if the variable is successfully added, 0 if the definition is
 *invalid.
 * - If memory allocation fails, returns 0.
 * - If the variable definition does not contain exactly one '=' delimiter,
 *returns 0.
 * - If the variable name is invalid, returns 0.
 * - If the value is not a valid double, returns 0.
 *
 **/
int def_handler(char variable[], Def** defs, int* defSize) {
  // Copy variable name into mutable string
  char* variableCopy = strdup(variable);
  if (!variableCopy) {
    return 0;
  }

  // Ensure there is an appropriate number of seperators
  if (delim_check(strdup(variable), '=') == 0) {
    return 0;
  }

  // Setup tokens to handle string when it is split
  char* tokens[DEF_TOKEN_SIZE] = {NULL, NULL, NULL};
  int count = 0;

  // Split the string into 3 tokens (3rd token is to detect invalid inputs)
  char* myptr = strtok(variableCopy, "=");
  while (myptr != NULL && count < DEF_TOKEN_SIZE) {  // Collect up to 3 tokens
    tokens[count] = myptr;
    count += 1;
    myptr = strtok(NULL, "=");
  }

  // Must have 2 tokens
  if (count != 2) {
    return 0;
  }

  // Validate name
  if (valid_variable_name(tokens[0]) == 0) {
    return 0;
  }

  // Value must be a double
  if (valid_double(tokens[1]) == 0) {
    return 0;
  }

  add_def(defs, defSize, tokens[0], atof(tokens[1]));

  free(variableCopy);

  return 1;
}

/**
 * unique_name_check()
 * ----------------
 * Checks whether all defined variables and loop variables have unique names.
 *
 * defs: Pointer to an array of defined variables.
 * defSize: Pointer to an integer representing the number of defined variables.
 * loops: Pointer to an array of loop variables.
 * loopSize: Pointer to an integer representing the number of loop variables.
 *
 * Returns: 1 if all variable names are unique, 0 if duplicates exist.
 *
 *a pointer to them 19/3/25 16:39
 **/
int unique_name_check(Def** defs, const int* defSize, Loop** loops,
                      const int* loopSize) {
  // Initialise array to hold all variable names from defs and loops
  int totalSize = *defSize + *loopSize;
  char names[totalSize][VARIABLE_NAME_SIZE];

  // Add all variable names from loops and defs into names
  int index = 0;
  for (int i = 0; i < *defSize; ++i) {
    strcpy(names[index++], (*defs)[i].name);
  }
  for (int i = 0; i < *loopSize; ++i) {
    strcpy(names[index++], (*loops)[i].name);
  }

  // Check for duplicates in names
  for (int i = 0; i < totalSize; ++i) {
    for (int j = i + 1; j < totalSize; ++j) {
      if (strcmp(names[i], names[j]) == 0) {
        return 0;
      }
    }
  }

  return 1;
}

/**
 * sig_handler()
 * ----------------
 * Processes and validates the `--sigfigures` command-line argument, ensuring it
 *is a single-digit integer between 2 and 9.
 *
 * sigCount: Pointer to an integer tracking the number of times `--sigfigures`
 *has been specified. sigFigures: Pointer to an integer where the validated
 *significant figures value is stored. input: Null-terminated string containing
 *the user-specified value for significant figures.
 *
 * Returns: void
 *
 * Errors:
 * - If `--sigfigures` is specified more than once, prints an error and exits
 *with code 12.
 * - If no value is provided after `--sigfigures`, prints an error and exits
 *with code 12.
 * - If the provided value is not a single-digit integer between 2 and 9, prints
 *an error and exits with code 12.
 *
 **/
void sig_handler(int* sigCount, int* sigFigures, const char* input) {
  // Increment SIG argument count
  (*sigCount)++;

  // Ensure multiple --sig arguments are not used
  if (*sigCount > 1) {
    fprintf(stderr, usageError);
    exit(USAGE_CODE);
  }

  // Ensure `--sig` is not the last argument (avoiding missing value)
  if (input == NULL) {
    fprintf(stderr, usageError);
    exit(USAGE_CODE);
  }

  // Check if the input is a valid single-digit integer between 2-9
  if ((input[0] - '0') >= SIG_FIGURES_MIN &&
      (input[0] - '0') <= SIG_FIGURES_MAX && input[1] == '\0') {
    *sigFigures = input[0] - '0';  // Convert char to int
  } else {
    fprintf(stderr, usageError);
    exit(USAGE_CODE);
  }
}

/**
 * invalid_filename_check()
 * ----------------
 * Ensures that command-line options `--def`, `--sigfigures`, and `--forloop`
 *are followed by a valid filename argument.
 *
 * count: The current argument index being processed.
 * argc: The total number of command-line arguments.
 *
 * Returns: void
 *
 * Errors:
 * - If an expected filename argument is missing, prints an error and exits with
 *code 12.
 *
 **/
// if --def, --sigfigures, or --forloop argument at the end isn't a filename,
// useage error
void invalid_filename_check(const int count, const int argc) {
  if (count >= argc - 1) {
    fprintf(stderr, usageError);
    exit(USAGE_CODE);
  }
}

/**
 * check_validity()
 * ----------------
 * Checks the validity of command-line arguments and processes them accordingly.
 *
 * argc: The number of command-line arguments.
 * argv: The array of command-line argument strings.
 * defs: Pointer to the array of defined variables.
 * defSize: Pointer to the number of defined variables.
 * loops: Pointer to the array of loop variables.
 * loopSize: Pointer to the number of loop variables.
 * sigFigures: Pointer to the significant figures setting.
 * filePresent: Pointer to an integer flag indicating whether a file is present.
 *
 * Returns: void
 *
 * Errors: If an invalid variable is encountered, prints an error and exits with
 *code 4. If several variables have the same name, prints an error and exits
 *with code 12.
 *
 **/
void check_validity(int argc, char* argv[], Def** defs, int* defSize,
                    Loop** loops, int* loopSize, int* sigFigures,
                    int* filePresent) {
  int count = 1;
  int sigCount = 0;

  // Loop through arguments and check validity
  while (count < argc) {
    // Compare strings using strcmp
    if (strcmp(argv[count], def) == 0) {
      invalid_filename_check(count, argc);

      if (def_handler(argv[count + 1], defs, defSize) == 0) {
        fprintf(stderr, invalidVariablesError);
        exit(INVALID_VARIABLES_CODE);
      }
      count += 2;
    } else if (strcmp(argv[count], loop) == 0) {
      invalid_filename_check(count, argc);

      if (loop_handler(argv[count + 1], loops, loopSize) == 0) {
        fprintf(stderr, invalidVariablesError);
        exit(INVALID_VARIABLES_CODE);
      }
      count += 2;
    } else if (strcmp(argv[count], sig) == 0) {
      invalid_filename_check(count, argc);

      sig_handler(&sigCount, sigFigures, argv[count + 1]);

      count += 2;
    } else if (strcmp(argv[count], sig) != 0 &&
               strcmp(argv[count], loop) != 0 &&
               strcmp(argv[count], def) != 0) {
      // If there is an input that isn't sig, loop, or def at the end, it
      // will be treated as a file
      if (count == argc - 1) {
        file_validator(argv[count], filePresent);
      } else {
        fprintf(stderr, usageError);
        exit(USAGE_CODE);
      }
      count += 1;
    }
  }

  // Check that variables all have unique names
  if (unique_name_check(defs, defSize, loops, loopSize) == 0) {
    fprintf(stderr, duplicateNameError);
    exit(DUPLICATE_NAME_CODE);
  }
}

/**
 * variable_print()
 * ----------------
 * Prints all defined variables and loop variables along with their values,
 * formatted according to the specified number of significant figures.
 *
 * defs: Pointer to an array of defined variables.
 * defSize: Pointer to an integer representing the number of defined variables.
 * loops: Pointer to an array of loop variables.
 * loopSize: Pointer to an integer representing the number of loop variables.
 * sigFigures: The number of significant figures to use when printing values.
 *
 * Returns: void
 *
 **/
void variable_print(Def** defs, const int* defSize, Loop** loops,
                    const int* loopSize, int sigFigures) {
  // Create formats for printing with sigFigures values
  char defFormat[DEF_FORMAT_SIZE];
  snprintf(defFormat, sizeof(defFormat), "%%s = %%.%dg\n", sigFigures);

  char loopFormat[LOOP_FORMAT_SIZE];
  snprintf(loopFormat, sizeof(loopFormat),
           "%%s = %%.%dg (%%.%dg, %%.%dg, %%.%dg)\n", sigFigures, sigFigures,
           sigFigures, sigFigures);

  // Print defs if they exist
  if (*defSize == 0) {
    printf("There are no variables.\n");
  } else {
    printf("Variables:\n");
    for (int i = 0; i < *defSize; ++i) {
      printf(defFormat, (*defs)[i].name, (*defs)[i].value);
    }
  }

  // Print loops if they exist
  if (*loopSize == 0) {
    printf("No loop variables were found.\n");
  } else {
    printf("Loop variables:\n");
    for (int i = 0; i < *loopSize; ++i) {
      printf(loopFormat, (*loops)[i].name, (*loops)[i].start, (*loops)[i].start,
             (*loops)[i].increment, (*loops)[i].end);
    }
  }
}

/**
 * main2()
 * ----------------
 * Entry point for the program, handles initialization of key variables and
 *execution of input processing.
 *
 * argc: The number of command-line arguments.
 * argv: The array of command-line argument strings.
 *
 * Returns: 0 on successful execution.
 *
 * Errors: If input processing fails, appropriate error messages are printed,
 *and the program may exit.
 *
 *18/3/25 08:44
 **/
int main(int argc, char* argv[]) {
  // Initialise array of structs for --def and --forloop variables
  Def* defs = NULL;
  int defSize = 0;
  Loop* loops = NULL;
  int loopSize = 0;

  // Initialise significant figures to default size of 3
  int sigFigures = DEFAULT_SIG_FIGURES;

  // Stores 1 if a readable file is input
  int filePresent = 0;

  check_validity(argc, argv, &defs, &defSize, &loops, &loopSize, &sigFigures,
                 &filePresent);

  printf(welcomeMessage);
  variable_print(&defs, &defSize, &loops, &loopSize, sigFigures);

  // Utilise file if present, else process user input
  if (filePresent == 1) {
    file_handler(argv[argc - 1], &defs, &defSize, &loops, &loopSize,
                 sigFigures);
  } else {
    printf(noFileFound);
    char* line;
    while ((line = read_line(stdin)) != NULL) {
      line_handler(line, &defs, &defSize, &loops, &loopSize, sigFigures);
      free(line);
    }
  }
  printf(endMessage);

  return 0;
}
