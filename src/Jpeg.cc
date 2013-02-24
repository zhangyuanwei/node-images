#include "Image.h"

#ifdef HAVE_JPEG

#include <stdlib.h>
#include <string.h>

#include <setjmp.h>
#include <jpeglib.h>

struct my_jpeg_error_mgr {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

void jpeg_cb_error_exit(j_common_ptr cinfo){
	struct my_jpeg_error_mgr *mptr;
	mptr=(struct my_jpeg_error_mgr*) cinfo->err;
	//(*cinfo->err->output_message) (cinfo);
	longjmp(mptr->setjmp_buffer, 1);
}

DECODER_FN(Jpeg){
	struct jpeg_decompress_struct cinfo;
	struct my_jpeg_error_mgr jerr;

	int width, height, line;
	size_t row_stride;
	JSAMPARRAY buffer;

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit=jpeg_cb_error_exit;

	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		output->Free();
		return FAIL;
	}

	jpeg_create_decompress(&cinfo);	
	jpeg_mem_src(&cinfo, (unsigned char *) input->data, input->length);	
	jpeg_read_header(&cinfo, TRUE);
	cinfo.out_color_space = JCS_EXT_RGBA;
	jpeg_start_decompress(&cinfo);

	width = cinfo.output_width;
	height = cinfo.output_height;
	//components = cinfo.output_components;

	row_stride = width * cinfo.output_components;

	if(output->Malloc(width, height) != SUCCESS) 
		longjmp(jerr.setjmp_buffer, 1);

	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

	while((line = cinfo.output_scanline) < height){
		jpeg_read_scanlines(&cinfo, buffer, 1);
		memcpy(output->data[line], buffer[0], row_stride);
	}
	output->alpha = false;

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	return SUCCESS;
}

ENCODER_FN(Jpeg){
	//TODO
	return FAIL;
}

#endif
