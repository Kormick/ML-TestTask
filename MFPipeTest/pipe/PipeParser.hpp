#ifndef PIPEPARSER_HPP
#define PIPEPARSER_HPP

#include "../MFTypes.h"

class PipeParser
{
public:
	enum class State
	{
		IDLE = 0x00,
		SYNC,
		DATA_TYPE,

		FRAME_TIME,
		FRAME_AV_PROPS,
		FRAME_USER_PROPS_SIZE,
		FRAME_USER_PROPS,
		FRAME_VIDEO_DATA_SIZE,
		FRAME_VIDEO_DATA,
		FRAME_AUDIO_DATA_SIZE,
		FRAME_AUDIO_DATA,
		FRAME_READY,

		BUFFER_FLAGS,
		BUFFER_DATA_SIZE,
		BUFFER_DATA,
		BUFFER_READY,

		MESSAGE_EVENT_NAME_SIZE,
		MESSAGE_EVENT_NAME,
		MESSAGE_EVENT_PARAM_SIZE,
		MESSAGE_EVENT_PARAM,
		MESSAGE_READY,

		DONE,
	};

	PipeParser();

	void reset();
	const std::vector<uint8_t> getData() const;
	State getState() const;
	size_t parse(const std::vector<uint8_t> &rawData, size_t startPos);

	size_t parse(const uint8_t *rawData, size_t size);

private:
	State state;
	DataType type;
	uint8_t syncBytes[4];
	uint8_t syncCount;
	size_t chunkSize;
	std::vector<uint8_t> data;
};
#endif // PIPEPARSER_HPP
