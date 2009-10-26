/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  DDDD   IIIII   GGGG  EEEEE  SSSSS  TTTTT                   %
%                  D   D    I    G      E      SS       T                     %
%                  D   D    I    G GG   EEE     SSS     T                     %
%                  D   D    I    G   G  E         SS    T                     %
%                  DDDD   IIIII   GGG   EEEEE  SSSSS    T                     %
%                                                                             %
%                                                                             %
%                       Compute Content Message Digest.                       %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                January 2008                                 %
%                                                                             %
%                                                                             %
%  Copyright 1999-2009 ImageMagick Studio LLC, a non-profit organization      %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    http://www.wizards-toolkit.org/script/license.php                        %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

/*
  Include declarations.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "wizard/studio.h"
#include "wizard/WizardsToolkit.h"
#include "wizard/blob-private.h"
#include "wizard/exception-private.h"
#if defined(__WINDOWS__)
#include <windows.h>
#endif
#include "content.h"
#include "utility_.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D i g e s t C o m m a n d                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DigestCommand() reads content from one or more files and generates its
%  message digest as RDF.
%
%  The format of the DigestCommand method is:
%
%      WizardBooleanType DigestCommand(int argc,char **argv,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o argc: The number of elements in the argument vector.
%
%    o argv: A text array containing the command line arguments.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

static void DigestUsage()
{
  static const char
    *options[]=
    {
      "-authenticate        read message digests from a file and authenticate them",
      "-debug events        display copious debugging information",
      "-(de)compress        automagically (de)compress BZIP and ZIP files",
      "-hash type           compute the message digest with this hash",
      "-help                print program options",
      "-list type           print a list of supported option arguments",
      "-version             print version information",
      (char *) NULL
    };

  const char
    **p;

  (void) fprintf(stdout,"Version: %s\n",GetWizardVersion(
    (unsigned long *) NULL));
  (void) fprintf(stdout,"Copyright: %s\n\n",GetWizardCopyright());
  (void) fprintf(stdout,"Usage: %s [options ...] file [ file ...] digest-rdf\n",
    GetClientName());
  (void) fprintf(stdout,"       %s -authenticate [options ...] digest-rdf "
    "[ digest-rdf ...]\n",GetClientName());
  (void) fprintf(stdout,"\nWhere options include:\n");
  for (p=options; *p != (char *) NULL; p++)
    (void) fprintf(stdout,"  %s\n",*p);
  Exit(0);
}

static WizardBooleanType AuthenticateDigest(int argc,char **argv,
  ExceptionInfo *exception)
{
#define DestroyDigest() \
{ \
  for (i=0; i < (long) argc; i++) \
    argv[i]=DestroyString(argv[i]); \
}
#define ThrowDigestException(asperity,tag,context) \
{ \
  (void) ThrowWizardException(exception,GetWizardModule(),asperity,tag, \
    context); \
  DestroyDigest(); \
  return(WizardFalse); \
}

  BlobInfo
    *authenticate_blob,
    *content_blob,
    *digest_blob;

  char
    *create_date,
    date[MaxTextExtent],
    *digest,
    key[MaxTextExtent],
    *message,
    *modify_date,
    *option,
    *options,
    *path,
    *timestamp;

  HashInfo
    *hash_info;

  HashType
    hash;

  int
    c;

  register long
    i;

  size_t
    length;

  ssize_t
    count;

  StringInfo
    *content;

  WizardBooleanType
    compress,
    status;

  status=WizardFalse;
  authenticate_blob=OpenBlob(argv[argc-1],WriteBinaryBlobMode,WizardTrue,
    exception);
  if (authenticate_blob == (BlobInfo *) NULL)
    return(WizardFalse);
  hash=UndefinedHash;
  compress=WizardFalse;
  for (i=1; i < (argc-1); i++)
  {
    option=argv[i];
    if (*option == '-')
      {
        switch(*(option+1))
        {
          case '(':
          {
            if (LocaleCompare(option+1,"(de)compress") == 0)
              {
                compress=(*option == '-') ? WizardTrue : WizardFalse;
                break;
              }
            ThrowDigestException(OptionFatalError,"unrecognized option: `%s'",
              option);
            break;
          }
          case 'a':
          {
            if (strcasecmp(option,"-authenticate") == 0)
              break;
            ThrowDigestException(OptionFatalError,"unrecognized option: `%s'",
              option);
            break;
          }
          case 'd':
          {
            if (strcasecmp(option,"-debug") == 0)
              {
                LogEventType
                  event_mask;

                i++;
                if (i == argc)
                  ThrowDigestException(OptionError,"missing log event mask: "
                    "`%s'",option);
                event_mask=SetLogEventMask(argv[i]);
                if (event_mask == UndefinedEvents)
                  ThrowDigestException(OptionFatalError,"unrecognized log "
                    "event type: `%s'",argv[i]);
                break;
              }
            ThrowDigestException(OptionFatalError,"unrecognized option: `%s'",
              option);
            break;
          }
          case 'l':
          {
            if (LocaleCompare(option,"-list") == 0)
              {
                long
                  list;

                if (*option == '+')
                  break;
                i++;
                if (i == (long) argc)
                  ThrowDigestException(OptionError,"missing list type: `%s'",
                    option);
                if (LocaleCompare(argv[i],"configure") == 0)
                  {
                    (void) ListConfigureInfo((FILE *) NULL,exception);
                    Exit(0);
                  }
                list=ParseWizardOption(WizardListOptions,WizardFalse,argv[i]);
                if (list < 0)
                  ThrowDigestException(OptionFatalError,"unrecognized list "
                    "type: `%s'",argv[i]);
                (void) ListWizardOptions((FILE *) NULL,(WizardOption) list,
                  exception);
                Exit(0);
              }
            if (LocaleCompare("log",option+1) == 0)
              {
                if (*option == '+')
                  break;
                i++;
                if ((i == (long) argc) ||
                    (strchr(argv[i],'%') == (char *) NULL))
                  ThrowDigestException(OptionFatalError,"missing argument: "
                    "`%s'",option);
                break;
              }
            ThrowDigestException(OptionFatalError,"unrecognized option: `%s'",
              option);
            break;
          }
          case 'v':
          {
            if (strcasecmp(option,"-version") == 0)
              {
                (void) fprintf(stdout,"Version: %s\n",
                  GetWizardVersion((unsigned long *) NULL));
                (void) fprintf(stdout,"Copyright: %s\n\n",GetWizardCopyright());
                exit(0);
              }
            ThrowDigestException(OptionFatalError,"unrecognized option: `%s'",
              option);
            break;
          }
          default:
          {
            ThrowDigestException(OptionFatalError,"unrecognized option: `%s'",
              option);
            break;
          }
        }
        continue;
      }
    /*
      Read message digests from a file and verify them.
    */
    digest_blob=OpenBlob(argv[i],ReadBinaryBlobMode,WizardTrue,exception);
    if (digest_blob == (BlobInfo *) NULL)
      return(WizardFalse);
    (void) ResetWizardMemory(key,0,sizeof(key));
    create_date=ConstantString("unknown");
    digest=ConstantString("unknown");
    path=ConstantString("unknown");
    modify_date=ConstantString("unknown");
    timestamp=ConstantString("unknown");
    hash=SHA256Hash;
    for (c=ReadBlobByte(digest_blob); (c != '>') && (c != EOF); )
    {
      length=MaxTextExtent;
      options=AcquireString((char *) NULL);
      while ((isgraph(c) != WizardFalse) && (c != '>') && (c != EOF))
      {
        register char
          *p;
  
        if ((isalnum(c) == WizardFalse) && (c != '/'))
          c=ReadBlobByte(digest_blob);
        else
          {
            /*
              Get the key.
            */
            p=key;
            do
            {
              if (isspace((int) ((unsigned char) c)) != 0)
                break;
              if (c == (int) '>')
                break;
              if ((size_t) (p-key) < MaxTextExtent)
                *p++=(char) c;
              c=ReadBlobByte(digest_blob);
            } while (c != EOF);
            *p='\0';
            p=options;
            while (isspace((int) ((unsigned char) c)) != 0)
              c=ReadBlobByte(digest_blob);
            if (c == (int) '>')
              {
                /*
                  Get the value.
                */
                c=ReadBlobByte(digest_blob);
                while ((c != (int) '<') && (c != EOF))
                {
                  if ((size_t) (p-options+1) >= length)
                    {
                      *p='\0';
                      length<<=1;
                      options=(char *) ResizeQuantumMemory(options,length+
                        MaxTextExtent,sizeof(*options));
                      if (options == (char *) NULL)
                        break;
                      p=options+strlen(options);
                    }
                  if (options == (char *) NULL)
                    {
                      (void) ThrowWizardException(exception,GetWizardModule(),
                        ResourceError,"memory allocation failed: `%s'",
                        GetBlobFilename(digest_blob));
                      return(WizardFalse);
                    }
                  *p++=(char) c;
                  c=ReadBlobByte(digest_blob);
                  if (*options != '<')
                    if (isspace((int) ((unsigned char) c)) != 0)
                      break;
                }
              }
            *p='\0';
            /*
              Interpret key.
            */
            switch (*key)
            {
              case 'd':
              case 'D':
              {
                if (LocaleCompare(key,"digest:create-date") == 0)
                  {
                    StripString(options);
                    create_date=ConstantString(options);
                    break;
                  }
                if (LocaleCompare(key,"digest:modify-date") == 0)
                  {
                    StripString(options);
                    modify_date=ConstantString(options);
                    break;
                  }
                if (LocaleCompare(key,"digest:timestamp") == 0)
                  {
                    StripString(options);
                    timestamp=ConstantString(options);
                    break;
                  }
                if (LocaleNCompare(key,"digest:",7) == 0)
                  {
                    long
                      algorithm;

                    algorithm=ParseWizardOption(WizardHashOptions,WizardFalse,
                      key+7);
                    if (algorithm < 0)
                      break;
                    hash=(HashType) algorithm;
                    StripString(options);
                    digest=ConstantString(options);
                    break;
                  }
                break;
              }
              case 'r':
              case 'R':
              {
                if (LocaleNCompare(key,"rdf:about",9) == 0)
                  {
                    StripString(key+10);
                    if (path != (char *) NULL)
                      path=DestroyString(path);
                    path=ConstantString(key+10);
                    if (digest != (char *) NULL)
                      {
                        digest=DestroyString(digest);
                        digest=ConstantString("unknown");
                      }
                    if (timestamp != (char *) NULL)
                      {
                        timestamp=DestroyString(timestamp);
                        timestamp=ConstantString("timestamp");
                      }
                    if (create_date != (char *) NULL)
                      {
                        create_date=DestroyString(create_date);
                        create_date=ConstantString("unknown");
                      }
                    if (modify_date != (char *) NULL)
                      {
                        modify_date=DestroyString(modify_date);
                        modify_date=ConstantString("unknown");
                      }
                    break;
                  }
                break;
              }
              case '/':
              {
                if ((LocaleCompare(key,"/digest:Content") == 0) ||
                    (LocaleCompare(key,"/rdf:Description") == 0))
                  {
                    content_blob=OpenBlob(path,ReadBinaryBlobMode,WizardTrue,
                      exception);
                    if (content_blob == (BlobInfo *) NULL)
                      break;
                    /*
                      Compute content message digest and verify.
                    */
                    hash_info=AcquireHashInfo(SHA256Hash);
                    InitializeHash(hash_info);
                    for (content=AcquireStringInfo(WizardMaxBufferExtent); ; )
                    {
                      count=ReadBlobChunk(content_blob,WizardMaxBufferExtent,
                        GetStringInfoDatum(content));
                      if (count <= 0)
                        break;
                      length=(size_t) count;
                      SetStringInfoLength(content,length);
                      UpdateHash(hash_info,content);
                    }
                    FinalizeHash(hash_info);
                    content=DestroyStringInfo(content);
                    if (strcmp(digest,GetHashHexDigest(hash_info)) != 0)
                      {
                        message=AcquireString("Path: ");
                        (void) ConcatenateString(&message,path);
                        (void) ConcatenateString(&message,"\n");
                        (void) ConcatenateString(&message,"  create date: ");
                        (void) ConcatenateString(&message,create_date);
                        (void) ConcatenateString(&message,"\n");
                        (void) ConcatenateString(&message,"  modify date: ");
                        (void) ConcatenateString(&message,modify_date);
                        (void) ConcatenateString(&message,"\n");
                        (void) ConcatenateString(&message,"  digest (");
                        (void) ConcatenateString(&message,timestamp);
                        (void) ConcatenateString(&message,"):\n    ");
                        (void) ConcatenateString(&message,digest);
                        (void) ConcatenateString(&message,"\n");
                        (void) ConcatenateString(&message,"  errant digest (");
                        (void) FormatWizardTime(time((time_t *) NULL),
                          MaxTextExtent,date);
                        (void) ConcatenateString(&message,date);
                        (void) ConcatenateString(&message,"):\n    ");
                        (void) ConcatenateString(&message,GetHashHexDigest(
                          hash_info));
                        (void) ConcatenateString(&message,"\n");
                        count=WriteBlobString(authenticate_blob,message);
                        if (count != (ssize_t) strlen(message))
                          ThrowFileException(exception,FileError,argv[argc-1]);
                        message=DestroyString(message);
                      }
                    hash_info=DestroyHashInfo(hash_info);
                    if (CloseBlob(content_blob) != WizardFalse)
                      ThrowFileException(exception,FileError,argv[i]);
                    content_blob=DestroyBlob(content_blob);
                    break;
                  }
              }
              default:
                break;
            }
          }
        while (isspace((int) ((unsigned char) c)) != 0)
          c=ReadBlobByte(digest_blob);
      }
      options=DestroyString(options);
      if (CloseBlob(digest_blob) != WizardFalse)
        ThrowFileException(exception,FileError,argv[i]);
    }
    if (path != (char *) NULL)
      path=DestroyString(path);
    if (digest != (char *) NULL)
      digest=DestroyString(digest);
    if (timestamp != (char *) NULL)
      timestamp=DestroyString(timestamp);
    if (create_date != (char *) NULL)
      create_date=DestroyString(create_date);
    if (modify_date != (char *) NULL)
      modify_date=DestroyString(modify_date);
  }
  (void) CloseBlob(authenticate_blob);
  authenticate_blob=DestroyBlob(authenticate_blob);
  if (hash == UndefinedHash)
    ThrowDigestException(OptionError,"missing a digest RDF file: `%s'",
      argv[argc-1]);
  return(status);
}

WizardExport WizardBooleanType DigestCommand(int argc,char **argv,
  ExceptionInfo *exception)
{
  BlobInfo
    *content_blob,
    *digest_blob;

  char
    algorithm[MaxTextExtent],
    *canonical_path,
    content_extent[MaxTextExtent],
    *digest_rdf,
    *digest,
    *option,
    timestamp[MaxTextExtent];

  const struct stat
    *properties;

  HashInfo
    *hash_info;

  HashType
    hash;

  register long
    i;

  size_t
    length;

  ssize_t
    count;

  StringInfo
    *content;

  WizardBooleanType
    compress,
    status;

  WizardSizeType
    extent;

  /*
    Parse command-line options.
  */
  if (argc == 2)
    {
      option=argv[1];
      if ((LocaleCompare("version",option+1) == 0) ||
          (LocaleCompare("-version",option+1) == 0))
        {
          (void) fprintf(stdout,"Version: %s\n",GetWizardVersion(
            (unsigned long *) NULL));
          (void) fprintf(stdout,"Copyright: %s\n\n",GetWizardCopyright());
          return(WizardTrue);
        }
    }
  if (argc < 3)
    DigestUsage();
  status=ExpandFilenames(&argc,&argv);
  if (status == WizardFalse)
    ThrowDigestException(ResourceError,"memory allocation failed: `%s'",
      strerror(errno));
  for (i=1; i < (argc-1); i++)
  {
    option=argv[i];
    if (LocaleCompare(option+1,"authenticate") == 0)
      {
        status=AuthenticateDigest(argc,argv,exception);
        DestroyDigest();
        return(status);
      }
  }
  digest_blob=OpenBlob(argv[argc-1],WriteBinaryBlobMode,WizardTrue,exception);
  if (digest_blob == (BlobInfo *) NULL)
    return(WizardFalse);
  hash=SHA256Hash;
  (void) WriteBlobString(digest_blob,"<?xml version=\"1.0\"?>\n");
  (void) WriteBlobString(digest_blob,"<rdf:RDF xmlns:rdf=\""
    "http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n");
  (void) WriteBlobString(digest_blob,"         xmlns:digest=\""
     "http://www.wizards-toolkit.org/digest/1.0/\">\n");
  compress=WizardFalse;
  for (i=1; i < (argc-1); i++)
  {
    option=argv[i];
    if (*option == '-')
      {
        switch(*(option+1))
        {
          case '(':
          {
            if (LocaleCompare(option+1,"(de)compress") == 0)
              {
                compress=(*option == '-') ? WizardTrue : WizardFalse;
                break;
              }
            ThrowDigestException(OptionFatalError,"unrecognized option: `%s'",
              option);
            break;
          }
          case 'd':
          {
            if (strcasecmp(option,"-debug") == 0)
              {
                LogEventType
                  event_mask;

                i++;
                if (i == argc)
                  ThrowDigestException(OptionError,"missing log event mask: "
                    "`%s'",option);
                event_mask=SetLogEventMask(argv[i]);
                if (event_mask == UndefinedEvents)
                  ThrowDigestException(OptionFatalError,"unrecognized log "
                    "event type: `%s'",argv[i]);
                break;
              }
            ThrowDigestException(OptionFatalError,"unrecognized option: `%s'",
              option);
            break;
          }
          case 'h':
          {
            if (LocaleCompare("hash",option+1) == 0)
              {
                long
                  type;

                if (*option == '+')
                  break;
                i++;
                if (i == (long) argc)
                  ThrowDigestException(OptionError,"missing hash type: `%s'",
                    option);
                type=ParseWizardOption(WizardHashOptions,WizardFalse,argv[i]);
                if (type < 0)
                  ThrowDigestException(OptionFatalError,"unrecognized hash "
                    "type: `%s'",argv[i]);
                hash=(HashType) type;
                break;
              }
            if ((LocaleCompare("help",option+1) == 0) ||
                (LocaleCompare("-help",option+1) == 0))
              DigestUsage();
            ThrowDigestException(OptionFatalError,"unrecognized option: `%s'",
              option);
            break;
          }
          case 'l':
          {
            if (LocaleCompare("log",option+1) == 0)
              {
                if (*option == '+')
                  break;
                i++;
                if ((i == (long) argc) ||
                    (strchr(argv[i],'%') == (char *) NULL))
                  ThrowDigestException(OptionFatalError,"missing argument: "
                    "`%s'",option);
                break;
              }
            ThrowDigestException(OptionFatalError,"unrecognized option: `%s'",
              option);
            break;
          }
          case 'v':
          {
            if (strcasecmp(option,"-version") == 0)
              {
                (void) fprintf(stdout,"Version: %s\n",
                  GetWizardVersion((unsigned long *) NULL));
                (void) fprintf(stdout,"Copyright: %s\n\n",GetWizardCopyright());
                exit(0);
              }
            ThrowDigestException(OptionFatalError,"unrecognized option: `%s'",
              option);
            break;
          }
          default:
          {
            ThrowDigestException(OptionFatalError,"unrecognized option: `%s'",
              option);
            break;
          }
        }
        continue;
      }
    /*
      Compute message digest for this content.
    */
    content_blob=OpenBlob(argv[i],ReadBinaryBlobMode,compress,exception);
    if (content_blob == (BlobInfo *) NULL)
      continue;
    properties=GetBlobProperties(content_blob);
    hash_info=AcquireHashInfo(SHA256Hash);
    InitializeHash(hash_info);
    extent=0;
    for (content=AcquireStringInfo(WizardMaxBufferExtent); ; )
    {
      count=ReadBlobChunk(content_blob,WizardMaxBufferExtent,GetStringInfoDatum(
        content));
      if (count <= 0)
        break;
      length=(size_t) count;
      SetStringInfoLength(content,length);
      UpdateHash(hash_info,content);
      extent+=length;
    }
    FinalizeHash(hash_info);
    digest_rdf=AcquireString("  <digest:Content rdf:about=\"");
    canonical_path=CanonicalXMLContent(argv[i],WizardFalse);
    (void) ConcatenateString(&digest_rdf,canonical_path);
    canonical_path=DestroyString(canonical_path);
    (void) ConcatenateString(&digest_rdf,"\">\n");
    (void) ConcatenateString(&digest_rdf,"    <digest:timestamp>");
    (void) FormatWizardTime(time((time_t *) NULL),MaxTextExtent,timestamp);
    (void) ConcatenateString(&digest_rdf,timestamp);
    (void) ConcatenateString(&digest_rdf,"</digest:timestamp>\n");
    (void) ConcatenateString(&digest_rdf,"    <digest:modify-date>");
    (void) FormatWizardTime(properties->st_mtime,MaxTextExtent,timestamp);
    (void) ConcatenateString(&digest_rdf,timestamp);
    (void) ConcatenateString(&digest_rdf,"</digest:modify-date>\n");
    (void) ConcatenateString(&digest_rdf,"    <digest:create-date>");
    (void) FormatWizardTime(properties->st_mtime,MaxTextExtent,timestamp);
    (void) ConcatenateString(&digest_rdf,timestamp);
    (void) ConcatenateString(&digest_rdf,"</digest:create-date>\n");
    (void) ConcatenateString(&digest_rdf,"    <digest:extent>");
    (void) FormatWizardString(content_extent,MaxTextExtent,WizardSizeFormat,
      extent);
    (void) ConcatenateString(&digest_rdf,content_extent);
    (void) ConcatenateString(&digest_rdf,"</digest:extent>\n");
    (void) ConcatenateString(&digest_rdf,"    <digest:");
    (void) FormatWizardString(algorithm,MaxTextExtent,"%s",
      WizardOptionToMnemonic(WizardHashOptions,hash));
    LocaleLower(algorithm);
    (void) ConcatenateString(&digest_rdf,algorithm);
    (void) ConcatenateString(&digest_rdf,">");
    digest=GetHashHexDigest(hash_info);
    (void) ConcatenateString(&digest_rdf,digest);
    digest=DestroyString(digest);
    (void) ConcatenateString(&digest_rdf,"</digest:");
    (void) FormatWizardString(algorithm,MaxTextExtent,"%s",
      WizardOptionToMnemonic(WizardHashOptions,hash));
    LocaleLower(algorithm);
    (void) ConcatenateString(&digest_rdf,algorithm);
    (void) ConcatenateString(&digest_rdf,">\n");
    (void) ConcatenateString(&digest_rdf,"  </digest:Content>\n");
    content=DestroyStringInfo(content);
    hash_info=DestroyHashInfo(hash_info);
    if (CloseBlob(content_blob) != WizardFalse)
      ThrowFileException(exception,FileError,argv[i]);
    content_blob=DestroyBlob(content_blob);
    length=strlen(digest_rdf);
    count=WriteBlob(digest_blob,length,(unsigned char *) digest_rdf);
    digest_rdf=DestroyString(digest_rdf);
    if (count != (ssize_t) length)
      ThrowFileException(exception,FileError,argv[argc-1]);
  }
  (void) WriteBlobString(digest_blob,"</rdf:RDF>\n");
  status=CloseBlob(digest_blob);
  /*
    Free resources.
  */
  digest_blob=DestroyBlob(digest_blob);
  DestroyDigest();
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  M a i n                                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/
int main(int argc,char **argv)
{
  char
    *option;

  double
    elapsed_time,
    user_time;

  ExceptionInfo
    *exception;

  register long
    i;

  TimerInfo
    *timer;

  unsigned int
    iterations;

  WizardBooleanType
    regard_warnings,
    status;

  WizardsToolkitGenesis(*argv);
  exception=AcquireExceptionInfo();
  iterations=1;
  status=WizardTrue;
  regard_warnings=WizardFalse;
  for (i=1; i < (long) (argc-1); i++)
  {
    option=argv[i];
    if ((strlen(option) == 1) || ((*option != '-') && (*option != '+')))
      continue;
    if (LocaleCompare("bench",option+1) == 0)
      iterations=(unsigned int) atol(argv[++i]);
    if (LocaleCompare("debug",option+1) == 0)
      (void) SetLogEventMask(argv[++i]);
    if (LocaleCompare("regard-warnings",option+1) == 0)
      regard_warnings=WizardTrue;
  }
  timer=(TimerInfo *) NULL;
  if (iterations > 1)
    timer=AcquireTimerInfo();
  for (i=0; i < (long) iterations; i++)
  {
    status=DigestCommand(argc,argv,exception);
    if ((status == WizardFalse) ||
        (GetExceptionSeverity(exception) != UndefinedException))
      {
        if ((GetExceptionSeverity(exception) < ErrorException) &&
            (regard_warnings == WizardFalse))
          status=WizardTrue;
        CatchException(exception);
      }
  }
  if (iterations > 1)
    {
      elapsed_time=GetElapsedTime(timer);
      user_time=GetUserTime(timer);
      (void) fprintf(stderr,"Performance: %ui %gips %0.3fu %ld:%02ld.%03ld\n",
        iterations,1.0*iterations/elapsed_time,user_time,(long)
        (elapsed_time/60.0+0.5),(long) floor(fmod(elapsed_time,60.0)),
        (long) (1000.0*(elapsed_time-floor(elapsed_time))+0.5));
      timer=DestroyTimerInfo(timer);
    }
  exception=DestroyExceptionInfo(exception);
  WizardsToolkitTerminus();
  return(status == WizardFalse ? 1 : 0);
}