/*
 * Blp.cc
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013 hugehardzhang <hugehardzhang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL INTEL AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "Image.h"
#include "Resize.h"

#ifndef HAVE_BLP
#include <sstream>
#include <vector>
#include <stdlib.h>
#include <string.h>

#include <setjmp.h>
#include <jpeglib.h>

typedef std::vector<uint8_t> buffer;
const int MAX_NR_OF_BLP_MIP_MAPS = 16;

struct rgba
{
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
};

struct BLP_HEADER
{
    BLP_HEADER()
    {
        MagicNumber = '1PLB';
        Compression = 0;
        AlphaBits = 0;
        Width = 0;
        Height = 0;
        Unknown1 = 0;
        Unknown2 = 0;
        memset(Offset, 0, MAX_NR_OF_BLP_MIP_MAPS * sizeof(uint32_t));
        memset(Size, 0, MAX_NR_OF_BLP_MIP_MAPS * sizeof(uint32_t));
    }

    uint32_t MagicNumber;
    uint32_t Compression;
    uint32_t AlphaBits;
    uint32_t Width;
    uint32_t Height;
    uint32_t Unknown1;
    uint32_t Unknown2;
    uint32_t Offset[MAX_NR_OF_BLP_MIP_MAPS];
    uint32_t Size[MAX_NR_OF_BLP_MIP_MAPS];
};

struct JPEG_SOURCE_MANAGER
{
	JPEG_SOURCE_MANAGER()
	{
		input = NULL;
		inputSize = 0;
		Buffer = NULL;
	}

	jpeg_source_mgr Manager;
	const JOCTET* input;
	size_t inputSize;
	const JOCTET* Buffer;
};

struct JPEG_DESTINATION_MANAGER
{
    JPEG_DESTINATION_MANAGER()
    {
        DestinationBuffer = NULL;
        DestinationBufferSize = 0;
        Buffer = NULL;
    }

    jpeg_destination_mgr Manager;
    JOCTET *DestinationBuffer;
    size_t DestinationBufferSize;
    JOCTET *Buffer;
};

void DestinationInit(jpeg_compress_struct *Info)
{
    JPEG_DESTINATION_MANAGER *DestinationManager;

    DestinationManager = reinterpret_cast<JPEG_DESTINATION_MANAGER *>(Info->dest);

    DestinationManager->Buffer = DestinationManager->DestinationBuffer;
    DestinationManager->Manager.next_output_byte = DestinationManager->Buffer;
    DestinationManager->Manager.free_in_buffer = DestinationManager->DestinationBufferSize;
}

boolean DestinationEmpty(jpeg_compress_struct *Info)
{
    JPEG_DESTINATION_MANAGER *DestinationManager;

    DestinationManager = reinterpret_cast<JPEG_DESTINATION_MANAGER *>(Info->dest);

    DestinationManager->Manager.next_output_byte = DestinationManager->Buffer;
    DestinationManager->Manager.free_in_buffer = DestinationManager->DestinationBufferSize;

    return true;
}

void DestinationTerminate(jpeg_compress_struct * /*Info*/)
{
    // Empty
}

void SourceInit(jpeg_decompress_struct* /*Info*/)
{
	//Empty
}

boolean SourceFill(jpeg_decompress_struct* Info)
{
	JPEG_SOURCE_MANAGER* SourceManager;

	SourceManager = reinterpret_cast<JPEG_SOURCE_MANAGER*>(Info->src);

	SourceManager->Buffer = SourceManager->input;
	SourceManager->Manager.next_input_byte = SourceManager->Buffer;
	SourceManager->Manager.bytes_in_buffer = SourceManager->inputSize;

	return true;
}

void SourceSkip(jpeg_decompress_struct* Info, long NrOfBytes)
{
	JPEG_SOURCE_MANAGER* SourceManager;

	SourceManager = reinterpret_cast<JPEG_SOURCE_MANAGER*>(Info->src);

	if(NrOfBytes > 0)
	{
		while(NrOfBytes > static_cast<long>(SourceManager->Manager.bytes_in_buffer))
		{
			NrOfBytes -= static_cast<long>(SourceManager->Manager.bytes_in_buffer);
			SourceFill(Info);
		}

		SourceManager->Manager.next_input_byte += NrOfBytes;
		SourceManager->Manager.bytes_in_buffer -= NrOfBytes;
	}
}

void SourceTerminate(jpeg_decompress_struct* /*Info*/)
{
	//Empty
}

void SetMemorySource(jpeg_decompress_struct* Info, const JOCTET* Buffer, size_t Size)
{
	JPEG_SOURCE_MANAGER* SourceManager;

	Info->src = reinterpret_cast<jpeg_source_mgr*>((*Info->mem->alloc_small)(reinterpret_cast<j_common_ptr>(Info), JPOOL_PERMANENT, sizeof(JPEG_SOURCE_MANAGER)));
	SourceManager = reinterpret_cast<JPEG_SOURCE_MANAGER*>(Info->src);

	SourceManager->Buffer = reinterpret_cast<JOCTET*>((*Info->mem->alloc_small)(reinterpret_cast<j_common_ptr>(Info), JPOOL_PERMANENT, Size * sizeof(JOCTET)));
	SourceManager->input = Buffer;
	SourceManager->inputSize = Size;
	SourceManager->Manager.init_source = SourceInit;
	SourceManager->Manager.fill_input_buffer = SourceFill;
	SourceManager->Manager.skip_input_data = SourceSkip;
	SourceManager->Manager.resync_to_restart = jpeg_resync_to_restart;
	SourceManager->Manager.term_source = SourceTerminate;
	SourceManager->Manager.bytes_in_buffer = 0;
	SourceManager->Manager.next_input_byte = NULL;
}

void SetMemoryDestination(jpeg_compress_struct *Info, JOCTET *Buffer, size_t Size)
{
    JPEG_DESTINATION_MANAGER *DestinationManager;

    Info->dest = reinterpret_cast<jpeg_destination_mgr *>((*Info->mem->alloc_small)(reinterpret_cast<j_common_ptr>(Info), JPOOL_PERMANENT, sizeof(JPEG_DESTINATION_MANAGER)));
    DestinationManager = reinterpret_cast<JPEG_DESTINATION_MANAGER *>(Info->dest);

    DestinationManager->Buffer = NULL;
    DestinationManager->DestinationBuffer = Buffer;
    DestinationManager->DestinationBufferSize = Size;
    DestinationManager->Manager.init_destination = DestinationInit;
    DestinationManager->Manager.empty_output_buffer = DestinationEmpty;
    DestinationManager->Manager.term_destination = DestinationTerminate;
}

bool writeToJpeg(PixelArray *input, buffer &output, int Width, int Height, int Quality)
{
    int RealSize;
    int DummySize;
    int line;
    buffer TempBuffer;
    JSAMPROW Pointer[1];
    jpeg_compress_struct Info;
    jpeg_error_mgr ErrorManager;

    Info.err = jpeg_std_error(&ErrorManager);

    DummySize = ((Width * Height * 4) * 2) + 10000;
    TempBuffer.resize(DummySize);

    jpeg_create_compress(&Info);

    SetMemoryDestination(&Info, TempBuffer.data(), TempBuffer.size());

    Info.image_width = Width;
    Info.image_height = Height;
    Info.input_components = 4;
    Info.in_color_space = JCS_UNKNOWN;

    jpeg_set_defaults(&Info);
    jpeg_set_quality(&Info, Quality, true);
    jpeg_start_compress(&Info, true);

    while ((line = Info.next_scanline) < Info.image_height)
    {
        Pointer[0] = (JSAMPROW)input->data[line];
        (void)jpeg_write_scanlines(&Info, Pointer, 1);
    }

    jpeg_finish_compress(&Info);

    RealSize = DummySize - static_cast<int>(Info.dest->free_in_buffer);
    output.resize(RealSize);

    memcpy(output.data(), TempBuffer.data(), RealSize);

    jpeg_destroy_compress(&Info);

    return true;
}

bool readJpeg(const buffer& input, PixelArray *output, unsigned int Width, unsigned int Height) {
    jpeg_decompress_struct Info;
	jpeg_error_mgr ErrorManager;
	Info.err = jpeg_std_error(&ErrorManager);

    jpeg_create_decompress(&Info);
	SetMemorySource(&Info, input.data(), input.size());
	jpeg_read_header(&Info, true);
	jpeg_start_decompress(&Info);
    Info.out_color_space = JCS_EXT_RGBA;

    if ((Info.output_components != 3) && (Info.output_components != 4))
	{
		// LOG("Nr of channels must be 3 or 4!");
		return false;
	}

	int line;
	JSAMPROW row_pointer[1];
	while((line = Info.output_scanline) < Height){
		row_pointer[0] = (JSAMPROW) output->data[line];
		jpeg_read_scanlines(&Info, row_pointer, 1);
	}
    output->toBGRA();
	output->type = SOLID;

	jpeg_finish_decompress(&Info);
	jpeg_destroy_decompress(&Info);

	return true;
}

bool LoadCompressed(BLP_HEADER &Header, const ImageData *input, PixelArray *output)
{
    buffer temp;
	uint32_t  JpegHeaderSize;
	memcpy(reinterpret_cast<char*>(&JpegHeaderSize), input->data + sizeof(BLP_HEADER), sizeof(uint32_t));
	temp.resize(Header.Size[0] + JpegHeaderSize);
	memcpy(temp.data(), input->data + sizeof(BLP_HEADER) + sizeof(uint32_t), JpegHeaderSize);
	memcpy(temp.data() + JpegHeaderSize, input->data + Header.Offset[0], Header.Size[0]);
    return readJpeg(temp, output, Header.Width, Header.Height);
}

bool LoadUncompressed(BLP_HEADER &Header, const ImageData *input, PixelArray *output)
{
    static const int PALETTE_SIZE = 256;
    rgba const* Palette = reinterpret_cast<rgba const*>(input->data + sizeof(BLP_HEADER));
    uint8_t const* SourcePixel = reinterpret_cast<uint8_t const*>(input->data + Header.Offset[0]);
    int Size = Header.Width * Header.Height;
    Pixel* TargetPixel = *output->data;
    uint8_t const* SourceAlpha = SourcePixel + Size;
    switch (Header.AlphaBits)
    {
    default:
    case 0:
    	for (int i = 0; i < Size; i++)
    	{
    		TargetPixel[i].R = Palette[SourcePixel[i]].r;
            TargetPixel[i].G = Palette[SourcePixel[i]].g;
            TargetPixel[i].B = Palette[SourcePixel[i]].b;
    		TargetPixel[i].A = 255;
    	}
    	break;
    case 1:
    	for (int i = 0; i < Size; i++)
    	{
    		TargetPixel[i].R = Palette[SourcePixel[i]].r;
            TargetPixel[i].G = Palette[SourcePixel[i]].g;
            TargetPixel[i].B = Palette[SourcePixel[i]].b;
    		TargetPixel[i].A = (SourceAlpha[i >> 3] & (1 << (i & 7))) ? 1 : 0;
    	}
    	break;
    case 4:
    	for (int i = 0; i < Size; i++)
    	{
    		TargetPixel[i].R = Palette[SourcePixel[i]].r;
            TargetPixel[i].G = Palette[SourcePixel[i]].g;
            TargetPixel[i].B = Palette[SourcePixel[i]].b;
    		switch (i & 1)
    		{
    		case 0: TargetPixel[i].A = SourceAlpha[i >> 1] & 0x0F; break;
    		case 1: TargetPixel[i].A = (SourceAlpha[i >> 1] & 0xF0) >> 4; break;
    		}
    	}
    	break;
    case 8:
    	for (int i = 0; i < Size; i++)
    	{
    		TargetPixel[i].R = Palette[SourcePixel[i]].r;
            TargetPixel[i].G = Palette[SourcePixel[i]].g;
            TargetPixel[i].B = Palette[SourcePixel[i]].b;
    		TargetPixel[i].A = SourceAlpha[i];
    	}
    	break;
    }
    return true;
}

DECODER_FN(Blp)
{
    BLP_HEADER Header;
    memcpy(reinterpret_cast<char *>(&Header), input->data, sizeof(BLP_HEADER));
    if (Header.MagicNumber != '1PLB')
    {
        // LOG("The file is not a BLP file!");
        return FAIL;
    }

    if (output->Malloc(Header.Width, Header.Height) != SUCCESS)
    {
        return FAIL;
    }

    output->type = SOLID;

    switch (Header.Compression)
    {
    default:
    case 0:
        if (!LoadCompressed(Header, input, output))
            return FAIL;
        break;
    case 1:
        if (!LoadUncompressed(Header, input, output))
            return FAIL;
        break;
    }

    // TODO
    return SUCCESS;
}

ENCODER_FN(Blp)
{

    int32_t i;
    int32_t X;
    int32_t Y;
    int32_t Size;
    int32_t Index;
    int32_t BufferIndex;
    int32_t TotalSize;
    int32_t NrOfMipMaps;
    int32_t TextureSize;
    int32_t CurrentWidth;
    int32_t CurrentHeight;
    int32_t CurrentOffset;
    uint32_t JpegHeaderSize;
    BLP_HEADER Header;
    std::vector<buffer> MipMapBufferList;

    buffer out;

    JpegHeaderSize = 4;
    MipMapBufferList.resize(MAX_NR_OF_BLP_MIP_MAPS);

    Header.Compression = 0;
    Header.AlphaBits = 8;
    Header.Width = input->width;
    Header.Height = input->height;
    Header.Unknown1 = 4;
    Header.Unknown2 = 1;

    NrOfMipMaps = 0;

    Size = std::max(Header.Width, Header.Height);
    while (Size >= 1)
    {
        Size /= 2;
        NrOfMipMaps++;
    }

    if (NrOfMipMaps > MAX_NR_OF_BLP_MIP_MAPS)
    {
        NrOfMipMaps = MAX_NR_OF_BLP_MIP_MAPS;
    }

    if (NrOfMipMaps < 1)
    {
        return FAIL;
    }

    CurrentWidth = Header.Width;
    CurrentHeight = Header.Height;
    CurrentOffset = sizeof(BLP_HEADER) + sizeof(uint32_t) + JpegHeaderSize;

    // 修改颜色
    input->toBGRA();

    for (i = 0; i < NrOfMipMaps; i++)
    {
        PixelArray newArray, *pixels;
        pixels = input;
        if (input->width != CurrentWidth || input->height != CurrentHeight)
        {
            pixels = &newArray;
            if (pixels->Malloc(CurrentWidth, CurrentHeight) != SUCCESS)
            {
                return FAIL;
            }
            pixels->type = input->type;

            resize(input, pixels, NULL);
        }

        if (!writeToJpeg(pixels, MipMapBufferList[i], CurrentWidth, CurrentHeight, 100))
        {
            // 销毁内存
            newArray.Free();
            return FAIL;
        }

        TextureSize = MipMapBufferList[i].size();

        Header.Offset[i] = CurrentOffset;
        Header.Size[i] = TextureSize - JpegHeaderSize;

        CurrentWidth /= 2;
        CurrentHeight /= 2;
        CurrentOffset += Header.Size[i];

        if (CurrentWidth < 1)
            CurrentWidth = 1;
        if (CurrentHeight < 1)
            CurrentHeight = 1;
    }

    TotalSize = sizeof(BLP_HEADER) + sizeof(uint32_t) + JpegHeaderSize;
    for (i = 0; i < NrOfMipMaps; i++)
    {
        if (MipMapBufferList[i].size() <= 0)
            break;
        TotalSize += Header.Size[i];
    }

    output->position = TotalSize;
    output->length = TotalSize;
    out.resize(TotalSize);

    CurrentOffset = 0;

    memcpy(&out[CurrentOffset], &Header, sizeof(BLP_HEADER));
    CurrentOffset += sizeof(BLP_HEADER);

    memcpy(&out[CurrentOffset], &JpegHeaderSize, sizeof(uint32_t));
    CurrentOffset += sizeof(uint32_t);

    Size = Header.Size[0] + JpegHeaderSize;
    memcpy(&out[CurrentOffset], &((MipMapBufferList[0])[0]), Size);
    CurrentOffset += Size;

    for (i = 1; i < NrOfMipMaps; i++)
    {
        if (MipMapBufferList[i].size() <= 0)
            break;

        memcpy(&out[CurrentOffset], &((MipMapBufferList[i])[JpegHeaderSize]), Header.Size[i]);
        CurrentOffset += Header.Size[i];
    }

    output->data = (uint8_t *)malloc(output->length);
    memcpy(output->data, out.data(), output->length);

    return SUCCESS;
}

#endif
