/* incbin.h - put a file inside the program, in chip RAM or in any RAM.
 *
 *   CHIP_INCBIN(my_module, "assets/song.mod");     chip RAM: modules, and anything Paula plays directly
 *   DATA_INCBIN(my_effect, "assets/fx/boom.raw");  any RAM: effects for the mixer, tables
 *
 * Each gives  UBYTE my_x[]  and  UBYTE my_x_end[]  (size = my_x_end - my_x). The path is looked up from the
 * project folder. Use these and not "section name,data_c" in a vasm file: this compiler's linker drops vasm's
 * chip RAM mark, the data lands in slow or fast RAM on any machine that has some, and Paula plays silence
 * (tools/hunks.py shows where each part of a built program will be loaded).
 */
#ifndef INCBIN_H
#define INCBIN_H
#include <exec/types.h>

#define INCBIN_IN(section, symbol, path) \
    __asm__(".section " section "\n" \
            "\t.even\n" \
            "\t.globl _" #symbol "\n" \
            "_" #symbol ":\n" \
            "\t.incbin \"" path "\"\n" \
            "\t.globl _" #symbol "_end\n" \
            "_" #symbol "_end:\n" \
            "\t.even\n" \
            "\t.text\n"); \
    extern UBYTE symbol[], symbol##_end[]

#define CHIP_INCBIN(symbol, path) INCBIN_IN(".datachip", symbol, path)
#define DATA_INCBIN(symbol, path) INCBIN_IN(".data", symbol, path)
#endif
