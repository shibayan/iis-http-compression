// brotli.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"


typedef struct
{
    brotli::BrotliCompressor *compressor;
    brotli::BrotliParams params;

    size_t brotli_ring;
    uint8_t *brotli_out;
    uint8_t *brotli_last;

    bool last;
} iis_brotli_context;

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
    *context = new iis_brotli_context();

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
    auto ctx = reinterpret_cast<iis_brotli_context *>(context);

    // create compressor
    if (ctx->compressor == nullptr)
    {
        ctx->params.quality = compression_level;

        ctx->compressor = new brotli::BrotliCompressor(ctx->params);
        ctx->brotli_ring = ctx->compressor->input_block_size();
    }

    // Copy remain output buffer
    if (ctx->brotli_out < ctx->brotli_last)
    {
        size_t size = std::min((size_t)output_buffer_size, (size_t)(ctx->brotli_last - ctx->brotli_out));

        memcpy(output_buffer, ctx->brotli_out, size);

        ctx->brotli_out += size;

        *input_used = 0;
        *output_used = size;

        return S_OK;
    }

    if (ctx->last)
    {
        return S_FALSE;
    }

    ctx->brotli_out = nullptr;
    ctx->brotli_last = nullptr;

    size_t input_size = std::min((size_t)input_buffer_size, ctx->brotli_ring);

    if (input_size != 0)
    {
        // Copy to Brotli ring buffer
        ctx->compressor->CopyInputToRingBuffer(input_size, input_buffer);
    }
    else
    {
        // no input buffer = finish
        ctx->last = true;
    }

    size_t output_size;

    // Write Brotli
    if (!ctx->compressor->WriteBrotliData(ctx->last, true, &output_size, &ctx->brotli_out))
    {
        return E_FAIL;
    }

    // end of iterator
    ctx->brotli_last = ctx->brotli_out + output_size;

    output_size = std::min((size_t)output_buffer_size, output_size);

    // copy to output_buffer
    memcpy(output_buffer, ctx->brotli_out, output_size);

    // forward pointer
    ctx->brotli_out += output_size;

    *input_used = input_size;
    *output_used = output_size;

    return S_OK;
}

//
// Destroy compression context
//
DLL_EXPORT VOID WINAPI DestroyCompression(
    IN PVOID context
    )
{
    auto ctx = reinterpret_cast<iis_brotli_context *>(context);

    delete ctx->compressor;
    delete ctx;
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