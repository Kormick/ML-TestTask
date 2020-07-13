#ifndef PARSER_HPP
#define PARSER_HPP

#include <iomanip>
#include <iostream>

#include "../MFTypes.h"
#include "PipeParser.hpp"

bool testParserFrame()
{
	std::shared_ptr<MF_FRAME> frame = std::make_shared<MF_FRAME>();

	frame->time.rtStartTime = 0;
	frame->time.rtEndTime = 1;

	frame->av_props.vidProps.fccType = eMFCC_I420;
	frame->av_props.vidProps.nWidth = 2;
	frame->av_props.vidProps.nHeight = 3;
	frame->av_props.vidProps.nRowBytes = 4;
	frame->av_props.vidProps.nAspectX = 5;
	frame->av_props.vidProps.nAspectY = 6;
	frame->av_props.vidProps.dblRate = 7.89;

	frame->av_props.audProps.nChannels = 10;
	frame->av_props.audProps.nSamplesPerSec = 11;
	frame->av_props.audProps.nBitsPerSample = 12;
	frame->av_props.audProps.nTrackSplitBits = 13;

	frame->str_user_props = "user_props";

	for (size_t i = 0; i < 10; ++i)
		frame->vec_video_data.push_back(i);
	for (size_t i = 0; i < 10; ++i)
		frame->vec_audio_data.push_back(i);

	auto bytes = serialize("", frame);

	PipeParser parser;
	auto size = parser.parse(bytes.data(), bytes.size());

	if (size != bytes.size())
	{
		std::cout << "Frame parser failed: " << std::endl;
		std::cout << "Parsed " << size << ". Expected " << bytes.size() << std::endl;
		return false;
	}

	if (parser.getState() != PipeParser::State::FRAME_READY)
	{
		std::cout << "Frame parser failed: " << std::endl;
		std::cout << "State " << static_cast<int32_t>(parser.getState()) << std::endl;
		return false;
	}

	const auto data = parser.getData();

	bytes.erase(bytes.begin(), bytes.begin() + 13); // Remove DATA_SYNC, data type byte and channel from serialized data.
	if (bytes != data)
	{
		std::cout << "Frame parser failed: " << std::endl;

		{
			std::cout << "Data: " << std::endl;
			std::ios state(nullptr);
			state.copyfmt(std::cout);
			for (auto byte : data) {
				std::cout << std::hex << std::setw(2) << static_cast<int>(byte) << " ";
			}
			std::cout << std::endl;
			std::cout.copyfmt(state);
		}

		{
			std::cout << "Expected: " << std::endl;
			std::ios state(nullptr);
			state.copyfmt(std::cout);
			for (auto byte : bytes) {
				std::cout << std::hex << std::setw(2) << static_cast<int>(byte) << " ";
			}
			std::cout << std::endl;
			std::cout.copyfmt(state);
		}

		return false;
	}

	return true;
}

bool testParserBuffer()
{
	std::shared_ptr<MF_BUFFER> buffer = std::make_shared<MF_BUFFER>();
	buffer->flags = eMFBF_Buffer;
	for (auto i = 0; i < 10; ++i)
		buffer->data.push_back(i);

	auto bytes = serialize("", buffer);

	PipeParser parser;
	auto size = parser.parse(bytes.data(), bytes.size());

	if (size != bytes.size())
	{
		std::cout << "Buffer parser failed: " << std::endl;
		std::cout << "Parsed " << size << ". Expected " << bytes.size() << std::endl;
		return false;
	}

	if (parser.getState() != PipeParser::State::BUFFER_READY)
	{
		std::cout << "Buffer parser failed: " << std::endl;
		std::cout << "State " << static_cast<int32_t>(parser.getState()) << std::endl;
		return false;
	}

	const auto data = parser.getData();

	bytes.erase(bytes.begin(), bytes.begin() + 13); // Remove DATA_SYNC, data type byte and channel from serialized data.
	if (bytes != data)
	{
		std::cout << "Buffer parser failed: " << std::endl;

		{
			std::cout << "Data: " << std::endl;
			std::ios state(nullptr);
			state.copyfmt(std::cout);
			for (auto byte : data) {
				std::cout << std::hex << std::setw(2) << static_cast<int>(byte) << " ";
			}
			std::cout << std::endl;
			std::cout.copyfmt(state);
		}

		{
			std::cout << "Expected: " << std::endl;
			std::ios state(nullptr);
			state.copyfmt(std::cout);
			for (auto byte : bytes) {
				std::cout << std::hex << std::setw(2) << static_cast<int>(byte) << " ";
			}
			std::cout << std::endl;
			std::cout.copyfmt(state);
		}
		return false;
	}

	return true;
}

bool testParserMessage()
{
	std::shared_ptr<Message> message = std::make_shared<Message>();
	message->name = "name";
	message->param = "param";

	auto bytes = serialize("", message);

	PipeParser parser;
	auto size = parser.parse(bytes.data(), bytes.size());

	if (size != bytes.size())
	{
		std::cout << "Message parser failed: " << std::endl;
		std::cout << "Parsed " << size << ". Expected " << bytes.size() << std::endl;
		return false;
	}

	if (parser.getState() != PipeParser::State::MESSAGE_READY)
	{
		std::cout << "Message parser failed: " << std::endl;
		std::cout << "State " << static_cast<int32_t>(parser.getState()) << std::endl;
		return false;
	}

	const auto data = parser.getData();

	bytes.erase(bytes.begin(), bytes.begin() + 13); // Remove DATA_SYNC, data type byte and channel from serialized data.
	if (bytes != data)
	{
		std::cout << "Message parser failed: " << std::endl;

		{
			std::cout << "Data: " << std::endl;
			std::ios state(nullptr);
			state.copyfmt(std::cout);
			for (auto byte : data) {
				std::cout << std::hex << std::setw(2) << static_cast<int>(byte) << " ";
			}
			std::cout << std::endl;
			std::cout.copyfmt(state);
		}

		{
			std::cout << "Expected: " << std::endl;
			std::ios state(nullptr);
			state.copyfmt(std::cout);
			for (auto byte : bytes) {
				std::cout << std::hex << std::setw(2) << static_cast<int>(byte) << " ";
			}
			std::cout << std::endl;
			std::cout.copyfmt(state);
		}

		return false;
	}

	return true;
}

bool testParser()
{
	auto bool_to_str = [](bool res) {
		return res ? "OK" : "FAILED";
	};

	bool res = true;

	{
		bool inRes = testParserFrame();
		std::cout << "\ttestParserFrame(): " << bool_to_str(inRes) << std::endl;
		res = res && inRes;
	}

	{
		bool inRes = testParserBuffer();
		std::cout << "\ttestParserBuffer(): " << bool_to_str(inRes) << std::endl;
		res = res && inRes;
	}

	{
		bool inRes = testParserMessage();
		std::cout << "\ttestParserMessage(): " << bool_to_str(inRes) << std::endl;
		res = res && inRes;
	}

	return res;
}

#endif // PARSER_HPP
