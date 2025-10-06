// translation of ffmpeg's hw_decode.c to libavpp

#include <av/Frame.hpp>
#include <av/HWDeviceContext.hpp>
#include <av/MediaReader.hpp>
#include <av/Util.hpp>

#include <fstream>
#include <iostream>
#include <vector>

extern "C"
{
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
}

static AVPixelFormat
get_hw_format_callback(AVCodecContext *ctx, const AVPixelFormat *pix_fmts)
{
	const auto hw_pix_fmt = *static_cast<const AVPixelFormat *>(ctx->opaque);
	for (const auto *p = pix_fmts; *p != AV_PIX_FMT_NONE; ++p)
		if (*p == hw_pix_fmt)
			return *p;
	std::cerr << "get_hw_format_callback: Failed to get HW surface format.\n";
	return AV_PIX_FMT_NONE;
}

static AVPixelFormat
find_hw_pix_fmt(const AVCodec *decoder, const AVHWDeviceType type)
{
	for (int i = 0;; i++)
	{
		const auto config = avcodec_get_hw_config(decoder, i);
		if (!config)
			throw std::runtime_error("Decoder does not support device type");
		if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
			config->device_type == type)
		{
			return config->pix_fmt;
		}
	}
	return AV_PIX_FMT_NONE;
}

void hw_decode(
	const char *const dev_type_name,
	const char *const inpath,
	const char *const outpath)
{
	const auto dev_type = av_hwdevice_find_type_by_name(dev_type_name);
	if (dev_type == AV_HWDEVICE_TYPE_NONE)
	{
		std::cerr << "Device type " << dev_type_name << " is not supported.\n";
		std::cerr << "Available device types:";
		for (const auto type : av::HWDeviceTypes{})
			std::cerr << " " << av_hwdevice_get_type_name(type);
		std::cerr << "\n";
		throw std::runtime_error("Unsupported device type");
	}

	av::HWDeviceContext hwdctx{dev_type};
	av::MediaReader ifmt{inpath};
	ifmt.print_info(0);

	const AVCodec *decoder = nullptr;
	const auto video_stream =
		ifmt.find_best_stream(AVMEDIA_TYPE_VIDEO, -1, -1, &decoder);

	auto hw_pix_fmt = find_hw_pix_fmt(decoder, dev_type);

	auto decoder_ctx = video_stream.create_decoder();
	decoder_ctx.copy_params(video_stream->codecpar);

	decoder_ctx.set_hwdevice_ctx(hwdctx);
	decoder_ctx->opaque = &hw_pix_fmt;
	decoder_ctx->get_format = get_hw_format_callback;
	decoder_ctx.open(decoder);

	std::cout << "decoder pixel format: "
			  << av_get_pix_fmt_name(decoder_ctx->pix_fmt) << '\n';

	std::ofstream ofile{outpath, std::ios::binary};
	if (!ofile)
		throw std::runtime_error("Failed to open output file");

	av::OwnedFrame sw_frame;
	auto process_frame = [&](const av::Frame &hw_frame)
	{
		av::Frame frame_to_write{nullptr};

		if (hw_frame->format == hw_pix_fmt)
		{
			av::HWFramesContext::transfer_data(sw_frame, hw_frame);
			frame_to_write = sw_frame;
		}
		else
		{
			frame_to_write = hw_frame;
		}

		const auto size = frame_to_write.image_get_buffer_size();
		std::vector<uint8_t> buffer(size);
		frame_to_write.image_copy_to_buffer(buffer.data(), buffer.size());

		ofile.write(reinterpret_cast<const char *>(buffer.data()), size);
	};

	while (const auto pkt = ifmt.read_packet())
	{
		if (pkt->stream_index != video_stream->index)
			continue;

		decoder_ctx.send_packet(pkt);
		while (const auto hw_frame = decoder_ctx.receive_frame())
			process_frame(hw_frame);
	}

	// flush decoder
	decoder_ctx.send_packet(nullptr);
	while (const auto hw_frame = decoder_ctx.receive_frame())
		process_frame(hw_frame);
}

int main(const int argc, const char *const *const argv)
{
	if (argc != 4)
	{
		std::cerr << "Usage: " << argv[0]
				  << " <device type> <input file> <output file>\n";
		return EXIT_FAILURE;
	}

	try
	{
		hw_decode(argv[1], argv[2], argv[3]);
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
