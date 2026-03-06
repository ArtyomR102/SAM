#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "reciter.h"
#include "sam.h"
#include "render.h"
#include "debug.h"

#ifdef USESDL
#include <SDL.h>
#include <SDL_audio.h>
#endif

unsigned char ParseFormants(char* str) {
    char* buf = strdup(str);
    if (!buf) return 0;
    
    char* tok = strtok(buf, ",");
    unsigned char form[3];
    unsigned char counter = 0;
        
    while (tok != NULL) {
        if      (!strcmp(tok, "none"))      form[counter] = 0;
        else if (!strcmp(tok, "sine"))      form[counter] = 1;
        else if (!strcmp(tok, "triangle"))  form[counter] = 2;
        else if (!strcmp(tok, "sawtooth"))  form[counter] = 3;
        else if (!strcmp(tok, "square"))    form[counter] = 4;
        else if (!strcmp(tok, "rect25"))    form[counter] = 5;
        else if (!strcmp(tok, "rect125"))   form[counter] = 6;
        else if (!strcmp(tok, "nes_tri"))   form[counter] = 7;
        else if (!strcmp(tok, "vrc6_saw"))  form[counter] = 8;
        else if (!strcmp(tok, "half_sine")) form[counter] = 9;
        else if (!strcmp(tok, "noise"))     form[counter] = 10;
        else {
            free(buf);
            return 0;
        }
        
        counter += 1;
        tok = strtok(NULL, ",");
    }
    
    free(buf);
    switch (counter) {
        case 1: form[1] = form[0];
        case 2: form[2] = form[1]; break;
        case 3: break;
        default: return 0;
    }
    
    SetFormants(form[0], form[1], form[2]);
    return 1;
}


void WriteWav(char* filename, char* buffer, int bufferlength)
{
    FILE *file = fopen(filename, "wb");
    if (file == NULL) return;
    //RIFF header
    fwrite("RIFF", 4, 1,file);
    unsigned int filesize=bufferlength + 12 + 16 + 8 - 8;
    fwrite(&filesize, 4, 1, file);
    fwrite("WAVE", 4, 1, file);

    //format chunk
    fwrite("fmt ", 4, 1, file);
    unsigned int fmtlength = 16;
    fwrite(&fmtlength, 4, 1, file);
    unsigned short int format=1; //PCM
    fwrite(&format, 2, 1, file);
    unsigned short int channels=1;
    fwrite(&channels, 2, 1, file);
    unsigned int samplerate = 22050;
    fwrite(&samplerate, 4, 1, file);
    fwrite(&samplerate, 4, 1, file); // bytes/second
    unsigned short int blockalign = 1;
    fwrite(&blockalign, 2, 1, file);
    unsigned short int bitspersample=8;
    fwrite(&bitspersample, 2, 1, file);

    //data chunk
    fwrite("data", 4, 1, file);
    fwrite(&bufferlength, 4, 1, file);
    fwrite(buffer, bufferlength, 1, file);

    fclose(file);
}

void PrintUsage()
{
    printf("usage: sam [options] [Word1 Word2 ....]\n");
    printf("options\n");
    printf("    -phonetic               enters phonetic mode. (see below)\n");
    printf("    -sing                   special treatment of pitch\n");
    printf("    -pitch number           set pitch value (default=64)\n");
    printf("    -speed number           set speed value (default=72)\n");
    printf("    -throat number          set throat value (default=128)\n");
    printf("    -mouth number           set mouth value (default=128)\n");
    printf("    -formant waveform(s)    set formant(s) shape (default=sine,sine,square)\n");
    printf("    -wav filename           output to wav instead of libsdl\n");
    printf("    -pipe                   output to stdout instead of libsdl\n");
    printf("    -debug                  print additional debug messages\n");
    printf("NOTE: If no words are present, stdin is used\n");
    printf("\n");

    printf("     VOWELS                            VOICED CONSONANTS    \n");
    printf("IY           f(ee)t                    R        red        \n");
    printf("IH           p(i)n                     L        allow        \n");
    printf("EH           beg                       W        away        \n");
    printf("AE           Sam                       W        whale        \n");
    printf("AA           pot                       Y        you        \n");
    printf("AH           b(u)dget                  M        Sam        \n");
    printf("AO           t(al)k                    N        man        \n");
    printf("OH           cone                      NX       so(ng)        \n");
    printf("UH           book                      B        bad        \n");
    printf("UX           l(oo)t                    D        dog        \n");
    printf("ER           bird                      G        again        \n");
    printf("AX           gall(o)n                  J        judge        \n");
    printf("IX           dig(i)t                   Z        zoo        \n");
    printf("                       ZH       plea(s)ure    \n");
    printf("   DIPHTHONGS                          V        seven        \n");
    printf("EY           m(a)de                    DH       (th)en        \n");
    printf("AY           h(igh)                        \n");
    printf("OY           boy                        \n");
    printf("AW           h(ow)                     UNVOICED CONSONANTS    \n");
    printf("OW           slow                      S         Sam        \n");
    printf("UW           crew                      Sh        fish        \n");
    printf("                                       F         fish        \n");
    printf("                                       TH        thin        \n");
    printf(" SPECIAL PHONEMES                      P         poke        \n");
    printf("UL           sett(le) (=AXL)           T         talk        \n");
    printf("UM           astron(omy) (=AXM)        K         cake        \n");
    printf("UN           functi(on) (=AXN)         CH        speech        \n");
    printf("Q            kitt-en (glottal stop)    /H        a(h)ead    \n");
    printf("\n");
    printf(" SUPPORTED FORMANT SHAPES\n");
    printf("sine        Sine wave\n");
    printf("triangle    Triangle wave\n");
    printf("sawtooth    Sawtooth wave\n");
    printf("square      50%% width square wave\n");
    printf("rect25      25%% width square wave\n");
    printf("rect125     12.5%% width square wave\n");
    printf("nes_tri     NES triangle wave\n");
    printf("vrc6_saw    VRC6 sawtooth wave\n");
    printf("half_sine   abs(sin()) wave\n");
    printf("noise       (not implemented yet!)\n");
}

#ifdef USESDL

int pos = 0;
void MixAudio(void *unused, Uint8 *stream, int len)
{
    int bufferpos = GetBufferLength();
    char *buffer = GetBuffer();
    int i;
    if (pos >= bufferpos) return;
    if ((bufferpos-pos) < len) len = (bufferpos-pos);
    
    for(i=0; i<len; i++)
    {
        stream[i] = buffer[pos];
        pos++;
    }
}

void OutputSound()
{
    int bufferpos = GetBufferLength();
    bufferpos /= 50;
    SDL_AudioSpec fmt;

    fmt.freq = 22050;
    fmt.format = AUDIO_U8;
    fmt.channels = 1;
    fmt.samples = 2048;
    fmt.callback = MixAudio;
    fmt.userdata = NULL;

    /* Open the audio device and start playing sound! */
    if ( SDL_OpenAudio(&fmt, NULL) < 0 )
    {
        printf("Unable to open audio: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_PauseAudio(0);
    //SDL_Delay((bufferpos)/7);

    while (pos < bufferpos)
    {
        SDL_Delay(100);
    }

    SDL_CloseAudio();
}

#else

void OutputSound() {}

#endif

int debug = 0;

int main(int argc, char** argv)
{
    int i;
    int phonetic = 0;

    char* wavfilename = NULL;
    char input[256];

    for(i=0; i<256; i++) input[i] = 0;

    if (argc <= 1)
    {
        PrintUsage();
        return 1;
    }
    
    SetFormants(1, 1, 4); // Default
    
    i = 1;
    while(i < argc)
    {
        if (argv[i][0] != '-')
        {
            strncat(input, argv[i], 255);
            strncat(input, " ", 255);
        } else
        {
            if (strcmp(&argv[i][1], "wav")==0)
            {
                wavfilename = argv[i+1];
                i++;
            } else
            if (strcmp(&argv[i][1], "sing")==0)
            {
                EnableSingmode();
            } else
            if (strcmp(&argv[i][1], "phonetic")==0)
            {
                phonetic = 1;
            } else
            if (strcmp(&argv[i][1], "debug")==0)
            {
                debug = 1;
            } else
            if (strcmp(&argv[i][1], "pitch")==0)
            {
                SetPitch(atoi(argv[i+1]));
                i++;
            } else
            if (strcmp(&argv[i][1], "speed")==0)
            {
                SetSpeed(atoi(argv[i+1]));
                i++;
            } else
            if (strcmp(&argv[i][1], "mouth")==0)
            {
                SetMouth(atoi(argv[i+1]));
                i++;
            } else
            if (strcmp(&argv[i][1], "formant")==0)
            {
                if (!ParseFormants(argv[i+1]))
                    printf("Unable to parse formants; using default values");
                i++;
            } else
            if (strcmp(&argv[i][1], "throat")==0)
            {
                SetThroat(atoi(argv[i+1]));
                i++;
            } else
            {
                PrintUsage();
                return 1;
            }
        }

        i++;
    } //while

    for(i=0; input[i] != 0; i++)
        input[i] = toupper((int)input[i]);

    if (debug)
    {
        if (phonetic) printf("phonetic input: %s\n", input);
        else printf("text input: %s\n", input);
    }

    if (!phonetic)
    {
        strncat(input, "[", 256);
        if (!TextToPhonemes((unsigned char *)input)) return 1;
        if (debug)
            printf("phonetic input: %s\n", input);
    } else strncat(input, "\x9b", 256);

#ifdef USESDL
    if ( SDL_Init(SDL_INIT_AUDIO) < 0 )
    {
        printf("Unable to init SDL: %s\n", SDL_GetError());
        exit(1);
    }
    atexit(SDL_Quit);
#endif

    SetInput(input);
    if (!SAMMain())
    {
        PrintUsage();
        return 1;
    }

    if (wavfilename != NULL)
        WriteWav(wavfilename, GetBuffer(), GetBufferLength()/50);
    else
        OutputSound();


    return 0;

}
