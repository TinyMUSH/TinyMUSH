#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "src/netmush/ansi.h"

int main() {
    char input[] = "r";
    char *ptr = input;
    ColorState color = {0};
    bool hilite = false;
    
    int consumed = ansi_parse_single_x_code(&ptr, &color, &hilite);
    
    printf("Input: 'r'\n");
    printf("Consumed: %d\n", consumed);
    printf("Foreground is_set: %d\n", color.foreground.is_set);
    printf("Background is_set: %d\n", color.background.is_set);
    printf("Highlight: %d\n", color.highlight);
    
    // Test ANSI generation
    char ansi_buf[256] = {0};
    size_t offset = 0;
    ColorStatus result = to_ansi_escape_sequence(ansi_buf, sizeof(ansi_buf), &offset, &color, ColorTypeTrueColor);
    
    printf("ANSI result status: %d\n", result);
    printf("ANSI buffer: '%s'\n", ansi_buf);
    printf("ANSI length: %zu\n", offset);
    
    return 0;
}
