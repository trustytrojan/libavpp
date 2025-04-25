#include <av/HWDeviceContext.hpp>
#include <av/HWFramesContext.hpp>
#include <av/MediaReader.hpp>
#include <av/MediaWriter.hpp>

#include <iostream>

std::ostream &operator<<(std::ostream &ostr, const AVRational &r)
{
	return ostr << r.num << '/' << r.den;
}

bool operator==(const AVRational &a, const AVRational &b)
{
	return a.num == b.num && a.den == b.den;
}

void print_stream_timing(const av::Stream &s, const char *const name)
{
	std::cerr << name << ": \n"
			  << "  time_base: " << s->time_base << '\n'
			  << "  avg_frame_rate: " << s->avg_frame_rate << '\n'
			  << "  r_frame_rate: " << s->r_frame_rate << '\n'
			  << "  codecpar->framerate: " << s->codecpar->framerate << '\n';
}

void print_codec_timing(const av::CodecContext &c, const char *const name)
{
	std::cerr << name << ": \n"
			  << "  time_base: " << c->time_base << '\n'
			  << "  framerate: " << c->framerate << '\n';
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

	av::HWDeviceContext hwdctx{AV_HWDEVICE_TYPE_VAAPI};
	av::MediaReader ifmt{argv[1]};
	const auto video_stream = ifmt.find_best_stream(AVMEDIA_TYPE_VIDEO);
	print_stream_timing(video_stream, "input");

	auto decoder_ctx = video_stream.create_decoder();
	decoder_ctx.copy_params(video_stream->codecpar);
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
	print_codec_timing(decoder_ctx, "decoder");
	decoder_ctx.open();

	av::MediaWriter ofmt{argv[3]};
	const auto encoder = av::find_encoder_by_name(argv[2]);
	av::Encoder encoder_ctx{encoder};
	av::Stream ost{nullptr};

	bool initialized{};

	while (const auto pkt = ifmt.read_packet())
	{
		if (pkt->stream_index != video_stream->index)
			continue;

		// dec_enc()
		decoder_ctx.send_packet(pkt);
		while (const auto frm = decoder_ctx.receive_frame())
		{
			if (!initialized)
			{
				/* we need to ref hw_frames_ctx of decoder to initialize
				 * encoder's codec. Only after we get a decoded frame, can we
				 * obtain its hw_frames_ctx
				 */
				if (!(encoder_ctx->hw_frames_ctx =
						  av_buffer_ref(decoder_ctx->hw_frames_ctx)))
					throw av::Error("av_buffer_ref", AVERROR(ENOMEM));

				/* set AVCodecContext Parameters for encoder, here we keep them
				 * stay the same as decoder. xxx: now the sample can't handle
				 * resolution change case.
				 */
				encoder_ctx->time_base = {
					1, 1000}; // av_inv_q(decoder_ctx->framerate);
				encoder_ctx->pix_fmt = AV_PIX_FMT_VAAPI;
				encoder_ctx->width = decoder_ctx->width;
				encoder_ctx->height = decoder_ctx->height;
				print_codec_timing(encoder_ctx, "encoder");
				encoder_ctx.open();

				ost = ofmt.new_stream(encoder);
				ost.copy_params(encoder_ctx);
				ost->time_base = encoder_ctx->time_base;
				print_stream_timing(ost, "output");

				ofmt.write_header();

				initialized = true;
			}

			// encode_write()
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

	ofmt.write_trailer();
}
