#include <stdio.h>
#include <stdlib.h>
#define STB_DEFINE
#include "stb.h"

#define Assert(Value) do { if (!(Value)) *(int *)0 = 0; } while (0);
#define ArrayCount(Array) sizeof(Array)/sizeof(Array[0])

inline bool 
StrContains(char *String, char Character)
{
    while (*String)
    {
        if (*String++ == Character)
        {
            return true;
        }
    }
    return false;
}

inline bool
StrEqual(char *String1, char *String2)
{
    while (*String1 && *String2)
    {
        if (*String1++ != *String2++)
            return false;
    }
    
    return *String1 == *String2;
}

inline size_t
StrLen(char *String)
{
    size_t Result = 0;
    while (*String)
    {
        ++Result;
        ++String;
    }
    return Result;
}

inline char *
StrClone(char *String)
{
    char *Result = (char *)calloc(StrLen(String) + 1, sizeof(char));
    
    char *Walker = Result;
    while (*String)
    {
        *Walker++ = *String++;
    }
    
    return Result;
}

// TODO(Chen): FINISH THIS AND REFINE THE FUCKING RETURN TYPE PARSING
inline bool
StrEndsWith(char *String, char *Ending)
{
    size_t StringLength = StrLen(String);
    size_t EndingLength = StrLen(Ending);
    
    if (StringLength > EndingLength)
    {
        int StartingIndex = StringLength - EndingLength;
        for (int I = 0; I < EndingLength; ++I)
        {
            if (String[StartingIndex+I] != Ending[I])
            {
                return false;
            }
        }
        return true;
    }
    
    return false;
}

inline char
ToUpper(char Character)
{
    if (Character >= 'a' && Character <= 'z')
    {
        return Character + ('A' - 'a');
    }
    return Character;
}

inline char *
ReadEntireFile(char *Filename)
{
    char *Result = 0;
    
    FILE *FileHandle = fopen(Filename, "rb");
    if (FileHandle)
    {
        fseek(FileHandle, 0, SEEK_END);
        size_t FileSize = ftell(FileHandle);
        rewind(FileHandle);
        
        Result = (char *)calloc(FileSize + 1, sizeof(char));
        size_t BytesRead = fread(Result, sizeof(char), FileSize, FileHandle);
        
        if (BytesRead != FileSize)
        {
            free(Result);
            printf("failed to read %s with fread()\n", Filename);
            return 0;
        }
        
        fclose(FileHandle);
    }
    
    return Result;
}

inline bool
WriteFile(char *Filename, char *Content)
{
    FILE *FileHandle = fopen(Filename, "wb");
    if (FileHandle)
    {
        size_t FileSize = StrLen(Content);
        size_t ByteWritten = fwrite(Content, sizeof(char), FileSize, FileHandle);
        
        if (ByteWritten == FileSize)
        {
            fclose(FileHandle);
            return true;
        }
        else
        {
            fclose(FileHandle);
            printf("failed to load all content from %s\n", Filename);
            return false;
        }
    }
    
    return false;
}

/*

a hand-written loader would look like:

typedef [RETURN_TYPE]__stdcall [FUNC_PROC_NAME]([ARGS]);
.
.
.
.
.

[FUNC_PROC_NAME] [FUNC_NAME];
.
.
.
.
.

typedef void *function_loader(char *FunctionName);
void LoadGLFunction(function_loader *Loader)
{
[FUNC_NAME] = Loader([FUNC_NAME]);
.
.
.
.
.
}

*/

static char *IgnoredChars = " \n\r\t";

struct parser
{
    char *Source;
    char Token[255];
    size_t Cursor;
};

inline char
Char(parser *Parser)
{
    return Parser->Source[Parser->Cursor];
}

inline void
GetNextToken(parser *Parser)
{
    while (Char(Parser) && StrContains(IgnoredChars, Char(Parser)))
    {
        Parser->Cursor += 1;
    }
    
    if (Char(Parser))
    {
        int TokenIndex = 0;
        while (Char(Parser) && !StrContains(IgnoredChars, Char(Parser)))
        {
            Assert(TokenIndex < ArrayCount(Parser->Token));
            Parser->Token[TokenIndex++] = Char(Parser);
            Parser->Cursor += 1;
        }
        Parser->Token[TokenIndex] = 0;
    }
    else
    {
        // NOTE(Chen): nullify the token of parser
        Parser->Token[0] = 0;
    }
}


struct function
{
    char *ReturnType;
    char *Name;
    char *ArgList;
};

static function *
GetFunctions(char *Source)
{
    function *FunctionList = 0;
    
    parser Parser = {};
    Parser.Source = Source;
    
    while (Char(&Parser))
    {
        GetNextToken(&Parser);
        if (StrEqual(Parser.Token, "GLAPI"))
        {
            // load up that function
            {
                function NewFunction = {};
                
                //parse return type
                {
                    char *LeadingString = 0;
                    bool FirstIt = true;
                    do
                    {
                        if (!FirstIt)
                        {
                            stb_arr_pop(LeadingString);
                        }
                        else
                        {
                            FirstIt = false;
                        }
                        
                        stb_arr_push(LeadingString, Char(&Parser));
                        stb_arr_push(LeadingString, 0);
                        Parser.Cursor++;
                    } while (!StrEndsWith(LeadingString, "APIENTRY"));
                    
                    LeadingString[StrLen(LeadingString) - StrLen("APIENTRY") - 1] = 0;
                    NewFunction.ReturnType = StrClone(LeadingString);
                    
                    stb_arr_free(LeadingString);
                }
                
                GetNextToken(&Parser);
                NewFunction.Name = StrClone(Parser.Token);
                
                while (Char(&Parser) != ';')
                {
                    stb_arr_push(NewFunction.ArgList, Char(&Parser));
                    ++Parser.Source;
                }
                stb_arr_push(NewFunction.ArgList, 0);
                
                stb_arr_push(FunctionList, NewFunction);
            }
        }
        else
        {
            //go to next line
            while (Char(&Parser) && Char(&Parser) != '\n')
            {
                Parser.Cursor += 1;
            }
        }
    }
    
    return FunctionList;
}

#define StrAppend(String, Appendex) for (int J = 0; J < StrLen(Appendex); ++J) stb_arr_push(String, Appendex[J]); 
#define StrAppendUpper(String, Appendex) for (int J = 0; J < StrLen(Appendex); ++J) stb_arr_push(String, ToUpper(Appendex[J])); 
static char *
Output(function *Functions)
{
    char *Typedef = "typedef ";
    char *StdCall = "__stdcall ";
    char *OutputFile = 0;
    
    //declare all functions
    for (int I = 0; I < stb_arr_len(Functions); ++I)
    {
        function Function = Functions[I];
        
        //typedef
        StrAppend(OutputFile, Typedef);
        StrAppend(OutputFile, Function.ReturnType);
        stb_arr_push(OutputFile, ' ');
        StrAppend(OutputFile, StdCall);
        StrAppendUpper(OutputFile, Function.Name);
        StrAppend(OutputFile, Function.ArgList);
        stb_arr_push(OutputFile, ';');
        stb_arr_push(OutputFile, '\n');
        
        //function pointer
        StrAppendUpper(OutputFile, Function.Name);
        stb_arr_push(OutputFile, ' ');
        stb_arr_push(OutputFile, '*');
        StrAppend(OutputFile, Function.Name);
        stb_arr_push(OutputFile, ';');
        stb_arr_push(OutputFile, '\n');
    }
    
    char *LoaderFunctionDecl = "typedef void *load_function(char *FunctionName);\n";
    char *LoaderDecl = "void LoadGLFunctions(load_function *LoadFunction) {\n";
    char *LoadFunction = "LoadFunction";
    char *Tab        = "   ";
    stb_arr_push(OutputFile, '\n');
    StrAppend(OutputFile, LoaderFunctionDecl);
    StrAppend(OutputFile, LoaderDecl);
    
    //loader
    for (int I = 0; I < stb_arr_len(Functions); ++I)
    {
        //lvalue
        StrAppend(OutputFile, Tab);
        StrAppend(OutputFile, Functions[I].Name);
        stb_arr_push(OutputFile, ' ');
        stb_arr_push(OutputFile, '=');
        stb_arr_push(OutputFile, ' ');
        
        //cast
        stb_arr_push(OutputFile, '(');
        StrAppendUpper(OutputFile, Functions[I].Name);
        stb_arr_push(OutputFile, '*');
        stb_arr_push(OutputFile, ')');
        
        //rvalue
        StrAppend(OutputFile, LoadFunction);
        stb_arr_push(OutputFile, '(');
        stb_arr_push(OutputFile, '"');
        StrAppend(OutputFile, Functions[I].Name);
        stb_arr_push(OutputFile, '"');
        stb_arr_push(OutputFile, ')');
        stb_arr_push(OutputFile, ';');
        stb_arr_push(OutputFile, '\n');
    }
    
    stb_arr_push(OutputFile, '}');
    stb_arr_push(OutputFile, '\n');
    return OutputFile;
}

int main(int ArgumentCount, char **Arguments)
{
    if (ArgumentCount == 3)
    {
        char *InputFilename =  Arguments[1];
        char *OutputFilename = Arguments[2];
        
        char *OpenglHeader = ReadEntireFile(InputFilename);
        if (OpenglHeader)
        {
            function *FunctionList = GetFunctions(OpenglHeader);
            char *OutputString = Output(FunctionList);
            WriteFile(OutputFilename, OutputString);
        }
        else
        {
            printf("failed to load file %s\n", InputFilename);
        }
    }
    else
    {
        printf("use it like this: %s [input file] [output file]", Arguments[0]);
    }
    
    getchar();
    return 0;
}
