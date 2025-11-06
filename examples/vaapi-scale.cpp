/**
 * @file vaapi-scale.cpp
 * @brief Hardware-accelerated video scaling using VAAPI and libavpp filter API
 *
 * This example demonstrates how to use VAAPI hardware acceleration to scale
 * video frames from one resolution to another using the filter API.
 *
 * The filter chain:
 * (input) -> buffer -> scale_vaapi -> buffersink -> (output)
 *
 * Input and output are NV12 format files.
 */

extern "C"
{
#include <errno.h>
#include <libavutil/hwcontext.h>
#include <stdio.h>
#include <string.h>
}

#include <av/BufferSrc.hpp>
#include <av/FilterContext.hpp>
#include <av/FilterGraph.hpp>
#include <av/Frame.hpp>
#include <av/HWDeviceContext.hpp>
#include <av/HWFramesContext.hpp>
#include <av/Util.hpp>

class ScalingPipeline
{
	av::FilterGraph graph;
	av::BufferSrc src;
	av::FilterContext sink;
	av::HWFramesContext &in_hwfctx;

	void
	setup_src(const int width, const int height, AVBufferRef *hw_frames_ctx)
	{
		// Create and configure the buffer filter (source)
		src = graph.alloc_buffersrc("src", false);

		// Set buffer options
		char size_str[64];
		snprintf(size_str, sizeof(size_str), "%d:%d", width, height);
		src.opt_set("video_size", static_cast<const char *>(size_str));
		src.opt_set("pix_fmt", static_cast<int64_t>(AV_PIX_FMT_VAAPI));
		src.opt_set("time_base", AVRational{1, 25});

		// Set the hardware frames context for the input BEFORE init using
		// parameters
		av::BufferSrc::Parameters params;
		params->hw_frames_ctx = av_buffer_ref(hw_frames_ctx);
		src.parameters_set(params);

		src.init(static_cast<const char *>(nullptr));
	}

	av::FilterContext setup_scale(const int out_width, const int out_height)
	{
		// Create and configure the scale_vaapi filter
		const AVFilter *scale = av::filter_get_by_name("scale_vaapi");
		auto scale_ctx = graph.alloc_filter(scale, "scale");

		// Initialize with options string format "w=1280:h=720"
		char options[128];
		snprintf(options, sizeof(options), "w=%d:h=%d", out_width, out_height);
		scale_ctx.init(static_cast<const char *>(options));

		return scale_ctx;
	}

	void setup_sink()
	{
		// Create the buffersink filter (sink)
		const AVFilter *buffersink = av::filter_get_by_name("buffersink");
		sink = graph.alloc_filter(buffersink, "sink");
		sink.init(static_cast<const char *>(nullptr));
	}

public:
	ScalingPipeline(
		av::HWFramesContext &in_hwfctx,
		const int in_width,
		const int in_height,
		const int out_width,
		const int out_height)
		: in_hwfctx{in_hwfctx}
	{
		// Setup filters
		setup_src(in_width, in_height, in_hwfctx);
		auto scale_ctx = setup_scale(out_width, out_height);
		setup_sink();

		// Link the filters together
		src >> scale_ctx >> sink;

		// Finalize graph
		graph.configure();
	}

	void operator<<(AVFrame *frame) { src.add_frame(frame); }
	void operator>>(AVFrame *frame) { sink.get_frame(frame); }
};

void vaapi_scale(
	const int in_width,
	const int in_height,
	const int out_width,
	const int out_height,
	const char *const inpath,
	const char *const outpath)
{
	FILE *fin, *fout;
	const auto in_size = in_width * in_height;
	const auto out_size = out_width * out_height;

	printf(
		"Scaling from %dx%d to %dx%d\n",
		in_width,
		in_height,
		out_width,
		out_height);

	if (!(fin = fopen(inpath, "r")))
	{
		fprintf(stderr, "Failed to open input file: %s\n", strerror(errno));
		throw;
	}

	if (!(fout = fopen(outpath, "w+b")))
	{
		fprintf(stderr, "Failed to open output file: %s\n", strerror(errno));
		fclose(fin);
		throw;
	}

	// Create VAAPI hardware device context
	av::HWDeviceContext hwdevctx{AV_HWDEVICE_TYPE_VAAPI};

	// Create hardware frames contexts for input and output
	av::HWFramesContext in_hwfctx{
		hwdevctx, AV_PIX_FMT_VAAPI, AV_PIX_FMT_NV12, in_width, in_height};

	// Create the scaling pipeline
	ScalingPipeline pipeline{
		in_hwfctx, in_width, in_height, out_width, out_height};

	// Allocate frames
	av::OwnedFrame sw_in_frame, hw_in_frame, hw_out_frame, sw_out_frame;

	// Setup software input frame
	sw_in_frame->width = in_width;
	sw_in_frame->height = in_height;
	sw_in_frame->format = AV_PIX_FMT_NV12;
	sw_in_frame.get_buffer();

	// Setup software output frame
	sw_out_frame->width = out_width;
	sw_out_frame->height = out_height;
	sw_out_frame->format = AV_PIX_FMT_NV12;
	sw_out_frame.get_buffer();

	int frame_count = 0;
	while (true)
	{
		// Read NV12 data from input file
		if (fread(sw_in_frame->data[0], 1, in_size, fin) < in_size)
			break;
		if (fread(sw_in_frame->data[1], 1, in_size / 2, fin) < in_size / 2)
			break;

		// Transfer software frame to hardware
		hw_in_frame.unref();
		in_hwfctx.get_buffer(hw_in_frame);
		av::HWFramesContext::transfer_data(hw_in_frame, sw_in_frame);

		// Send frame to the filter graph
		pipeline << hw_in_frame;

		// Get filtered output
		try
		{
			pipeline >> hw_out_frame;

			// Transfer hardware frame back to software
			av::HWFramesContext::transfer_data(sw_out_frame, hw_out_frame);

			// Write NV12 data to output file
			if (fwrite(sw_out_frame->data[0], 1, out_size, fout) != out_size)
			{
				fprintf(stderr, "Failed to write Y plane\n");
				throw;
			}
			if (fwrite(sw_out_frame->data[1], 1, out_size / 2, fout) !=
				out_size / 2)
			{
				fprintf(stderr, "Failed to write UV plane\n");
				throw;
			}

			frame_count++;
			hw_out_frame.unref();
		}
		catch (const av::Error &e)
		{
			if (e.errnum == AVERROR(EAGAIN))
			{
				// Need to feed more frames
				continue;
			}
			else
			{
				throw;
			}
		}
	}

	// Flush the filter graph
	pipeline << nullptr;
	while (true)
	{
		try
		{
			pipeline >> hw_out_frame;

			// Transfer hardware frame back to software
			av::HWFramesContext::transfer_data(sw_out_frame, hw_out_frame);

			// Write NV12 data to output file
			if (fwrite(sw_out_frame->data[0], 1, out_size, fout) != out_size)
			{
				fprintf(stderr, "Failed to write Y plane\n");
				throw;
			}
			if (fwrite(sw_out_frame->data[1], 1, out_size / 2, fout) !=
				out_size / 2)
			{
				fprintf(stderr, "Failed to write UV plane\n");
				throw;
			}

			frame_count++;
			hw_out_frame.unref();
		}
		catch (const av::Error &e)
		{
			if (e.errnum == AVERROR_EOF)
			{
				// No more frames
				break;
			}
			else
			{
				throw;
			}
		}
	}

	printf("Processed %d frames\n", frame_count);

	fclose(fin);
	fclose(fout);
}

int main(int argc, char *argv[])
{
	if (argc < 7)
	{
		fprintf(
			stderr,
			"Usage: %s <in_width> <in_height> <out_width> <out_height> "
			"<input nv12 file> <output nv12 file>\n",
			argv[0]);
		return -1;
	}

	const int in_width = atoi(argv[1]);
	const int in_height = atoi(argv[2]);
	const int out_width = atoi(argv[3]);
	const int out_height = atoi(argv[4]);

	try
	{
		vaapi_scale(
			in_width, in_height, out_width, out_height, argv[5], argv[6]);
	}
	catch (const av::Error &e)
	{
		fprintf(stderr, "Error: %s\n", e.what());
		return -1;
	}

	return 0;
}
