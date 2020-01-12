#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <wchar.h>
#include <windows.h>
#include <io.h>

#define MAXLINE_LEN 20
#define HISTORY_SIZE 3

#define ESC "\x1b"
#define CSI "\x1b["

typedef struct
{
    int buff_index;
    int hist_size;
    unsigned char** buffers;
} console;

int printable(int c)
{
    if (c >= 32 && c <= 254)
    {
        return 1;
    }
    return 0;
}

console* console_create()
{
    console* c = malloc(sizeof(console));;
    //c->buffer = calloc(MAXLINE_LEN, sizeof(char));
    c->buff_index = 0;
    c->hist_size = 0;
    c->buffers = calloc(HISTORY_SIZE, sizeof(unsigned char*));
    return c;
}

void console_del(console* c)
{
    //free(c->buffer);
    for (int i=0; i < c->hist_size; i++)
    {
        free(c->buffers[i]);
    }
    free(c->buffers);
    free(c);
    return;
}

void buff_shift_left(char* buf, int start)
{
    int i = start;
    while (buf[i] != '\x00') //not checking the length of the buffer because there is ALWAYS a \x00 at the end (by design)
    {
        buf[i] = buf[i + 1];
        i++;
    }
}

void buff_shift_right(char* buf, int start, int end)
{
    int i = end;
    buf[i + 1] = buf[i];
    while (i > start)
    {
        buf[i] = buf[i - 1];
        i--;
    }
}

char* console_input(console* c)
{
    // copy line to history

    // write in newline
    c->buff_index++;                            // choosing the next buffer in the list
    c->buff_index %= HISTORY_SIZE;              // cycle if history is reached
    if (c->buffers[c->buff_index] == 0)         // alloc memory is not done
    {
        c->buffers[c->buff_index] = malloc(MAXLINE_LEN);
        c->hist_size++;
    }
    unsigned char* out = c->buffers[c->buff_index];      // renaming the buffer where we will write
    int a;                                      // input is stored here
    int end = 0;                                // bool to exit the while loop when enter is pressed
    int buff_end = 0;                           // how much actual content is written in the buffer
    int cursor_pos = 0;                         // the position of the cursor on the screen

    while (!end)
    {
        a = _getch();
        switch (a)
        {
            case 3: // Ctrl+C interrupt
            while (cursor_pos != 0)
            {
                printf(ESC "D");
                cursor_pos--;
            }
            printf(CSI "s"); // save cursor pos
            while (buff_end != 0)
            {
                printf(" ");
                buff_end--;
            }
            printf(CSI "u"); // restor cursor pos
            buff_end = 0;
            break;

            case 13: // enter key -> exit the loop
            end = 1;
            printf("\n");
            break;

            case 8: // backspace
            if (cursor_pos != 0)
            {
                buff_end--;
                cursor_pos--;
                // buffer shift to the left
                buff_shift_left(out, cursor_pos);
                // display
                printf(ESC "D"); // curror left
                printf(CSI "s"); // save cursor pos
                write(1, (out + cursor_pos), buff_end - cursor_pos); // re print the end of the line + an extra space
                printf(" ");
                printf(CSI "u"); // restor cursor pos
            }
            break;

            case 224: // special key
            a = _getch(); // second part of the special key
            switch (a)
            {
                case (83): // suppr keys
                if (cursor_pos != buff_end)
                {
                    buff_end--;
                    // buffer shift to the left
                    buff_shift_left(out, cursor_pos);
                    // display
                    printf(CSI "s"); // save cursor pos
                    write(1, (out + cursor_pos), buff_end - cursor_pos); // re print the end of the line + an extra space
                    printf(" ");
                    printf(CSI "u"); // restor cursor pos
                }
                break;

                case (75): // left arrow
                if (cursor_pos != 0)
                {
                    cursor_pos--;
                    printf(ESC "D");
                }
                break;

                case (77): // right arrow
                if (cursor_pos < buff_end)
                {
                    cursor_pos++;
                    printf(ESC "C");
                }
                break;
                case (115): // ctrl left arrow
                while (cursor_pos != 0)
                {
                    cursor_pos--;
                    printf(ESC "D");
                    if (out[cursor_pos-1] == ' ')
                        break;
                }
                break;

                case (116): // ctrl left arrow
                while (cursor_pos < buff_end)
                {
                    cursor_pos++;
                    printf(ESC "C");
                    if (out[cursor_pos] == ' ')
                        break;
                }
                break;

                default:
                break;
            }
            break;

            default:
            if (printable(a)) // printable char range
            {
                if (buff_end < MAXLINE_LEN-1) // leaving one char of the array unused for the \x00 of the end
                {
                    // buffer shift to the left
                    buff_shift_right(out, cursor_pos, buff_end);
                    out[cursor_pos] = a;
                    // display
                    printf("%c", a);
                    if (cursor_pos < buff_end)
                    {
                        printf(CSI "s"); // save cursor pos
                        write(1, (out + cursor_pos + 1), buff_end - cursor_pos); // re print the end of the line + an extra space
                        printf(CSI "u"); // restor cursor pos
                    }

                    buff_end++;
                    cursor_pos++;
                }
            }
            break;
        }
    }
    out[buff_end] = '\x00'; // add a End Of String et the end since there is at least on char left empty
    return out;
}

int EnableVTMode() // dark and evil magic to put the terminal into Vitual Mode (accept escape sequence)
{
    // Set output mode to handle virtual terminal sequences
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
    {
        return 0;
    }

    dwMode |= 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
    if (!SetConsoleMode(hOut, dwMode))
    {
        return 0;
    }
    return 1;
}

int main(int agrc, char** argv)
{
    if (!EnableVTMode())
    {
        printf("Unable to enter VTMode\n");
        return 1;
    }
    //printf("%c\n", 135);

    char* input_buff = calloc(MAXLINE_LEN, sizeof(char));
    console* c = console_create();
    input_buff = console_input(c);
    printf("%s\n", c->buffers[1]);
    console_del(c);
    free(input_buff);
    return 0;
}
