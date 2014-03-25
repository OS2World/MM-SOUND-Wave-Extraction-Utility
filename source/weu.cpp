/*
 * $Id: weu.cpp,v 1.5 1999/10/05 07:32:37 Stephane Exp $
 */

#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#include <stdio.h>      // fopen, fread, fclose
#include <stdlib.h>     // max
#include <string.h>     // strcpy, stricmp, strlen
#include <os2.h>
#include "libsndfile\\src\\sndfile.h"


// application definitions
#define WEU_VERSION     "v0.5"
#define WEU_DATE_MONTH  "October 04"
#define WEU_DATE_YEAR   "1999"
#define WEU_EMAIL       "charette@writeme.com"
#define MAX_PATH        1000


// header structure used in WAVE files
typedef struct
{
   BYTE id[4];
   ULONG len;
   USHORT format;
   USHORT channels;
   ULONG sampleRate;
   ULONG averageBytesPerSec;
   USHORT blockAlign;
   USHORT bitsPerSample;
   USHORT cbSize;          // size in bytes of extra header (only if type != 0x0001)
   USHORT samplesPerBlock; // (only if type == 0x0011)
} WAVEFORMATHEADER;


// function prototypes
int main(int argC, char *argV[]);
void printHeader(void);
void findFiles(                                                CHAR *originalFilename,       CHAR *waveFilename, const CHAR *fileSpec, const int conversionType, const int conversionBits, const BOOL bQuiet, const BOOL bVerbose, const BOOL bConvert, const BOOL bOverwrite, const BOOL bRecursive);
void processFile(                                        const CHAR *originalFilename, const CHAR *waveFilename, const ULONG fileSize, const int conversionType, const int conversionBits, const BOOL bQuiet, const BOOL bVerbose, const BOOL bConvert, const BOOL bOverwrite);
void convertFile( const CHAR *buffer, const ULONG index, const CHAR *originalFilename, const CHAR *waveFilename, const ULONG fileSize, const int conversionType, const int conversionBits, const BOOL bQuiet, const BOOL bVerbose, const BOOL bConvert, const BOOL bOverwrite);
ULONG chunkIndex(const CHAR *buffer, const CHAR *name, const ULONG startingIndex);


/*
 * printHeader
 */
void printHeader(void)
{
   printf(  "\n"
            "[1;32mWEU[0m - [1;32mW[0;32mave [1;32mE[0;32mxtraction [1;32mU[0;32mtility[0m\n"
            "[1;34m%s[0m, [1;34m%s[0m, [1;34m%s[0m\n"
            "[1;34mSt‚phane Charette[0m, [1;34m%s[0m\n"
            "\n",
            WEU_VERSION, WEU_DATE_MONTH, WEU_DATE_YEAR, WEU_EMAIL
            );

   return;
}


/*
 * main
 */
int main(int argC, char *argV[])
{
   // setup all of the "global" variables

   int result              = 0;

   // hope we don't have a pathname over  bytes in size...
   CHAR *originalFilename  = new CHAR[MAX_PATH];
   CHAR *waveFilename      = new CHAR[MAX_PATH];

   if(originalFilename == NULL || waveFilename == NULL)
   {
      // why is "new" returning NULL instead of throwing an exception?
      printf("[1;31mError - memory allocation failed during initialization[0m\n");
      return 1;
   }

   int conversionType      = SF_FORMAT_WAV | SF_FORMAT_PCM;
   int conversionBits      = 16;
   BOOL bQuiet             = FALSE;
   BOOL bVerbose           = FALSE;
   BOOL bConvert           = FALSE;
   BOOL bOverwrite         = FALSE;
   BOOL bRecursive         = FALSE;

   // look for any switches
   int argIndex;
   for(argIndex = 1; argIndex < argC; argIndex ++)
   {
      if(argV[argIndex][0] == '/')
      {
         // this is a switch -- look at all characters provided
         int j = 1;
         while(argV[argIndex][j] != '\0')
         {
            switch(argV[argIndex][j])
            {
               case 'q':
               case 'Q':
               {
                  bQuiet = TRUE;
                  bVerbose = FALSE;
                  break;
               }
               case 'v':
               case 'V':
               {
                  bVerbose = TRUE;
                  bQuiet = FALSE;
                  break;
               }
               case 'y':
               case 'Y':
               {
                  bOverwrite = TRUE;
                  break;
               }
               case 'S':
               case 's':
               {
                  bRecursive = TRUE;
                  break;
               }
               case 'C':
               case 'c':
               {
                  int type = argV[argIndex][j+1];
                  if(type >= '0' && type <= '9')
                  {
                     // since we've specified a converstion type, increase
                     // the index into our command-line character counter
                     j++;

                     // set the conversion type
                     switch(type)
                     {
                        case '1':
                        {
                           conversionType = SF_FORMAT_WAV | SF_FORMAT_PCM;
                           conversionBits = 8;
                           break;
                        }
                        case '2':
                        {
                           conversionType = SF_FORMAT_WAV | SF_FORMAT_PCM;
                           conversionBits = 16;
                           break;
                        }
                        case '3':
                        {
                           conversionType = SF_FORMAT_WAV | SF_FORMAT_MS_ADPCM;
                           conversionBits = 16;
                           break;
                        }
                        case '4':
                        {
                           conversionType = SF_FORMAT_AIFF | SF_FORMAT_PCM;
                           conversionBits = 8;
                           break;
                        }
                        case '5':
                        {
                           conversionType = SF_FORMAT_AIFF | SF_FORMAT_PCM;
                           conversionBits = 16;
                           break;
                        }
                        case '6':
                        {
                           conversionType = SF_FORMAT_AU | SF_FORMAT_PCM;
                           conversionBits = 8;
                           break;
                        }
                        case '7':
                        {
                           conversionType = SF_FORMAT_AU | SF_FORMAT_PCM;
                           conversionBits = 16;
                           break;
                        }
                        case '8':
                        {
                           conversionType = SF_FORMAT_AULE | SF_FORMAT_PCM;
                           conversionBits = 8;
                           break;
                        }
                        case '9':
                        {
                           conversionType = SF_FORMAT_AULE | SF_FORMAT_PCM;
                           conversionBits = 16;
                           break;
                        }
                        default:
                        {
                           printf("[1;31mError - invalid conversion format switch [37m/c%c [31mwas specified[0m\n", type);
                           break;
                        }
                     }
                  }
                  bConvert = TRUE;
                  break;
                }
               default:
               {
                  printf("[1;31mError - invalid switch [37m%c [31mwas specified in [37m%s[0m\n", argV[argIndex][j], argV[argIndex]);
                  break;
               }
            }
            // get the next character in this argument
            j ++;
         }
      }
      else
      {
         // the first non-switch will be considered the start of filenames;
         // no switches can be specified halfway through the list of filenames
         break;
      }
   }

   // if we forgot to specify a filename...
   if(argIndex >= argC)
   {
      // ...then print the usage information
      printHeader();
      printf(  "Usage:\n"
               "\t[1;36mwea.exe [30m[[36m/q[1;30m|[36m/v[30m] [30m[[36m/y[30m] [[36m/c[30m[[36m1[30m|[36m2[30m|...[36m9[30m]] [[36m/s[30m] [36mfile1 [30m<[36mfile2[30m> [30m<[36mfile...[30m>[0m\n"
               "where\n"
               "\t[1;36m/q [0;36mis used to specify \"quiet mode\"; output only errors\n"
               "\t   (both the options [1;36m/q[0;36m and [1;36m/v[0;36m are mutually exclusive)\n"
               "\t[1;36m/v [0;36mis used to specify verbosity; output everything possible\n"
               "\t   (both the options [1;36m/q[0;36m and [1;36m/v[0;36m are mutually exclusive)\n"
               "\t[1;36m/y [0;36moverwrite output .WAV files if they already exist\n"
               "\t[1;36m/c [0;36mis used to convert from extended WAVE format to a default\n"
               "\t   PCM format that can be played in OS/2\n"
               "\t   - [1;36m/c [0;36mand [1;36m/c2 [0;36m(they are the same) both convert to 16-bit WAV-PCM\n"
               "\t   - [1;36m/c1 [0;36m: to  8-bit WAV-PCM"  "\t- [1;36m/c3 [0;36m: to 16-bit WAV-ADPCM\n"
               "\t   - [1;36m/c4 [0;36m: to  8-bit AIFF-PCM" "\t- [1;36m/c5 [0;36m: to 16-bit AIFF-PCM\n"
               "\t   - [1;36m/c6 [0;36m: to  8-bit AU-PCM"   "\t- [1;36m/c7 [0;36m: to 16-bit AU-PCM\n"
               "\t   - [1;36m/c8 [0;36m: to  8-bit AULE-PCM" "\t- [1;36m/c9 [0;36m: to 16-bit AULE-PCM\n"
               "\t[1;36m/s [0;36mis used to enable recursive searches through subdirectories\n"
               "\t[1;36mfile [0;36mis a file or list of files that should be searched\n"
               "\t   to obtain the WAVE information; wildcards accepted\n"
               "\n"
               "[0mOutput files will automatically have [1;37m.WAV[0m extensions appended, even if the [1;36m/c[0;36m# [0m\n"
               "option would normally dictate the use of [1;37m.AIF [0mor [1;37m.AU[0m.  It is up to the user to\n"
               "rename the files if necessary.\n"
               "\n"
               "[1;30mOn my OS/2 system, the 16-bit WAVE-PCM conversion format seems to be the only\n"
               "one I can use to convert Kodak digital camera image files to OS/2-compatible\n"
               "sound files.[0m\n"
               );

      return 2;
   }

   if(!bQuiet)
   {
      printHeader();
   }

   // every argument left in the argV list should now be filenames that must be dealt with
   for(; argIndex < argC; argIndex ++)
   {
      findFiles(originalFilename, waveFilename, argV[argIndex], conversionType, conversionBits, bQuiet, bVerbose, bConvert, bOverwrite, bRecursive);
   }

   delete [] originalFilename;
   delete [] waveFilename;

   return result;
}


/*
 * findFiles
 */
void findFiles(CHAR *originalFilename        ,
               CHAR *waveFilename            ,
               const CHAR *fileSpec          ,
               const int conversionType      ,
               const int conversionBits      ,
               const BOOL bQuiet             ,
               const BOOL bVerbose           ,
               const BOOL bConvert           ,
               const BOOL bOverwrite         ,
               const BOOL bRecursive         )
{
   HDIR hdir               = HDIR_CREATE;
   FILEFINDBUF3 findBuffer = {0};
   ULONG entries           = 1;
   APIRET rc;

   // start looking for files matching the criteria given on the command-line
   rc = DosFindFirst(fileSpec                      ,
                     &hdir                         ,
                     FILE_ARCHIVED | FILE_READONLY ,
                     &findBuffer                   ,
                     sizeof(findBuffer)            ,
                     &entries                      ,
                     FIL_STANDARD                  );

   // if we didn't even find 1 file...
   if(!bRecursive && rc == ERROR_NO_MORE_FILES)
   {
      // ...then let the user know
      printf("[1;31mError - cannot find the file %s (rc=%i, 0x%04x)[0m\n", fileSpec, rc, rc);
   }

   char *lastSlash = max(strrchr(fileSpec, '\\'), strrchr(fileSpec, '/'));
   int indexAfterLastSlash = 0;
   if(lastSlash)
   {
      indexAfterLastSlash = lastSlash - fileSpec + 1;
   }
   strcpy(originalFilename, fileSpec);
   strcpy(waveFilename, fileSpec);

   // as long as we have more files that match this filespec...
   while(!rc)
   {
      // ...then use the original pathname...
      originalFilename[indexAfterLastSlash]  = '\0';
      waveFilename[indexAfterLastSlash]      = '\0';

      // ...and append the filename taken from DosFindFirst/DosFindNext...
      strcat(originalFilename,   findBuffer.achName);
      strcat(waveFilename,       findBuffer.achName);

      // ...and put the .WAV extension on the wavefile...
      char *lastDot = strrchr(waveFilename, '.');
      if(lastDot)
      {
         lastDot[0] = '\0';
      }
      strcat(waveFilename, ".WAV");

      // ...make certain the two filenames don't match...
      if(stricmp(originalFilename, waveFilename) == 0)
      {
         // ...tell the user we cannot use the same file for input and output!
         printf("[1;31mError - the file [37m%s[31m cannot be both input and output[0m\n", originalFilename);
      }
      else
      {
         // ...otherwise process this file...
         processFile(originalFilename  ,
                     waveFilename      ,
                     findBuffer.cbFile ,
                     conversionType    ,
                     conversionBits    ,
                     bQuiet            ,
                     bVerbose          ,
                     bConvert          ,
                     bOverwrite        );
      }

      // ...before moving on to the next
      rc = DosFindNext(hdir, &findBuffer, sizeof(findBuffer), &entries);
   }

   // close the directory handle
   DosFindClose(hdir);

   if(rc != ERROR_NO_MORE_FILES)
   {
      printf("[1;31mError - cannot find the file %s (rc=%i, 0x%04x)[0m\n", fileSpec, rc, rc);
   }

   // see if we need to check subdirectories
   if(bRecursive)
   {
      originalFilename[indexAfterLastSlash]  = '\0';

      if(originalFilename[0] == '\0')
      {
         strcpy(originalFilename, ".\\");
         indexAfterLastSlash = 2;
      }

      // start looking for directories
      strcat(originalFilename, "*");
      hdir               = HDIR_CREATE;
      entries            = 1;
      rc = DosFindFirst(originalFilename              ,
                        &hdir                         ,
                        MUST_HAVE_DIRECTORY  |
                        FILE_ARCHIVED        |
                        FILE_DIRECTORY       |
                        FILE_READONLY                 ,
                        &findBuffer                   ,
                        sizeof(findBuffer)            ,
                        &entries                      ,
                        FIL_STANDARD                  );

      if(!rc)
      {
         // hope we don't have a pathname over MAX_PATH bytes in size...
         CHAR *newOriginalFilename = new CHAR[MAX_PATH];
         CHAR *newFileSpec = new CHAR[MAX_PATH];

         while(!rc)
         {
            // skip the "." and ".." directories
            if(newOriginalFilename              != NULL  &&
               newFileSpec                      != NULL  &&
               strcmp(findBuffer.achName, ".")  != 0     &&
               strcmp(findBuffer.achName, "..") != 0     )
            {
               // ..so copy the current path...
               strcpy(newFileSpec, originalFilename);
               if(indexAfterLastSlash)
               {
                  newFileSpec[indexAfterLastSlash] = '\0';
               }

               // ...and append the new direcotry to this path...
               strcat(newFileSpec, findBuffer.achName);

               // ...followed by a slash (since this is a diretory)...
               strcat(newFileSpec, "\\");

               // ...and then the original filespec...
               if(lastSlash)
               {
                  strcat(newFileSpec, &lastSlash[1]);
               }
               else
               {
                  strcat(newFileSpec, fileSpec);
               }

               // ...watch all this recursive stuff...
               if(strlen(newFileSpec) > MAX_PATH - 100)
               {
                  printf("[1;31mWarning - pathname close to exceeding maximum length[0m\n");
                  if(!bVerbose)
                     printf("[1;30m-> [1;31mPath: [0m%s\n", newFileSpec);
               }

               // ...before going off and finding some new files!
               if(bVerbose)
                  printf("[1;30m-> [0;37mlooking for [1;37m%s[0m\n", newFileSpec);

               findFiles(  newOriginalFilename  ,
                           waveFilename         ,
                           newFileSpec          ,
                           conversionType       ,
                           conversionBits       ,
                           bQuiet               ,
                           bVerbose             ,
                           bConvert             ,
                           bOverwrite           ,
                           bRecursive           );
            }

            // move on to the next subdirectory
            rc = DosFindNext(hdir, &findBuffer, sizeof(findBuffer), &entries);
         }

         delete [] newOriginalFilename;
         delete [] newFileSpec;
      }

      DosFindClose(hdir);
   }

   return;
}


/*
 * processFile
 */
void processFile( const CHAR *originalFilename  ,
                  const CHAR *waveFilename      ,
                  const ULONG fileSize          ,
                  const int conversionType      ,
                  const int conversionBits      ,
                  const BOOL bQuiet             ,
                  const BOOL bVerbose           ,
                  const BOOL bConvert           ,
                  const BOOL bOverwrite         )
{
   if(bVerbose)
      printf("\n[1;30m-> [0;37mopening [1;37m%s [0;37mto read [1;37m%i bytes[0m\n", originalFilename, fileSize);
   else if(!bQuiet)
      printf("\n[1;30m-> [0;37mprocessing [1;37m%s[0;37m\n", originalFilename);

   // check to see if the output file already exists when /y wasn't specified
   if(!bOverwrite)
   {
      BOOL fileAlreadyExists = FALSE;

      // ...then the file shouldn't exist...
      FILE *waveFile = fopen(waveFilename, "rb");

      if(waveFile)
      {
         fileAlreadyExists = TRUE;
         fclose(waveFile);
      }

      if(fileAlreadyExists)
      {
         if(!bQuiet)
            printf("[1;30m-> [0;37moutput file [1;37m%s [0;37malready exists; skipping file[0m\n", waveFilename);
         return;
      }
   }

   FILE *file = fopen(originalFilename, "rb");

   if(!file)
   {
      printf("[1;31mError - cannot open the file %s[0m\n", originalFilename);
      return;
   }

   CHAR *buffer = new CHAR[fileSize];
   if(!buffer)
   {
       // why is "new" returning NULL instead of throwing an exception?
       printf("[1;31mError - memory allocation failed[0m\n");
       return;
   }

   fread(buffer, 1, fileSize, file);
   fclose(file);

   if(bVerbose)
      printf("[1;30m-> [0;37mlooking for WAVE[0m");
   fflush(NULL);

   // if the WAVE is found, then index will indicate the
   // very first character that indicates the start of
   // the WAVE file, i.e., the 'R' in "RIFF"
   ULONG index = 0;

   WAVEFORMATHEADER *wave = (WAVEFORMATHEADER*)0;

   while(index < fileSize)
   {
      if(buffer[index] == 'R')
      {
         if(memcmp("RIFF", &buffer[index], 4) == 0)
         {
            if(memcmp("WAVE", &buffer[index+8], 4) == 0)
            {
               // we've found the WAVE file -- extract it out

               // find the length of the WAVE file
               ULONG *waveLen = (ULONG*)&buffer[index+4];

               // find the fmt chunk
               ULONG fmtIndex = chunkIndex(buffer, "fmt ", index);

               if(fmtIndex)
               {
                  wave = (WAVEFORMATHEADER*)&buffer[fmtIndex];
               }
               else
               {
                  printf("[1;37mError - failed to find the \"fmt\" chunk in the WAVE file[0m\n");
               }

               if(fmtIndex && bVerbose)
               {
                  printf("\n[1;30m-> [0;37mfound WAVE at offset [1;37m0x%04x[0;37m, [1;37m%i bytes[0m\n", index, *waveLen);

                  // see if we can print any information on this sound file
                  if(memcmp("fmt ", &buffer[index+12], 4) == 0)
                  {
                     // the format tag should always be specified before the
                     // data, but nothing prevents other tags from being
                     // specified at the beginning of the file -- don't assume
                     // that "fmt " will be first in line (from "Multimedia
                     // Programming Interface and Data Specifications, v1.0,
                     // August 1991, IBM and MS)
                     printf("[1;30m-> [0;37mfound WAVE format tag:\n");

                     printf("\t[1;30m-> [0;37mformat length is [1;37m%i bytes\n", wave->len);

                     CHAR text[80] = "unknown format";

                     switch(wave->format)
                     {
                        case 0x0000:
                        {
                           strcpy(text, "WAVE_FORMAT_UNKNOWN");
                           break;
                        }
                        case 0x0001:
                        {
                           strcpy(text, "WAVE_FORMAT_PCM, MS Pulse Code Modulation");
                           break;
                        }
                        case 0x0002:
                        {
                           strcpy(text, "WAVE_FORMAT_ADPCM, Adaptive Differential PCM");
                           break;
                        }
                        case 0x0005:
                        {
                           strcpy(text, "WAVE_FORMAT_IBM_CVSD");
                           break;
                        }
                        case 0x0006:
                        {
                           strcpy(text, "WAVE_FORMAT_ALAW, IBM a-law format");
                           break;
                        }
                        case 0x0007:
                        {
                           strcpy(text, "WAVE_FORMAT_MULAW, IBM mu-law format");
                           break;
                        }
                        case 0x0010:
                        {
                           strcpy(text, "WAVE_FORMAT_OKI_ADPCM");
                           break;
                        }
                        case 0x0011:
                        {
                           strcpy(text, "WAVE_FORMAT_DVI_ADPCM, Intel DVI/IMA");
                           break;
                        }
                        case 0x0014:
                        {
                           strcpy(text, "WAVE_FORMAT_G723_ADPCM");
                           break;
                        }
                        case 0x0015:
                        {
                           strcpy(text, "WAVE_FORMAT_DIGISTD");
                           break;
                        }
                        case 0x0016:
                        {
                           strcpy(text, "WAVE_FORMAT_DIGIFIX");
                           break;
                        }
                        case 0x0020:
                        {
                           strcpy(text, "WAVE_FORMAT_YAMAHA_ADPCM");
                           break;
                        }
                        case 0x0021:
                        {
                           strcpy(text, "WAVE_FORMAT_SONARC");
                           break;
                        }
                        case 0x0022:
                        {
                           strcpy(text, "WAVE_FORMAT_DSPGROUP_TRUESPEECH");
                           break;
                        }
                        case 0x0025:
                        {
                           strcpy(text, "WAVE_FORMAT_APTX");
                           break;
                        }
                        case 0x0033:
                        {
                           strcpy(text, "WAVE_FORMAT_ANTEX_ADPCME");
                           break;
                        }
                        case 0x0040:
                        {
                           strcpy(text, "WAVE_FORMAT_G721_ADPCM");
                           break;
                        }
                        case 0x0200:
                        {
                           strcpy(text, "WAVE_FORMAT_CREATIVE_ADPCM");
                           break;
                        }
                     }

                     printf("\t[1;30m-> [0;37mformat type is [1;37m%i[0m, [1;37m0x0%02x [0m([0;36m%s[0m)\n", wave->format, wave->format, text);

                     printf("\t[1;30m-> [0;37mnumber of channels is [1;37m%i[0m\n", wave->channels);

                     printf("\t[1;30m-> [0;37msample rate is [1;37m%i[0m\n", wave->sampleRate);

                     printf("\t[1;30m-> [0;37maverage bytes per second is [1;37m%i[0m\n", wave->averageBytesPerSec);

                     printf("\t[1;30m-> [0;37mblock alignment is [1;37m%i[0m\n", wave->blockAlign);

                     printf("\t[1;30m-> [0;37mbits per sample is [1;37m%i[0m\n", wave->bitsPerSample);

                     // if the format is greater than normal PCM format...
                     if(wave->format > 0x0001)
                     {
                        // ...display the number of bytes in the extended header
                        printf("\t[1;30m-> [0;37mextended format contains [1;37m%i bytes[0m\n", wave->cbSize);

                        // if the format is in Intel's DVI/IMA format...
                        if(wave->format == 0x0011)
                        {
                           // ...display the number of samples per block
                           if(wave->cbSize < 2)
                           {
                              printf("[1;31mWarning - cannot determine the number of samples per block[0m\n");
                           }
                           else
                           {
                              printf("\t[1;30m-> [0;37msamples per block is [1;37m%i[0m\n", wave->samplesPerBlock);
                           }
                        }
                        else if(wave->len - sizeof(WAVEFORMATHEADER) + 8 != 0)
                        {
                           printf("\t[1;30m-> [0;37manother [1;37m%i[0;37m bytes of unknown header information remain[0m\n", wave->len - sizeof(WAVEFORMATHEADER) + 8 );
                        }
                     }
                  }
               }

               if(!bQuiet)
                  printf("[1;30m-> [0;37mcreating [1;37m%s[0m\n", waveFilename);

               FILE *waveFile = fopen(waveFilename, "wb");

               if(!waveFile)
               {
                  printf("[1;31mError - cannot open the output file [1;37m%s[0m\n", waveFilename);
               }
               else if(fwrite(&buffer[index], 1, *waveLen + 8, waveFile) != *waveLen + 8)
               {
                  printf("[1;31mError - cannot write to file[0m\n");
               }

               if(bVerbose)
                  printf("[1;30m-> [0;37mclosing [1;37m%s[0m\n", waveFilename);
               fclose(waveFile);

               // stop looking for WAVE components in this file
               break;
            }
         }
      }
      // we haven't found the WAVE yet -- keep looking
      index ++;
      if(bVerbose && (index % (fileSize/32) == 0))
      {
         // print a mark
         printf(".");
         fflush(NULL);
      }
   }

   if(!bQuiet && index == fileSize)
   {
      if(bVerbose)
         printf("\n");
      printf("[1;30m-> [0;37mno WAVE file was found[0m\n");
   }

   // do we have a file _and_ need to convert it?
   if(index != fileSize && bConvert)
   {
      convertFile(&buffer[index], 0, originalFilename, waveFilename, fileSize, conversionType, conversionBits, bQuiet, bVerbose, bConvert, bOverwrite);
   }

   delete [] buffer;

   if(bVerbose && index != fileSize)
      printf("[1;30m-> [0;37mdone![0m\n");

   return;

}


/*
 * convertFile
 */
void convertFile( const CHAR *buffer            ,
                  const ULONG index             ,
                  const CHAR *originalFilename  ,
                  const CHAR *waveFilename      ,
                  const ULONG fileSize          ,
                  const int conversionType      ,
                  const int conversionBits      ,
                  const BOOL bQuiet             ,
                  const BOOL bVerbose           ,
                  const BOOL bConvert           ,
                  const BOOL bOverwrite         )
{
   if(!bQuiet)
   {
      printf("[1;30m-> [0;37mconverting file to [36m");
      switch(conversionBits)
      {
         case 8:
         {
            printf("8-bit ");
            break;
         }
         case 16:
         {
             printf("16-bit ");
             break;
         }
      }
      switch(conversionType)
      {
         case (SF_FORMAT_WAV | SF_FORMAT_PCM):
         {
            printf("WAVE-PCM");
            break;
         }
         case (SF_FORMAT_WAV | SF_FORMAT_MS_ADPCM):
         {
            printf("WAVE-ADPCM");
            break;
         }
         case (SF_FORMAT_AIFF | SF_FORMAT_PCM):
         {
            printf("AIFF-PCM");
            break;
         }
         case (SF_FORMAT_AU | SF_FORMAT_PCM):
         {
            printf("AU-PCM");
            break;
         }
         case (SF_FORMAT_AULE | SF_FORMAT_PCM):
         {
            printf("AULE-PCM");
            break;
         }
         default:
         {
            printf("UNKNOWN");
            break;
         }
      }
      printf(" [0mformat[0;37m\n");
   }

   ULONG fmtIndex = chunkIndex(buffer, "fmt ", index);
   ULONG factIndex = chunkIndex(buffer, "fact", index);
   ULONG dataIndex = chunkIndex(buffer, "data", index);

   if(fmtIndex == 0)
   {
      printf("[1;31mWarning - \"fmt\" chunk was not found[0m\n");
   }
   else if(bVerbose)
      printf("[1;30m-> [0;37m\'fmt \' chunk starts at offset [1;37m0x%04x[0m\n", fmtIndex);

   if(factIndex == 0)
   {
      printf("[1;31mWarning - \"fact\" chunk was not found[0m\n");
   }
   else if(bVerbose)
      printf("[1;30m-> [0;37m\'fact\' chunk starts at offset [1;37m0x%04x[0m\n", factIndex);

   if(dataIndex == 0)
   {
      printf("[1;31mWarning - \"data\" chunk was not found[0m\n");
   } else if(bVerbose)
      printf("[1;30m-> [0;37m\'data\' chunk starts at offset [1;37m0x%04x[0m\n", dataIndex);

   if(bVerbose)
   {
      WAVEFORMATHEADER *wave = (WAVEFORMATHEADER*)&buffer[fmtIndex];
      ULONG *compressedSoundDataLen = (ULONG*)(&buffer[dataIndex + 4]);
      printf("[1;30m-> [0;37mcompressed sound data length is [1;37m%i bytes[0m\n", *compressedSoundDataLen);
   }

   /************************************************************************
    *                                                                      *
    *    Thank goodness for free software!  The LIBSNDFILE libraries       *
    *    written by Erik de Castro Lopo <mailto:erikd@zip.com.au> will     *
    *    take care of converting the sound file to standard PCM format.    *
    *    Please see his pages at <www.zip.com.au/~erikd/libsndfile/>       *
    *    for additional information.                                       *
    *    Stephane Charette, charette@writeme.com, 1999.09.23               *
    *                                                                      *
    ************************************************************************/

   int *ptr          = (int*)0;
   size_t itemsRead  = (size_t)0;
   SF_INFO oldInfo   = {0};
   SF_INFO newInfo   = {0};
   SNDFILE *oldFile  = (SNDFILE*)0;
   SNDFILE *newFile  = (SNDFILE*)0;

   // open the input file
   oldFile = sf_open_read(waveFilename, &oldInfo);

   if(!oldFile)
   {
      printf("[1;31mError - cannot open input file for conversion[0m\n");
   }
   else
   {
      if(bVerbose)
         printf("[1;30m-> [0;37muncompressed sound data length is [1;37m%i bytes[0m\n", oldInfo.samples * oldInfo.channels * oldInfo.pcmbitwidth / 8);

      // now create a buffer to hold the sound data
      ptr = new int[oldInfo.samples * oldInfo.channels];

      if(!ptr)
      {
         printf("[1;31mError - cannot allocate memory to read data[0m\n");
      }
      else
      {
         // initialize the buffer
         for(int i = 0; i < oldInfo.samples * oldInfo.channels; i++)
         {
            ptr[i] = 0;
         }

         // read the file into the buffer
         itemsRead = sf_read_int(oldFile, ptr, oldInfo.samples * oldInfo.channels);

         if(itemsRead < 1)
         {
            printf("[1;31mError - cannot read the input file for conversion[0m\n");
         }
         else
         {
            // prepare to write the output file
            newInfo.samplerate   = oldInfo.samplerate;
            newInfo.samples      = oldInfo.samples;
            newInfo.channels     = oldInfo.channels;
            newInfo.pcmbitwidth  = conversionBits?conversionBits:oldInfo.pcmbitwidth;
            newInfo.format       = conversionType?conversionType:(SF_FORMAT_WAV | SF_FORMAT_PCM);
         }
      }
   }

   // close the input file since we're done with it
   if(oldFile)
   {
      sf_close(oldFile);
   }

   // were any items read?
   if(itemsRead > 0)
   {
      // open the file for output!
      if(!bQuiet)
         printf("[1;30m-> [0;37mwriting converted WAVE file to [1m%s[0m\n", waveFilename);

      newFile = sf_open_write(waveFilename, &newInfo);

      if(!newFile)
      {
         printf("[1;31mError - cannot open output file for conversion[0m\n");
      }
      else
      {
         // write the sound bytes into the output file
         size_t itemsWritten = sf_write_int(newFile, ptr, itemsRead);

         if(itemsWritten != itemsRead)
         {
            printf("[1;31mError - failed to write converted file[0m\n");
         }

         // close the output file
         sf_close(newFile);
      }
   }

   // free the memory buffer
   delete [] ptr;

   return;
}


/*
 * chunkIndex
 */
ULONG chunkIndex(const CHAR *buffer, const CHAR *name, const ULONG startingIndex)
{
   ULONG index = 0;

   // return the index value of the "name" chunk within a RIFF WAVE file,
   // or return the value 0 if the chunk was not found

   // first, ensure that the chunk name is 4 chars long
   if(strlen(name) == 4)
   {
      // then ensure that this is a RIFF file
      if(memcmp(&buffer[startingIndex], "RIFF", 4) == 0)
      {
         ULONG *fileSize = (ULONG*)(&buffer[4]);

         // now ensure that we're dealing with a WAVE file
         if(memcmp(&buffer[startingIndex+8], "WAVE", 4) == 0)
         {
            // ok -- this is the first chunk of a WAVE file; start looking!
            index = startingIndex + 12;

            while(index)
            {
               if(memcmp(&buffer[index], name, 4) == 0)
               {
                  // we've found the RIFF chunk we need!
                  break;
               }

               // otherwise, keep looking
               ULONG *chunkSize = (ULONG*)(&buffer[index + 4]);
               index += 8 + *chunkSize;

               // did we go past the end of the file?
               if(index >= *fileSize)
               {
                  // we never did find the chunk -- return empty-handed
                  index = 0;
                  break;
               }
            }
         }
      }
   }

   return index;
}

