// translation of ffmpeg's vaapi_encode.c example to libavpp

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
	int width,
	int height,
	int framerate,
	const char *const inpath,
	const char *const outpath)
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
	enc->time_base = (AVRational){1, framerate};
	enc->framerate = (AVRational){framerate, 1};
	enc->sample_aspect_ratio = (AVRational){1, 1};
	enc->pix_fmt = AV_PIX_FMT_VAAPI;

	av::HWFramesContext hwfctx{
		hwdevctx, AV_PIX_FMT_VAAPI, AV_PIX_FMT_NV12, width, height};
	enc.set_hwframe_ctx(hwfctx);
	enc.open();

	av::OwnedFrame sw_frame, hw_frame;
	sw_frame->width = width;
	sw_frame->height = height;
	sw_frame->format = AV_PIX_FMT_NV12;
	sw_frame.get_buffer();

	while (true)
	{
		if (fread(sw_frame->data[0], 1, size, fin) < size)
			break;
		if (fread(sw_frame->data[1], 1, size / 2, fin) < size / 2)
			break;

		// Get a new hardware frame from the pool for each software frame.
		hw_frame.unref();
		hwfctx.get_buffer(hw_frame);
		av::HWFramesContext::transfer_data(hw_frame, sw_frame);
		enc.send_frame(hw_frame.get());

		while (const auto pkt = enc.receive_packet())
		{
			pkt->stream_index = 0;
			if (fwrite(pkt->data, 1, pkt->size, fout) != pkt->size)
			{
				fprintf(stderr, "failed to write whole packet\n");
				throw;
			}
		}
	}

	// Flush the encoder
	enc.send_frame(nullptr);
	while (const auto pkt = enc.receive_packet())
	{
		pkt->stream_index = 0;
		if (fwrite(pkt->data, 1, pkt->size, fout) != pkt->size)
		{
			fprintf(stderr, "failed to write whole packet\n");
			throw;
		}
	}

	fclose(fin);
	fclose(fout);
}

int main(int argc, char *argv[])
{
	if (argc < 6)
	{
		fprintf(
			stderr,
			"Usage: %s <width> <height> <framerate> <input nv12 file> "
			"<output h264 file>\n",
			argv[0]);
		return -1;
	}

	int width = atoi(argv[1]);
	int height = atoi(argv[2]);
	int framerate = atoi(argv[3]);
	cpp_main(width, height, framerate, argv[4], argv[5]);
	return 0;
}
