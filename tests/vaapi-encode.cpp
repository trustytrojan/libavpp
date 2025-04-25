// translation of ffmpeg's vaapi-encode.c example to libavpp

extern "C"
{
#include <errno.h>
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
#include <stdio.h>
#include <string.h>
}

#include <av/Encoder.hpp>
#include <av/HWDeviceContext.hpp>
#include <av/HWFramesContext.hpp>

void cpp_main(
	int width, int height, const char *const inpath, const char *const outpath)
{
	FILE *fin, *fout;
	const auto size = width * height;
	printf("size: %d\n", size);

	if (!(fin = fopen(inpath, "r")))
	{
		fprintf(stderr, "Fail to open input file : %s\n", strerror(errno));
		throw;
	}

	if (!(fout = fopen(outpath, "w+b")))
	{
		fprintf(stderr, "Fail to open output file : %s\n", strerror(errno));
		throw;
	}

	av::HWDeviceContext hwdevctx{AV_HWDEVICE_TYPE_VAAPI};

	const AVCodec *codec;
	if (!(codec = avcodec_find_encoder_by_name("h264_vaapi")))
	{
		fprintf(stderr, "Could not find encoder.\n");
		throw;
	}

	av::Encoder enc{codec};
	enc->width = width;
	enc->height = height;
	enc->time_base = (AVRational){1, 25};
	enc->framerate = (AVRational){25, 1};
	enc->sample_aspect_ratio = (AVRational){1, 1};
	enc->pix_fmt = AV_PIX_FMT_VAAPI;

	av::HWFramesContext hwfctx{
		hwdevctx, AV_PIX_FMT_VAAPI, AV_PIX_FMT_NV12, width, height};
	enc.set_hwframe_ctx(hwfctx);
	enc.open();

	av::Frame sw_frame;
	sw_frame->width = width;
	sw_frame->height = height;
	sw_frame->format = AV_PIX_FMT_NV12;
	sw_frame.get_buffer();

	av::Frame hw_frame;
	hwfctx.get_buffer(hw_frame);

	while (true)
	{
		int err;

		if (!fread(sw_frame->data[0], size, 1, fin))
		{
			fprintf(
				stderr,
				"data[0] fread <= 0, ferror: %d, feof: %d\n",
				ferror(fin),
				feof(fin));
			break;
		}

		if (!fread(sw_frame->data[1], size / 2, 1, fin))
		{
			fprintf(
				stderr,
				"data[1] fread <= 0, ferror: %d, feof: %d\n",
				ferror(fin),
				feof(fin));
			break;
		}

		av::HWFramesContext::transfer_data(hw_frame, sw_frame);
		enc.send_frame(hw_frame.get());

		while (const auto pkt = enc.receive_packet())
		{
			pkt->stream_index = 0;
			if (pkt->size != fwrite(pkt->data, pkt->size, 1, fout))
			{
				fprintf(stderr, "failed to write whole packet\n");
				throw;
			}
		}
	}

	fclose(fin);
	fclose(fout);
}

int main(int argc, char *argv[])
{
	if (argc < 5)
	{
		fprintf(
			stderr,
			"Usage: %s <width> <height> <input nv12 file> <output h264 file>\n",
			argv[0]);
		return -1;
	}

	int width = atoi(argv[1]);
	int height = atoi(argv[2]);
	cpp_main(width, height, argv[3], argv[4]);
	return 0;
}
