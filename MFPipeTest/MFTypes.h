#ifndef MF_TYPES_H_
#define MF_TYPES_H_

#include <cmath>
#include <cstdint>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

typedef long long int REFERENCE_TIME;

typedef	enum MF_HRESULT
{
	RES_OK = 0,
	RES_FALSE = 1,
	NOTIMPL = 0x80004001L,
	OUTOFMEMORY = 0x8007000EL,
	INVALIDARG = 0x80070057L
} MF_HRESULT;

enum class DataType
{
	NONE = 0x00,
	FRAME,
	BUFFER,
	MESSAGE,
};

static constexpr uint32_t DATA_SYNC = 0xFBFCFDFE;

typedef struct M_TIME
{
	REFERENCE_TIME rtStartTime;
	REFERENCE_TIME rtEndTime;
} 	M_TIME;

typedef enum eMFCC
{
	eMFCC_Default = 0,
	eMFCC_I420 = 0x30323449,
	eMFCC_YV12 = 0x32315659,
	eMFCC_NV12 = 0x3231564e,
	eMFCC_YUY2 = 0x32595559,
	eMFCC_YVYU = 0x55595659,
	eMFCC_UYVY = 0x59565955,
	eMFCC_RGB24 = 0xe436eb7d,
	eMFCC_RGB32 = 0xe436eb7e,
} 	eMFCC;

typedef struct M_VID_PROPS
{
	eMFCC fccType;
	int nWidth;
	int nHeight;
	int nRowBytes;
	short nAspectX;
	short nAspectY;
	double dblRate;
} 	M_VID_PROPS;

typedef struct M_AUD_PROPS
{
	int nChannels;
	int nSamplesPerSec;
	int nBitsPerSample;
	int nTrackSplitBits;
} 	M_AUD_PROPS;

typedef struct M_AV_PROPS
{
	M_VID_PROPS vidProps;
	M_AUD_PROPS audProps;
} 	M_AV_PROPS;

typedef struct MF_BASE_TYPE
{
	virtual ~MF_BASE_TYPE() {}

	virtual std::vector<uint8_t> serialize() const = 0;
	virtual MF_BASE_TYPE* deserialize(const std::vector<uint8_t> &raw) = 0;
} MF_BASE_TYPE;

typedef struct MF_FRAME: public MF_BASE_TYPE
{
	typedef std::shared_ptr<MF_FRAME> TPtr;

	M_TIME      time = {};
	M_AV_PROPS    av_props = {};
	std::string    str_user_props;
	std::vector<uint8_t> vec_video_data;
	std::vector<uint8_t> vec_audio_data;

	std::vector<uint8_t> serialize() const override
	{
		std::vector<uint8_t> buf;

		auto to_bytes = [&buf](auto data) {
			const auto bytes = reinterpret_cast<const uint8_t *>(&data);
			buf.insert(buf.end(), bytes, bytes + sizeof(data));
		};

		to_bytes(DATA_SYNC);
		to_bytes(static_cast<uint8_t>(DataType::FRAME));
		to_bytes(time);
		to_bytes(av_props);

		to_bytes(str_user_props.size());
		buf.insert(buf.end(), str_user_props.begin(), str_user_props.end());

		to_bytes(vec_video_data.size());
		buf.insert(buf.end(), vec_video_data.begin(), vec_video_data.end());

		to_bytes(vec_audio_data.size());
		buf.insert(buf.end(), vec_audio_data.begin(), vec_audio_data.end());

		return buf;
	}

	MF_BASE_TYPE* deserialize(const std::vector<uint8_t> &raw) override
	{
		auto frame = new MF_FRAME();

		auto rawPtr = raw.data();

		frame->time = *reinterpret_cast<const M_TIME *>(rawPtr);
		rawPtr += sizeof(frame->time);

		frame->av_props = *reinterpret_cast<const M_AV_PROPS *>(rawPtr);
		rawPtr += sizeof(frame->av_props);

		auto str_user_props_size = *reinterpret_cast<const size_t *>(rawPtr);
		rawPtr += sizeof(str_user_props_size);
		frame->str_user_props = std::string(reinterpret_cast<const char *>(rawPtr), str_user_props_size);
		rawPtr += str_user_props_size;

		auto vec_video_data_size = *reinterpret_cast<const size_t *>(rawPtr);
		rawPtr += sizeof(vec_video_data_size);
		frame->vec_video_data = std::vector<uint8_t>(rawPtr, rawPtr + vec_video_data_size);
		rawPtr += vec_video_data_size;

		auto vec_audio_data_size = *reinterpret_cast<const size_t *>(rawPtr);
		rawPtr += sizeof(vec_audio_data_size);
		frame->vec_audio_data = std::vector<uint8_t>(rawPtr, rawPtr + vec_audio_data_size);
		rawPtr += sizeof(vec_audio_data_size);

		return frame;
	}
} MF_FRAME;

typedef enum eMFBufferFlags
{
	eMFBF_Empty = 0,
	eMFBF_Buffer = 0x1,
	eMFBF_Packet = 0x2,
	eMFBF_Frame = 0x3,
	eMFBF_Stream = 0x4,
	eMFBF_SideData = 0x10,
	eMFBF_VideoData = 0x20,
	eMFBF_AudioData = 0x40,
} 	eMFBufferFlags;

typedef struct MF_BUFFER: public MF_BASE_TYPE
{
	typedef std::shared_ptr<MF_BUFFER> TPtr;

	eMFBufferFlags       flags;
	std::vector<uint8_t> data;

	std::vector<uint8_t> serialize() const override
	{
		std::vector<uint8_t> buf;

		auto to_bytes = [&buf](auto data) {
			const auto bytes = reinterpret_cast<const uint8_t *>(&data);
			buf.insert(buf.end(), bytes, bytes + sizeof(data));
		};

		to_bytes(DATA_SYNC);
		to_bytes(static_cast<uint8_t>(DataType::BUFFER));
		to_bytes(flags);

		to_bytes(data.size());
		buf.insert(buf.end(), data.begin(), data.end());

		return buf;
	}

	MF_BASE_TYPE* deserialize(const std::vector<uint8_t> &raw) override
	{
		auto buffer = new MF_BUFFER();

		auto rawPtr = raw.data();

		buffer->flags = *reinterpret_cast<const eMFBufferFlags *>(rawPtr);
		rawPtr += sizeof(buffer->flags);

		auto data_size = *reinterpret_cast<const size_t *>(rawPtr);
		rawPtr += sizeof(data_size);
		buffer->data = std::vector<uint8_t>(rawPtr, rawPtr + data_size);
		rawPtr += data_size;

		return buffer;
	}
} MF_BUFFER;

struct Message
{
	std::string name;
	std::string param;

	std::vector<uint8_t> serialize() const
	{
		std::vector<uint8_t> buf;

		auto to_bytes = [&buf](auto data) {
			const auto bytes = reinterpret_cast<const uint8_t *>(&data);
			buf.insert(buf.end(), bytes, bytes + sizeof(data));
		};

		to_bytes(DATA_SYNC);
		to_bytes(static_cast<uint8_t>(DataType::MESSAGE));

		to_bytes(name.size());
		buf.insert(buf.end(), name.begin(), name.end());

		to_bytes(param.size());
		buf.insert(buf.end(), param.begin(), param.end());

		return buf;
	}

	Message deserialize(const std::vector<uint8_t> &raw)
	{
		Message mes;

		auto rawPtr = raw.data();

		auto nameSize = *reinterpret_cast<const size_t *>(rawPtr);
		rawPtr += sizeof(size_t);
		mes.name = std::string(reinterpret_cast<const char *>(rawPtr), nameSize);
		rawPtr += nameSize;

		auto paramSize = *reinterpret_cast<const size_t *>(rawPtr);
		rawPtr += sizeof(size_t);
		mes.param = std::string(reinterpret_cast<const char *>(rawPtr), paramSize);
		rawPtr += paramSize;

		return mes;
	}
};

struct DataBuffer
{
	std::timed_mutex mutex;
	std::deque<std::shared_ptr<MF_BASE_TYPE>> data;
	std::deque<Message> messages;
};

struct MessageBuffer
{
	std::timed_mutex mutex;
	std::deque<Message> data;
};

inline bool operator==(const M_VID_PROPS &lh, const M_VID_PROPS &rh)
{
	return lh.fccType == rh.fccType
	        && lh.nWidth == rh.nWidth
	        && lh.nHeight == rh.nHeight
	        && lh.nRowBytes == rh.nRowBytes
	        && lh.nAspectX == rh.nAspectX
	        && lh.nAspectY == rh.nAspectY
	        && fabs(lh.dblRate - rh.dblRate) < 0.0001;
}

inline bool operator!=(const M_VID_PROPS &lh, const M_VID_PROPS &rh)
{
	return !(lh == rh);
}

inline bool operator==(const M_TIME &lh, const M_TIME &rh)
{
	return lh.rtStartTime == rh.rtStartTime
	        && lh.rtEndTime == rh.rtEndTime;
}

inline bool operator!=(const M_TIME &lh, const M_TIME &rh)
{
	return !(lh == rh);
}

inline bool operator==(const M_AUD_PROPS &lh, const M_AUD_PROPS &rh)
{
	return lh.nChannels == rh.nChannels
	        && lh.nSamplesPerSec == rh.nSamplesPerSec
	        && lh.nBitsPerSample == rh.nBitsPerSample
	        && lh.nTrackSplitBits == rh.nTrackSplitBits;
}

inline bool operator!=(const M_AUD_PROPS &lh, const M_AUD_PROPS &rh)
{
	return !(lh == rh);
}

inline bool operator==(const M_AV_PROPS &lh, const M_AV_PROPS &rh)
{
	return lh.vidProps == rh.vidProps
	        && lh.audProps == rh.audProps;
}

inline bool operator!=(const M_AV_PROPS &lh, const M_AV_PROPS &rh)
{
	return !(lh == rh);
}

inline bool operator==(const MF_FRAME &lh, const MF_FRAME &rh)
{
	return lh.time == rh.time
	        && lh.av_props == rh.av_props
	        && lh.str_user_props == rh.str_user_props
	        && lh.vec_video_data == rh.vec_video_data
	        && lh.vec_audio_data == rh.vec_audio_data;
}

inline bool operator!=(const MF_FRAME &lh, const MF_FRAME &rh)
{
	return !(lh == rh);
}

inline bool operator==(const MF_BUFFER &lh, const MF_BUFFER &rh)
{
	return lh.flags == rh.flags
	        && lh.data == rh.data;
}

inline bool operator!=(const MF_BUFFER &lh, const MF_BUFFER &rh)
{
	return !(lh == rh);
}
#endif
