#include <stdio.h>
#include <string.h>

int main() {
    char test1[] = "[aliases]";
    char test2[] = "[[aliases]]";
    char test3[] = "[[commands]]";
    
    printf("Testing section parsing logic:\n");
    
    char *trimmed = test1;
    printf("\nTest 1: '%s'\n", trimmed);
    printf("  Original: '%s'\n", trimmed);
    int is_double = (trimmed[1] == '[');
    printf("  is_double: %d\n", is_double);
    trimmed++;
    if (is_double) trimmed++;
    printf("  After increment: '%s'\n", trimmed);
    size_t end_offset = is_double ? 2 : 1;
    printf("  end_offset: %zu\n", end_offset);
    printf("  strlen(trimmed): %zu\n", strlen(trimmed));
    trimmed[strlen(trimmed) - end_offset] = '\0';
    printf("  Final: '%s'\n", trimmed);
    
    trimmed = test2;
    printf("\nTest 2: '%s'\n", trimmed);
    printf("  Original: '%s'\n", trimmed);
    is_double = (trimmed[1] == '[');
    printf("  is_double: %d\n", is_double);
    trimmed++;
    if (is_double) trimmed++;
    printf("  After increment: '%s'\n", trimmed);
    end_offset = is_double ? 2 : 1;
    printf("  end_offset: %zu\n", end_offset);
    printf("  strlen(trimmed): %zu\n", strlen(trimmed));
    trimmed[strlen(trimmed) - end_offset] = '\0';
    printf("  Final: '%s'\n", trimmed);
    
    trimmed = test3;
    printf("\nTest 3: '%s'\n", trimmed);
    printf("  Original: '%s'\n", trimmed);
    is_double = (trimmed[1] == '[');
    printf("  is_double: %d\n", is_double);
    trimmed++;
    if (is_double) trimmed++;
    printf("  After increment: '%s'\n", trimmed);
    end_offset = is_double ? 2 : 1;
    printf("  end_offset: %zu\n", end_offset);
    printf("  strlen(trimmed): %zu\n", strlen(trimmed));
    trimmed[strlen(trimmed) - end_offset] = '\0';
    printf("  Final: '%s'\n", trimmed);
    
    return 0;
}