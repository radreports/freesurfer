#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "mri.h"
#include "mri_identify.h"
#include "analyze.h"
#include "volume_io.h"
#include "machine.h"
#include "fio.h"

int is_brik(char *fname)
{

  FILE *fp;
  char header_name[STRLEN];
  char *dot;

  strcpy(header_name, fname);
  if((dot = strrchr(header_name, '.')) == NULL)
    return(0);

  dot++;
  if(strcmp(dot, "BRIK"))
    return(0);

  sprintf(dot, "HEAD");

  if((fp = fopen(fname, "r")) == NULL)
    return(0);

  fclose(fp);

  return(1);

} /* end is_brik() */

int is_siemens(char *fname)
{

  FILE *fp;
  char string[4];

  if((fp = fopen(fname, "r")) == NULL)
    return(0);

  fseek(fp, 5790, SEEK_SET);
  fread(string, 2, 1, fp);
  string[2] = '\0';
  if(strcmp(string, "SL"))
    {
    fclose(fp);
    return(0);
    }
  fseek(fp, 5802, SEEK_SET);
  fread(string, 2, 1, fp);
  string[2] = '\0';
  if(strcmp(string, "SP"))
    {
    fclose(fp);
    return(0);
    }
  fseek(fp, 5838, SEEK_SET);
  fread(string, 3, 1, fp);
  string[3] = '\0';
  if(strcmp(string, "FoV"))
    {
    fclose(fp);
    return(0);
    }


  fclose(fp);

  return(1);

} /* end is_siemens() */

int is_ge(char *fname)
{

  FILE *fp;
  long magic;

  if((fp = fopen(fname, "r")) == NULL)
    return(0);

  fread(&magic, 4, 1, fp);
  magic = orderLongBytes(magic);

  fclose(fp);

  if(magic == GE_MAGIC)
    return(1);

  return(0);

}  /*  end is_ge()  */

int is_new_ge(char *fname)
{

  FILE *fp;
  long magic;

  if((fp = fopen(fname, "r")) == NULL)
    return(0);

  fseek(fp, 3228, SEEK_CUR);
  fread(&magic, 4, 1, fp);
  magic = orderLongBytes(magic);

  fclose(fp);

  if(magic == GE_MAGIC)
    return(1);

  return(0);

}  /*  end is_new_ge()  */

int is_analyze(char *fname)
{

  FILE *fp;
  dsr hdr;
  char hfname[STRLEN], *dot;
  long hdr_length;

  strcpy(hfname, fname);
  if((dot = strrchr(hfname, '.')) != NULL)
    *dot = '\0';

  sprintf(hfname, "%s.hdr", hfname);

  if((fp = fopen(hfname, "r")) == NULL)
    return(0);

  if(fread(&hdr, sizeof(hdr), 1, fp) < 1)
  {
    fclose(fp);
    return(0);
  }

  fseek(fp, 0, SEEK_END);
  hdr_length = ftell(fp);
  fclose(fp);

  if(hdr_length != orderIntBytes(hdr.hk.sizeof_hdr))
    return(0);
  if(orderIntBytes(hdr.hk.extents) != 16384)
    return(0);
  if(hdr.hk.regular != 'r')
    return(0);

  return(1);

}  /*  end is_analyze()  */

int is_mnc(char *fname)
{

  char buf[3];
  FILE *fp;

  if((fp = fopen(fname, "r")) == NULL)
    return(0);

  fread(buf, 1, 3, fp);

  fclose(fp);

  if(strncmp(buf, "CDF", 3) == 0)
    return(1);

  return(0);

}  /*  end is_mnc()  */

int is_mgh(char *fname)
{

  FILE *fp;
  int version, width, height, depth, nframes, type, dof;

  if((fp = fopen(fname, "rb")) == NULL)
    return(0);

  version = freadInt(fp) ;
  width = freadInt(fp) ;
  height = freadInt(fp) ;
  depth =  freadInt(fp) ;
  nframes = freadInt(fp) ;
  type = freadInt(fp) ;
  dof = freadInt(fp) ;

  fclose(fp);

/* my estimates (ch) */
  if(width < 10 || height < 10 || width >= 1024 || height >= 1024)
    return(0);
  if(depth > 20000)
    return(0);
  if(nframes > 10000)
    return(0);

  return(1);

}  /*  end is_mgh()  */

/* EOF */
