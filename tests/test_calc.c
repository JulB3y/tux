#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "include/module.h"
#include "include/types.h"

Module *calc_module_create(void);

int main() {
    printf("Testing calc module...\n");
    
    // Test case 1: (2+7)-2 should equal 7
    const char* test1 = "(2+7)-2";
    Result results[1];
    
    Module* calc = calc_module_create();
    if (!calc) {
        printf("Failed to create calc module\n");
        return 1;
    }
    
    int found = calc->search(test1, results, 1);
    if (found && results[0].type == RESULT_CALC) {
        double result = atof(results[0].title);
        printf("Test 1: %s = %s (expected 7) - %s\n", test1, results[0].title, fabs(result - 7) < 1e-9 ? "PASS" : "FAIL");
    } else {
        printf("Test 1: %s - FAIL (no result)\n", test1);
    }
    
    // Test case 2: (5+3)-1 should equal 7
    const char* test2 = "(5+3)-1";
    found = calc->search(test2, results, 1);
    if (found && results[0].type == RESULT_CALC) {
        double result = atof(results[0].title);
        printf("Test 2: %s = %s (expected 7) - %s\n", test2, results[0].title, fabs(result - 7) < 1e-9 ? "PASS" : "FAIL");
    } else {
        printf("Test 2: %s - FAIL (no result)\n", test2);
    }
    
    // Test case 3: (10+5)-3 should equal 12
    const char* test3 = "(10+5)-3";
    found = calc->search(test3, results, 1);
    if (found && results[0].type == RESULT_CALC) {
        double result = atof(results[0].title);
        printf("Test 3: %s = %s (expected 12) - %s\n", test3, results[0].title, fabs(result - 12) < 1e-9 ? "PASS" : "FAIL");
    } else {
        printf("Test 3: %s - FAIL (no result)\n", test3);
    }
    
    calc->destroy(calc);
    printf("\nAll tests completed!\n");
    return 0;
}