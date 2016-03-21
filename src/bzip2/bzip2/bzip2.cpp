// bzip2.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"


//
// Initialize compression scheme
// When used with IIS, InitCompression is called once as soon
// as compression scheme dll is loaded by IIS compression module
//
DLL_EXPORT HRESULT WINAPI InitCompression(
    VOID
    )
{
    return S_OK;
}

//
// Uninitialize compression scheme
// When used with IIS, this method is called before compression
// scheme dll is unloaded by IIS compression module
//
DLL_EXPORT VOID WINAPI DeInitCompression(
    VOID
    )
{
}

//
// Create a new compression context
//
DLL_EXPORT HRESULT WINAPI CreateCompression(
    OUT PVOID *context,
    IN  ULONG reserved
    )
{
    auto stream = new bz_stream();

    stream->bzalloc = NULL;
    stream->bzfree = NULL;
    stream->opaque = NULL;

    BZ2_bzCompressInit(stream, 9, 0, 0);

    *context = stream;

    return S_OK;
}

//
// Compress data
//
DLL_EXPORT HRESULT WINAPI Compress(
    IN  OUT PVOID           context,            // compression context
    IN      CONST BYTE *    input_buffer,       // input buffer
    IN      LONG            input_buffer_size,  // size of input buffer
    IN      PBYTE           output_buffer,      // output buffer
    IN      LONG            output_buffer_size, // size of output buffer
    OUT     PLONG           input_used,         // amount of input buffer used
    OUT     PLONG           output_used,        // amount of output buffer used
    IN      INT             compression_level   // compression level (1...10)
    )
{
    auto stream = reinterpret_cast<bz_stream *>(context);

    stream->next_in = reinterpret_cast<char *>(const_cast<BYTE *>(input_buffer));
    stream->next_out = reinterpret_cast<char *>(output_buffer);

    stream->avail_in = input_buffer_size;
    stream->avail_out = output_buffer_size;

    int action = input_buffer_size == 0 ? BZ_FINISH : BZ_RUN;

    do {
        int ret = BZ2_bzCompress(stream, action);

        if (ret < 0)
        {
            return E_FAIL;
        }

    } while (stream->avail_out == 0);

    *input_used = input_buffer_size - stream->avail_in;
    *output_used = output_buffer_size - stream->avail_out;

    return action == BZ_RUN ? S_OK : S_FALSE;
}

//
// Destroy compression context
//
DLL_EXPORT VOID WINAPI DestroyCompression(
    IN PVOID context
    )
{
    auto stream = reinterpret_cast<bz_stream *>(context);

    BZ2_bzCompressEnd(stream);

    delete stream;
}

//
// Reset compression state
// Required export but not used on Windows Vista and Windows Server 2008 - IIS 7.0
// Deprecated and not required export for Windows 7 and Windows Server 2008 R2 - IIS 7.5
//
DLL_EXPORT HRESULT WINAPI ResetCompression(
    IN OUT PVOID context
    )
{
    return S_OK;
}