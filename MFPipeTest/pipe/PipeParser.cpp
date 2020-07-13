#include "PipeParser.hpp"

PipeParser::PipeParser()
{
	syncBytes[0] = static_cast<uint8_t>(DATA_SYNC);
	syncBytes[1] = static_cast<uint8_t>(DATA_SYNC >> 8);
	syncBytes[2] = static_cast<uint8_t>(DATA_SYNC >> (8 * 2));
	syncBytes[3] = static_cast<uint8_t>(DATA_SYNC >> (8 * 3));
	reset();
}

void PipeParser::reset()
{
	state = State::IDLE;
	type = DataType::NONE;
	syncCount = 0;
	chunkSize = 0;
	channel.clear();
	channelSize.clear();
	data.clear();
}

std::vector<uint8_t> PipeParser::getData() const
{
	return data;
}

std::string PipeParser::getChannel() const
{
	return channel;
}

PipeParser::State PipeParser::getState() const
{
	return state;
}

size_t PipeParser::parse(const uint8_t *rawData, size_t size)
{
	size_t pos = 0;

	while (pos < size)
	{
		const uint8_t byte = rawData[pos++];

		switch (state)
		{
			case State::IDLE:
			{
				if (byte == syncBytes[syncCount])
					syncCount++;
				else
					syncCount = 0;

				if (syncCount == 4)
				{
					state = State::CHANNEL_SIZE;
					chunkSize = sizeof(size_t);
				}

				break;
			}

			case State::CHANNEL_SIZE:
			{
				channelSize.push_back(byte);
				chunkSize--;
				if (chunkSize == 0)
				{
					chunkSize = *reinterpret_cast<const size_t *>(channelSize.data());
					state = chunkSize == 0 ? State::DATA_TYPE : State::CHANNEL;
				}
				break;
			}

			case State::CHANNEL:
			{
				channel.push_back(byte);
				chunkSize--;
				if (chunkSize == 0)
					state = State::DATA_TYPE;
				break;
			}

			case State::DATA_TYPE:
			{
				switch (static_cast<DataType>(byte))
				{
					case DataType::FRAME:
						type = DataType::FRAME;
						state = State::FRAME_TIME;
						chunkSize = sizeof(M_TIME);
						break;
					case DataType::BUFFER:
						type = DataType::BUFFER;
						state = State::BUFFER_FLAGS;
						chunkSize = sizeof(eMFBufferFlags);
						break;
					case DataType::MESSAGE:
						type = DataType::MESSAGE;
						state = State::MESSAGE_EVENT_NAME_SIZE;
						chunkSize = sizeof(size_t);
						break;
					default:
						type = DataType::NONE;
						state = State::IDLE;
						break;
				}

				break;
			}

			case State::FRAME_TIME:
			{
				data.push_back(byte);
				chunkSize--;
				if (chunkSize == 0)
				{
					state = State::FRAME_AV_PROPS;
					chunkSize = sizeof(M_AV_PROPS);
				}
				break;
			}

			case State::FRAME_AV_PROPS:
			{
				data.push_back(byte);
				chunkSize--;
				if (chunkSize == 0)
				{
					state = State::FRAME_USER_PROPS_SIZE;
					chunkSize = sizeof(size_t);
				}
				break;
			}

			case State::FRAME_USER_PROPS_SIZE:
			{
				data.push_back(byte);
				chunkSize--;
				if (chunkSize == 0)
				{
					state = State::FRAME_USER_PROPS;
					chunkSize = *reinterpret_cast<const size_t *>(data.data() + data.size() - sizeof(size_t));
				}
				break;
			}

			case State::FRAME_USER_PROPS:
			{
				data.push_back(byte);
				chunkSize--;
				if (chunkSize == 0)
				{
					state = State::FRAME_VIDEO_DATA_SIZE;
					chunkSize = sizeof(size_t);
				}
				break;
			}

			case State::FRAME_VIDEO_DATA_SIZE:
			{
				data.push_back(byte);
				chunkSize--;
				if (chunkSize == 0)
				{
					state = State::FRAME_VIDEO_DATA;
					chunkSize = *reinterpret_cast<const size_t *>(data.data() + data.size() - sizeof(size_t));
				}
				break;
			}

			case State::FRAME_VIDEO_DATA:
			{
				data.push_back(byte);
				chunkSize--;
				if (chunkSize == 0)
				{
					state = State::FRAME_AUDIO_DATA_SIZE;
					chunkSize = sizeof(size_t);
				}
				break;
			}

			case State::FRAME_AUDIO_DATA_SIZE:
			{
				data.push_back(byte);
				chunkSize--;
				if (chunkSize == 0)
				{
					state = State::FRAME_AUDIO_DATA;
					chunkSize = *reinterpret_cast<const size_t *>(data.data() + data.size() - sizeof(size_t));
				}
				break;
			}

			case State::FRAME_AUDIO_DATA:
			{
				data.push_back(byte);
				chunkSize--;
				if (chunkSize == 0)
				{
					state = State::FRAME_READY;
					return pos;
				}
				break;
			}

			case State::BUFFER_FLAGS:
			{
				data.push_back(byte);
				chunkSize--;
				if (chunkSize == 0)
				{
					state = State::BUFFER_DATA_SIZE;
					chunkSize = sizeof(size_t);
				}
				break;
			}

			case State::BUFFER_DATA_SIZE:
			{
				data.push_back(byte);
				chunkSize--;
				if (chunkSize == 0)
				{
					state = State::BUFFER_DATA;
					chunkSize = *reinterpret_cast<const size_t *>(data.data() + data.size() - sizeof(size_t));
				}
				break;
			}

			case State::BUFFER_DATA:
			{
				data.push_back(byte);
				chunkSize--;
				if (chunkSize == 0)
				{
					state = State::BUFFER_READY;
					return pos;
				}
				break;
			}

			case State::MESSAGE_EVENT_NAME_SIZE:
			{
				data.push_back(byte);
				chunkSize--;
				if (chunkSize == 0)
				{
					state = State::MESSAGE_EVENT_NAME;
					chunkSize = *reinterpret_cast<const size_t *>(data.data() + data.size() - sizeof(size_t));
				}
				break;
			}

			case State::MESSAGE_EVENT_NAME:
			{
				data.push_back(byte);
				chunkSize--;
				if (chunkSize == 0)
				{
					state = State::MESSAGE_EVENT_PARAM_SIZE;
					chunkSize = sizeof(size_t);
				}
				break;
			}

			case State::MESSAGE_EVENT_PARAM_SIZE:
			{
				data.push_back(byte);
				chunkSize--;
				if (chunkSize == 0)
				{
					state = State::MESSAGE_EVENT_PARAM;
					chunkSize = *reinterpret_cast<const size_t *>(data.data() + data.size() - sizeof(size_t));
				}
				break;
			}

			case State::MESSAGE_EVENT_PARAM:
			{
				data.push_back(byte);
				chunkSize--;
				if (chunkSize == 0)
				{
					state = State::MESSAGE_READY;
					return pos;
				}
				break;
			}

			default:
				return pos;
		}
	}

	return size;
}
