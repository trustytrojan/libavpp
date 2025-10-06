// translation of ffmpeg's vaapi_encode.c example to libavpp, with muxing

extern "C"
{
#include <errno.h>
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <stdio.h>
#include <string.h>
}

#include <av/Encoder.hpp>
#include <av/HWDeviceContext.hpp>
#include <av/HWFramesContext.hpp>
#include <av/MediaWriter.hpp>

void cpp_main(
	int width,
	int height,
	int framerate,
	const char *const inpath,
	const char *const outpath)
{
	FILE *fin;
	const auto size = width * height;

	if (!(fin = fopen(inpath, "r")))
	{
		fprintf(stderr, "Fail to open input file : %s\n", strerror(errno));
		throw;
	}

	// Set up the MediaWriter for the MP4 container
	av::MediaWriter writer{outpath};

	av::HWDeviceContext hwdevctx{AV_HWDEVICE_TYPE_VAAPI};

	const AVCodec *codec;
	if (!(codec = avcodec_find_encoder_by_name("h264_vaapi")))
	{
		fprintf(stderr, "Could not find encoder.\n");
		throw;
	}

	// Create and configure the video stream in the writer
	auto stream = writer.new_stream(codec);
	stream->id = writer->nb_streams - 1;

	// Configure the encoder
	av::Encoder enc{codec};
	enc->width = width;
	enc->height = height;
	enc->time_base = (AVRational){1, framerate};
	enc->framerate = (AVRational){framerate, 1};
	enc->sample_aspect_ratio = (AVRational){1, 1};
	enc->pix_fmt = AV_PIX_FMT_VAAPI;
	av_opt_set(enc->priv_data, "qp", "20", 0); // Example: Set constant quality

	// Set up hardware frames and link to encoder
	av::HWFramesContext hwfctx{
		hwdevctx, AV_PIX_FMT_VAAPI, AV_PIX_FMT_NV12, width, height};
	enc.set_hwframe_ctx(hwfctx);
	enc.open();

	// Copy encoder parameters to the muxer stream
	stream.copy_params(enc);
	stream->time_base = enc->time_base;

	// Write the container header
	writer.write_header();
	writer.print_info(0);

	av::OwnedFrame sw_frame, hw_frame;
	sw_frame->width = width;
	sw_frame->height = height;
	sw_frame->format = AV_PIX_FMT_NV12;
	sw_frame.get_buffer();

	int64_t frame_count = 0;
	while (true)
	{
		if (fread(sw_frame->data[0], 1, size, fin) < size)
			break;
		if (fread(sw_frame->data[1], 1, size / 2, fin) < size / 2)
			break;

		sw_frame->pts = frame_count++;

		hw_frame.unref();
		hwfctx.get_buffer(hw_frame);
		hw_frame->pts = sw_frame->pts; // <<< THIS IS THE FIX
		av::HWFramesContext::transfer_data(hw_frame, sw_frame);
		enc.send_frame(hw_frame);

		while (const auto pkt = enc.receive_packet())
		{
			pkt->stream_index = stream->index;
			av_packet_rescale_ts(pkt, enc->time_base, stream->time_base);
			writer.write_packet(pkt);
		}
	}

	// Flush the encoder
	enc.send_frame(nullptr);
	while (const auto pkt = enc.receive_packet())
	{
		pkt->stream_index = stream->index;
		av_packet_rescale_ts(pkt, enc->time_base, stream->time_base);
		writer.write_packet(pkt);
	}

	// Write the container trailer to finalize the file
	writer.write_trailer();

	fclose(fin);
}

int main(int argc, char *argv[])
{
	if (argc < 6)
	{
		fprintf(
			stderr,
			"Usage: %s <width> <height> <framerate> <input nv12 file> "
			"<output mp4 file>\n",
			argv[0]);
		return -1;
	}

	int width = atoi(argv[1]);
	int height = atoi(argv[2]);
	int framerate = atoi(argv[3]);
	cpp_main(width, height, framerate, argv[4], argv[5]);
	return 0;
}
