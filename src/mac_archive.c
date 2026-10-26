/* Classic Macintosh archive input for Prince of Persia Remastered.
 * Copyright (C) 2026 The-Alok. GPL-3.0-or-later; see COPYING.
 * No extraction paths from archives are written or executed. Forks are passed
 * in memory to the existing graphics importer. libxad routines and notices
 * are in third_party/libxad/, with a bounded, per-import memory adapter below. */
#include "mac_archive.h"
#include <SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <stdio.h>
#define FORK_LIMIT (64u*1024*1024)
#define TOTAL_LIMIT (256u*1024*1024)
static unsigned a16(const unsigned char *p){return (unsigned)p[0]*256+p[1];}
static uint32_t a32(const unsigned char *p){return (uint32_t)a16(p)*65536+a16(p+2);}
static uint32_t l32(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int range(size_t size,uint64_t at,uint64_t n){return at<=size&&n<=size-at;}
static int error_text(char *error,size_t n,const char *s){if(n)SDL_strlcpy(error,s,n);return 0;}
static uint16_t crc16(const unsigned char *p,size_t n,int ccitt){
 uint16_t c=0;while(n--){if(ccitt){c^=(uint16_t)*p++<<8;for(int j=0;j<8;j++)c=(c<<1)^((c&0x8000)?0x1021:0);}else{c^=*p++;for(int j=0;j<8;j++)c=(c>>1)^((c&1)?0xa001:0);}}return c;
}
/* libxad memory adapter. An abort frees all decoder allocations, including
 * those inside a partially constructed tree. No global decoder state. */
typedef uint8_t xadUINT8;typedef uint16_t xadUINT16;typedef uint32_t xadUINT32;
typedef int8_t xadINT8;typedef int16_t xadINT16;typedef int32_t xadINT32;
typedef void *xadPTR;typedef unsigned char *xadSTRPTR;
typedef struct Allocation {void *p;struct Allocation *next;} Allocation;
struct xadMasterBase {jmp_buf bad;Allocation *allocations;};
struct xadInOut {struct xadMasterBase *xio_xadMasterBase;int xio_Error;unsigned xio_Flags;
 uint32_t xio_CRC32;const unsigned char *in;size_t in_size,in_at,out_size,out_at;
 unsigned char *out;uint64_t bits;unsigned bit_count,padding;size_t operations;
};
#define XADM xadMasterBase,
#define XADMEMF_ANY 0
#define XADMEMF_CLEAR 0
#define XADMEMF_PUBLIC 0
#define XADERR_ILLEGALDATA 1
#define XADERR_NOMEMORY 2
#define XADERR_CHECKSUM 3
#define XADERR_DECRUNCH 4
#define XADIOF_LASTOUTBYTE 1
#define XADIOF_ERROR 2
#define XADIOF_NOCRC16 4
#define XADIOF_NOCRC32 8
#define XADIOF_LASTINBYTE 16
static void sit_abort(struct xadInOut *io){longjmp(io->xio_xadMasterBase->bad,1);}
static void *xadAllocVec(struct xadMasterBase *ctx,size_t n,int flags){
 (void)flags;if(n>TOTAL_LIMIT)longjmp(ctx->bad,1);
 Allocation *a=malloc(sizeof(*a));if(!a)longjmp(ctx->bad,1);
 a->p=calloc(1,n?n:1);if(!a->p){free(a);longjmp(ctx->bad,1);}a->next=ctx->allocations;ctx->allocations=a;return a->p;
}
static void xadFreeObjectA(struct xadMasterBase *ctx,void *p,int ignored){
 (void)ignored;Allocation **a=&ctx->allocations;while(*a){if((*a)->p==p){Allocation *old=*a;*a=old->next;free(old->p);free(old);return;}a=&(*a)->next;}
}
static void xadCopyMem(struct xadMasterBase *ctx,const void *s,void *d,size_t n){(void)ctx;memmove(d,s,n);}
static unsigned xadIOGetChar(struct xadInOut *io){
 if(!io->operations--)sit_abort(io);
 if(io->in_at==io->in_size){if(io->padding){io->padding--;return 0;}sit_abort(io);}
 unsigned c=io->in[io->in_at++];if(io->in_at==io->in_size)io->xio_Flags|=XADIOF_LASTINBYTE;return c;
}
static unsigned xadIOPutChar(struct xadInOut *io,unsigned c){
 if(io->out_at>=io->out_size)sit_abort(io);
 io->out[io->out_at++]=(unsigned char)c;
 if(!(io->xio_Flags&XADIOF_NOCRC32)){io->xio_CRC32^=(unsigned char)c;for(int i=0;i<8;i++)io->xio_CRC32=(io->xio_CRC32>>1)^((io->xio_CRC32&1)?0xedb88320u:0);}
 if(io->out_at==io->out_size)io->xio_Flags|=XADIOF_LASTOUTBYTE;
 return (unsigned char)c;
}
static unsigned xadIOGetBitsHigh(struct xadInOut *io,unsigned n){
 if(n>32)sit_abort(io);
 unsigned v=0;while(n--){if(!io->bit_count){io->bits=xadIOGetChar(io);io->bit_count=8;}v=(v<<1)|((io->bits>>(--io->bit_count))&1);}return v;
}
static unsigned xadIOReadBitsLow(struct xadInOut *io,unsigned n){
 if(n>32)sit_abort(io);
 while(io->bit_count<n){io->bits|=(uint64_t)xadIOGetChar(io)<<io->bit_count;io->bit_count+=8;}return (unsigned)(io->bits&((UINT64_C(1)<<n)-1));
}
static void xadIODropBitsLow(struct xadInOut *io,unsigned n){if(n>io->bit_count)sit_abort(io);io->bits>>=n;io->bit_count-=n;}
static unsigned xadIOGetBitsLow(struct xadInOut *io,unsigned n){unsigned v=xadIOReadBitsLow(io,n);xadIODropBitsLow(io,n);return v;}
static int xadIOWriteBuf(struct xadInOut *io){return io->out_at==io->out_size?0:XADERR_ILLEGALDATA;}
#include "third_party/libxad/stuffit.inc"
#include "third_party/libxad/compress.inc"
static unsigned char *sit_decode(const unsigned char *p,size_t n,size_t length,unsigned method,unsigned checksum){
 if(length>FORK_LIMIT||n>TOTAL_LIMIT)return NULL;
 unsigned char *out=malloc(length?length:1);if(!out)return NULL;
 if(!length){if(n){free(out);return NULL;}return out;}
 struct xadMasterBase *ctx=calloc(1,sizeof(*ctx));if(!ctx){free(out);return NULL;}
 struct xadInOut io={0};io.xio_xadMasterBase=ctx;io.in=p;io.in_size=n;io.out=out;io.out_size=length;
 io.xio_Flags=XADIOF_NOCRC32;io.operations=(length+n)*80+100000;io.padding=method==13?2:0;
 volatile int ok=0;
 if(!setjmp(ctx->bad)){
  int result=0;
  switch(method){case 0:if(n!=length)result=1;else{memcpy(out,p,n);io.out_at=length;}break;
   case 1:result=SIT_rle(&io);break;case 2:result=xadIO_Compress(&io,14|UCOMPBLOCK_MASK);break;
   case 3:result=SIT_huffman(&io);break;case 5:result=SIT_lzah(&io);break;
   case 8:result=SIT_mw(&io);break;case 13:result=SIT_13(&io);break;case 15:result=SIT_Arsenic(&io);break;default:result=1;}
  ok=!result&&io.out_at==length&&(method==15||crc16(out,length,0)==checksum);
 }
 while(ctx->allocations)xadFreeObjectA(ctx,ctx->allocations->p,0);
 free(ctx);
 if(!ok){free(out);return NULL;}return out;
}
static const char hqx_alphabet[]="!\"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr";
int mac_archive_kind(const unsigned char *p,size_t n,const char *name){
 if(n>=22&&(!memcmp(p,"SIT!",4)||!memcmp(p,"StuffIt (c)",11)))return 2;
 if(range(n,32769,5)&&!memcmp(p+32769,"CD001",5))return 3;
 if(n>=128&&p[0]==0&&p[1]>0&&p[1]<64&&p[74]==0&&p[82]==0&&range(n,128,(uint64_t)a32(p+83)+127))return 4;
 const char *ext=strrchr(name?name:"",'.');
 if(ext&&!SDL_strcasecmp(ext,".hqx"))return 1;
 for(size_t i=0;i<SDL_min(n,1024);i++)if(range(n,i,40)&&!memcmp(p+i,"(This file must be converted with BinHex",40))return 1;
 return 0;
}
static int binhex(const unsigned char *p,size_t n,MacArchiveVisitor visit,void *ctx,char *error,size_t error_size){
 size_t start=0,end=n;while(start<n&&p[start]!=':')start++;
 if(start==n||start>n-8){return error_text(error,error_size,"HQX header is missing or truncated. Use a complete BinHex file.");}
 start++;while(end>start&&p[end-1]!=':')end--;if(end<=start)return error_text(error,error_size,"Incomplete HQX file.");end--;
 unsigned char *raw=malloc(TOTAL_LIMIT);if(!raw)return error_text(error,error_size,"Not enough memory for HQX.");
 size_t count=0;unsigned bits=0,held=0;int escape=0,ok=0;
 for(size_t at=start;at<end;at++){
  unsigned c=p[at];if(c=='\r'||c=='\n'||c==' '||c=='\t')continue;
  const char *symbol=c?strchr(hqx_alphabet,(int)c):NULL;if(!symbol)goto bad;
  held=(held<<6)|(unsigned)(symbol-hqx_alphabet);bits+=6;
  if(bits>=8){bits-=8;unsigned b=(held>>bits)&255;
   if(escape){escape=0;if(b==0)b=0x90;else{if(b<2||!count||count+(b-1)>TOTAL_LIMIT)goto bad;memset(raw+count,raw[count-1],b-1);count+=b-1;continue;}}
   else if(b==0x90){escape=1;continue;}
   if(count==TOTAL_LIMIT)goto bad;
   raw[count++]=(unsigned char)b;
  }
 }
 if(escape||count<24||raw[0]<1||raw[0]>63)goto bad;
 size_t header=20+raw[0];if(!range(count,0,header+2)||raw[raw[0]+1]!=0||crc16(raw,header,1)!=a16(raw+header))goto bad;
 size_t data=a32(raw+header-8),resource=a32(raw+header-4);if(data>FORK_LIMIT||resource>FORK_LIMIT||!range(count,header+2,data+resource+4)||header+data+resource+6!=count)goto bad;
 if(crc16(raw+header+2,data,1)!=a16(raw+header+2+data)||crc16(raw+header+data+4,resource,1)!=a16(raw+header+data+resource+4))goto bad;
 char name[64];memcpy(name,raw+1,raw[0]);name[raw[0]]=0;
 ok=visit(name,(const char*)raw+raw[0]+2,raw+header+2,data,raw+header+data+4,resource,ctx);free(raw);return ok;
 bad:free(raw);return error_text(error,error_size,"HQX is incomplete, corrupt, or failed its BinHex checksum.");
}
static int stuffit(const unsigned char *p,size_t n,MacArchiveVisitor visit,void *ctx,char *error,size_t error_size){
 int newer=n>=100&&!memcmp(p,"StuffIt (c)",11);size_t at;
 if(newer){if(p[82]!=5||a32(p+84)!=n)return error_text(error,error_size,"Incomplete or unsupported StuffIt archive.");at=a32(p+88);}
 else {if(n<22||memcmp(p+10,"rLau",4)||a32(p+6)!=n)return error_text(error,error_size,"Incomplete StuffIt archive.");at=22;}
 size_t total=0;unsigned entries=0;
 while(at<n){
  if(++entries>4096)return error_text(error,error_size,"Too many StuffIt entries.");
  size_t header,extra=0,ds,rs,dc,rc;unsigned dm,rm,dcrc,rcrc;char name[256],type[4];int directory;
  if(newer){
   if(!range(n,at,48)||a32(p+at)!=0xa5a5a5a5u)goto bad;
   const unsigned char *h=p+at;header=a16(h+6);size_t nl=a16(h+30),pw=(h[9]&0x40)?0:h[47];directory=h[9]&0x40;
   if(h[9]&0x20)return error_text(error,error_size,"Password-protected StuffIt archives are not supported.");
   if(header<48||header>65536||!range(n,at,header)||nl>255||!range(header,48+pw,nl))goto bad;
   unsigned char *check=malloc(header);if(!check)goto bad;memcpy(check,h,header);check[32]=check[33]=0;unsigned crc=crc16(check,header,0);free(check);if(crc!=a16(h+32))goto bad;
   memcpy(name,h+48+pw,nl);name[nl]=0;
   if(directory&&a32(h+34)==0xffffffffu){at+=header;continue;}
   extra=h[4]==1?36:32;
   if(!range(n,at+header,extra))goto bad;
   const unsigned char *s=h+header;
   if(directory){at+=header+extra;continue;}
   if(s[1])extra+=14;
   if(!range(n,at+header,extra))goto bad;
   memcpy(type,s+4,4);ds=a32(h+34);dc=a32(h+38);dm=h[46];dcrc=a16(h+42);
   rs=s[1]?a32(s+36):0;rc=s[1]?a32(s+40):0;rm=s[1]?s[48]:0;rcrc=s[1]?a16(s+44):0;
  }else{
   header=112;if(!range(n,at,header))goto bad;const unsigned char *h=p+at;
   if(crc16(h,110,0)!=a16(h+110)||h[2]>63)goto bad;
   rm=h[0];dm=h[1];directory=(rm&0x7f)>=32||(dm&0x7f)>=32;
   if((rm|dm)&0x80)return error_text(error,error_size,"Password-protected StuffIt archives are not supported.");
   if(directory){at+=header;continue;}
   memcpy(name,h+3,h[2]);name[h[2]]=0;memcpy(type,h+66,4);
   rs=a32(h+84);ds=a32(h+88);rc=a32(h+92);dc=a32(h+96);rcrc=a16(h+100);dcrc=a16(h+102);
  }
  if(ds>FORK_LIMIT||rs>FORK_LIMIT||ds+rs>TOTAL_LIMIT-total||!range(n,at+header+extra,(uint64_t)rc+dc))goto bad;
  const unsigned char *source=p+at+header+extra;
  unsigned char *data=sit_decode(source+rc,dc,ds,dm,dcrc),*resource=sit_decode(source,rc,rs,rm,rcrc);
  if(!data||!resource){free(data);free(resource);return error_text(error,error_size,"StuffIt decompression/checksum failed, or this compression method is unsupported.");}
  int ok=visit(name,type,data,ds,resource,rs,ctx);free(data);free(resource);if(!ok)return 0;
  total+=ds+rs;at+=header+extra+rc+dc;
 }
 return 1;
 bad:return error_text(error,error_size,"StuffIt archive is incomplete, corrupt, or has invalid entry bounds/checksums.");
}
typedef struct {char name[512];const unsigned char *p;size_t n;} IsoFile;
typedef struct {const unsigned char *p;size_t n;IsoFile files[4096];size_t count;uint32_t dirs[256];size_t dir_count;} Iso;
static int iso_walk(Iso *iso,uint32_t block,uint32_t length,const char *parent,unsigned depth){
 if(depth>12||iso->dir_count==256||!range(iso->n,(uint64_t)block*2048,length))return 0;
 for(size_t i=0;i<iso->dir_count;i++)if(iso->dirs[i]==block)return 0;
 iso->dirs[iso->dir_count++]=block;
 const unsigned char *p=iso->p+(size_t)block*2048;size_t at=0;
 while(at<length){
  unsigned size=p[at];if(!size){at=((at/2048)+1)*2048;continue;}
  if(size<34||!range(length,at,size))return 0;
  const unsigned char *r=p+at;at+=size;
  unsigned nl=r[32];if(33u+nl>size)return 0;if(nl==1&&r[33]<=1)continue;
  if(r[25]&0x80)return 0;/* Multi-extent ISO files require explicit reconstruction. */
  uint32_t extent=l32(r+2),bytes=l32(r+10);if(extent!=a32(r+6)||bytes!=a32(r+14)||!range(iso->n,(uint64_t)extent*2048,bytes))return 0;
  char name[256];if(nl>255)return 0;memcpy(name,r+33,nl);name[nl]=0;
  char *version=strchr(name,';');if(version)*version=0;
  size_t su=33+nl+(!(nl&1));
  while(su+4<=size){unsigned len=r[su+2];if(len<4||su+len>size)return 0;
   if(r[su]=='N'&&r[su+1]=='M'&&len>=5&&r[su+4]==0){memcpy(name,r+su+5,len-5);name[len-5]=0;}su+=len;
  }
  char path[512];int wrote=SDL_snprintf(path,sizeof(path),"%s%s%s",parent,*parent?"/":"",name);if(wrote<0||(size_t)wrote>=sizeof(path))return 0;
  if(r[25]&2){if(!iso_walk(iso,extent,bytes,path,depth+1))return 0;}
  else {if(iso->count==4096)return 0;IsoFile *f=&iso->files[iso->count++];SDL_strlcpy(f->name,path,sizeof(f->name));f->p=iso->p+(size_t)extent*2048;f->n=bytes;}
 }
 return 1;
}
static int apple_double(const unsigned char *p,size_t n,const unsigned char **resource,size_t *length,char type[4]){
 if(n<26||a32(p)!=0x00051607)return 0;
 unsigned count=a16(p+24);if(!range(n,26,(uint64_t)count*12))return 0;
 for(unsigned i=0;i<count;i++){const unsigned char *e=p+26+i*12;unsigned id=a32(e),at=a32(e+4),size=a32(e+8);if(!range(n,at,size))return 0;
  if(id==2){*resource=p+at;*length=size;}else if(id==9&&size>=4)memcpy(type,p+at,4);
 }return 1;
}
static int iso_read(const unsigned char *p,size_t n,MacArchiveVisitor visit,void *ctx,char *error,size_t error_size){
 Iso *iso=calloc(1,sizeof(*iso));if(!iso)return error_text(error,error_size,"Not enough memory for ISO catalog.");iso->p=p;iso->n=n;
 int ok=0;const unsigned char *root=NULL;
 for(unsigned b=16;b<64&&range(n,(uint64_t)b*2048,2048);b++){
  const unsigned char *d=p+(size_t)b*2048;if(memcmp(d+1,"CD001",5)||d[6]!=1)break;if(d[0]==1){root=d+156;break;}if(d[0]==255)break;
 }
 if(!root||root[0]<34||!iso_walk(iso,l32(root+2),l32(root+10),"",0))goto bad;
 for(size_t i=0;i<iso->count;i++){
  IsoFile *f=&iso->files[i];const char *name=strrchr(f->name,'/');name=name?name+1:f->name;if(!strncmp(name,"._",2))continue;
  char sidecar[512];size_t prefix=(size_t)(name-f->name);if(prefix+2+strlen(name)>=sizeof(sidecar))goto bad;
  memcpy(sidecar,f->name,prefix);SDL_snprintf(sidecar+prefix,sizeof(sidecar)-prefix,"._%s",name);
  const unsigned char *resource=NULL;size_t length=0;char type[4]={'?','?','?','?'};
  for(size_t j=0;j<iso->count;j++)if(!SDL_strcasecmp(iso->files[j].name,sidecar)){
   if(!apple_double(iso->files[j].p,iso->files[j].n,&resource,&length,type))goto bad;
   break;
  }
  if(!visit(name,type,f->p,f->n,resource,length,ctx)){free(iso);return 0;}
 }
 ok=1;free(iso);return ok;
 bad:free(iso);return error_text(error,error_size,"ISO9660 catalog is incomplete or unsupported. Use a classic HFS/ISO bridge or an ISO containing the game image or preserved Macintosh forks.");
}
int mac_archive_read(const unsigned char *p,size_t n,const char *name,MacArchiveVisitor visit,void *ctx,char *error,size_t error_size){
 switch(mac_archive_kind(p,n,name)){
  case 1:return binhex(p,n,visit,ctx,error,error_size);
  case 2:return stuffit(p,n,visit,ctx,error,error_size);
  case 3:return iso_read(p,n,visit,ctx,error,error_size);
  case 4:{size_t ds=a32(p+83),rs=a32(p+87),at=128+((ds+127)&~(size_t)127);char file[64];
   if(ds>FORK_LIMIT||rs>FORK_LIMIT||!range(n,128,ds)||!range(n,at,rs))return error_text(error,error_size,"Incomplete MacBinary file.");
   memcpy(file,p+2,p[1]);file[p[1]]=0;return visit(file,(const char*)p+65,p+128,ds,p+at,rs,ctx);}
  default:return error_text(error,error_size,"This file is not a supported Macintosh archive or ISO image.");
 }
}
