#ifndef INCLUDED_IMPLODER_H
#define INCLUDED_IMPLODER_H

#include <godlib/base/base.h>

#define dIMPLODER_HEADER_SIZE 12
#define dIMPLODER_TRAILER_SIZE 50

/* Header queries require at least 12 readable bytes. Only IMP! data is accepted. */
U8  Imploder_IsPacked( const void * apData );
U32 Imploder_GetDepackSize( const void * apData );
U32 Imploder_GetPackedSize( const void * apData );

/* Destination: even-aligned, unpacked-size bytes. Source may equal destination;
   otherwise the buffers must not overlap. Separate source remains unchanged.
   Returns 1 on success. Trusted assets only: the ASM trusts the token stream. */
U8 Imploder_Depack( const void * apSrc, void * apDst );
/* Also checks actual input length, output capacity and the original word-sum
   before modifying dst. Length is the complete IMP! file, including checksum. */
U8 Imploder_DepackChecked( const void * apSrc, U32 aSrcSize,
                         void * apDst, U32 aDstSize );

#endif
