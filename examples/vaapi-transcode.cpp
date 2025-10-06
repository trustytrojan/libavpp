// translation of ffmpeg's vaapi_transcode.c to libavpp

#include "av/Util.hpp"
#include <av/HWDeviceContext.hpp>
#include <av/HWFramesContext.hpp>
#include <av/MediaReader.hpp>
#include <av/MediaWriter.hpp>

#include <iostream>

void transcode(
	const char *const inpath,
	const char *const outpath,
	const char *const enc_codec_name)
{
	av::HWDeviceContext hwdctx{AV_HWDEVICE_TYPE_VAAPI};
	av::MediaReader ifmt{inpath};
	const auto video_stream = ifmt.find_best_stream(AVMEDIA_TYPE_VIDEO);

	auto decoder_ctx = video_stream.create_decoder();
	decoder_ctx.set_hwdevice_ctx(hwdctx);
	decoder_ctx->get_format = [](auto, auto p)
	{
		for (; *p != AV_PIX_FMT_NONE; ++p)
			if (*p == AV_PIX_FMT_VAAPI)
				return *p;
		std::cerr << "decoder_ctx->get_format: Unable to decode this file "
					 "using VA-API\n";
		return AV_PIX_FMT_NONE;
	};
	decoder_ctx.open();

	av::MediaWriter ofmt{outpath};
	const auto encoder = av::find_encoder_by_name(enc_codec_name);
	av::Encoder encoder_ctx{encoder};
	av::Stream ost{nullptr};
	bool initialized{};

	while (const auto pkt = ifmt.read_packet())
	{
		if (pkt->stream_index != video_stream->index)
		{
			// for now, we only transcode video and ignore other streams
			continue;
		}

		decoder_ctx.send_packet(pkt);
		while (const auto frm = decoder_ctx.receive_frame())
		{
			if (!initialized)
			{
				// set AVCodecContext Parameters for encoder
				if (!(encoder_ctx->hw_frames_ctx =
						  av_buffer_ref(decoder_ctx->hw_frames_ctx)))
					throw av::Error("av_buffer_ref", AVERROR(ENOMEM));
				encoder_ctx->time_base = video_stream->time_base;
				encoder_ctx->framerate = video_stream->avg_frame_rate;
				encoder_ctx->pix_fmt = AV_PIX_FMT_VAAPI;
				encoder_ctx->width = decoder_ctx->width;
				encoder_ctx->height = decoder_ctx->height;
				encoder_ctx.open();

				ost = ofmt.new_stream(encoder);
				ost.copy_params(encoder_ctx);
				ost->time_base = encoder_ctx->time_base;

				ofmt.write_header();
				initialized = true;
			}

			encoder_ctx.send_frame(frm);
			while (const auto enc_pkt = encoder_ctx.receive_packet())
			{
				enc_pkt->stream_index = ost->index;
				av_packet_rescale_ts(
					enc_pkt, video_stream->time_base, ost->time_base);
				ofmt.write_packet(enc_pkt);
			}
		}
	}

	// flush encoder
	encoder_ctx.send_frame(nullptr);
	while (const auto enc_pkt = encoder_ctx.receive_packet())
	{
		enc_pkt->stream_index = ost->index;
		av_packet_rescale_ts(enc_pkt, video_stream->time_base, ost->time_base);
		ofmt.write_packet(enc_pkt);
	}

	ofmt.write_trailer();
}

int main(const int argc, const char *const *const argv)
{
	if (argc != 4)
	{
		std::cerr << "Usage: " << argv[0]
				  << " <input file> <encode codec> <output file>\n"
					 "The output format is guessed according to the file "
					 "extension.\n";
		return EXIT_FAILURE;
	}

	transcode(argv[1], argv[3], argv[2]);
}
