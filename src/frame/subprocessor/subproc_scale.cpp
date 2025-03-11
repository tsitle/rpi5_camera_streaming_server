#include "subproc_scale.hpp"

namespace framesubproc {

	FrameSubProcessorScale::FrameSubProcessorScale() :
			FrameSubProcessor("SCALE") {
	}

	void FrameSubProcessorScale::processFrame(cv::Mat &frame, const uint32_t frameNr) {
		gFrameNr = frameNr;
		//
		const uint32_t targetWidth = gStaticOptionsStc.scale.resolutionOutputScaled.width;
		const uint32_t targetHeight = gStaticOptionsStc.scale.resolutionOutputScaled.height;

		if (static_cast<uint32_t>(frame.size().width) == targetWidth &&
				static_cast<uint32_t>(frame.size().height) == targetHeight) {
			return;
		}

		const auto orgWidth = static_cast<uint32_t>(frame.size().width);
		const auto orgHeight = static_cast<uint32_t>(frame.size().height);
		const auto aspectRatio = static_cast<double>(orgWidth) / static_cast<double>(orgHeight);

		auto scaledWidth = targetWidth;
		auto scaledHeight = static_cast<uint32_t>(round(static_cast<double>(targetWidth) / aspectRatio));

		if (scaledHeight > targetHeight) {
			scaledHeight = targetHeight;
			scaledWidth = static_cast<uint32_t>(round(static_cast<double>(scaledHeight) * aspectRatio));
		}

		const auto frameScaled = cv::Mat(static_cast<int>(scaledHeight), static_cast<int>(scaledWidth), CV_8UC3);
		const auto frameTarget = cv::Mat(
				static_cast<int>(targetHeight),
				static_cast<int>(targetWidth),
				CV_8UC3,
				/*background color:*/cv::Scalar(0, 0, 0)
			);
		auto insetX = static_cast<int>((targetWidth - scaledWidth) / 2);
		auto insetY = static_cast<int>((targetHeight - scaledHeight) / 2);

		if (insetX + scaledWidth > targetWidth) {
			insetX = static_cast<int>(targetWidth - scaledWidth);
		}
		if (insetY + scaledHeight > targetHeight) {
			insetY = static_cast<int>(targetHeight - scaledHeight);
		}

		const cv::Mat insetImageForScaled(
				frameTarget,
				cv::Rect(insetX, insetY, static_cast<int>(scaledWidth), static_cast<int>(scaledHeight))
			);

		cv::resize(
				frame,
				frameScaled,
				cv::Size2d(scaledWidth, scaledHeight),
				0.0,
				0.0,
				cv::INTER_LINEAR
			);

		frameScaled(
				cv::Range(0, static_cast<int>(scaledHeight)),
				cv::Range(0, static_cast<int>(scaledWidth)))
			.copyTo(insetImageForScaled);

		frame = frameTarget;
	}

}  // namespace framesubproc
